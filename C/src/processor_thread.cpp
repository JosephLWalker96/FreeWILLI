#include "custom_types.h"
#include "process_data.h"
#include "TDOA_estimation.h"
#include "utils.h"
#include "pch.h"
#include "session.h"
#include "buffer_writter.h"

using TimePoint = std::chrono::system_clock::time_point;

void DataProcessor(Session &sess, ExperimentConfig &expConfig, ExperimentRuntime &expRuntime)
{
    /**
     * @brief Processes data segments from a shared buffer, performs filtering and analysis.
     *
     * This function continuously processes data segments retrieved from a shared buffer (`dataBuffer`).
     * It extracts timestamps and sample values, applies necessary adjustments and filters, and stores
     * the processed data into a segment (`dataSegment`). The processed data is then further analyzed
     * to detect pulses, apply filters, and estimate time differences and directions of arrival.
     *
     * @param expConfig Reference to an Experiment object containing configuration details like data segment length,
     *            number of channels, filter weights, and other processing parameters.
     * @param sess Reference to a Session object containing session-specific details and state, such as the
     *             data buffer, segment buffer, and various locks for synchronization.
     *
     * @throws std::runtime_error if there is an error in processing data, such as incorrect packet sizes
     *                            or unexpected time increments.
     */

    try
    {

        // the number of samples per channel within a dataSegment
        int channelSize = expConfig.DATA_SEGMENT_LENGTH / expConfig.NUM_CHAN;

        // Read filter weights from file
        std::vector<float> filterWeightsFloat = ReadFIRFilterFile(expRuntime.filterWeights);

        // Declare time checking variables
        bool previousTimeSet = false;
        auto previousTime = std::chrono::time_point<std::chrono::system_clock>::min();

        // pre-allocate memory for vectors
        sess.dataSegment.reserve(expConfig.DATA_SEGMENT_LENGTH);
        sess.dataTimes.reserve(expConfig.NUM_PACKS_DETECT);

        int paddedLength = filterWeightsFloat.size() + channelSize - 1;
        int fftOutputSize = (paddedLength / 2) + 1;
        std::cout << "Padded size: " << paddedLength << std::endl;

        // Matrices for (transformed) channel data
        static Eigen::MatrixXf channelData(paddedLength, expConfig.NUM_CHAN);
        static Eigen::MatrixXcf savedFFTs(fftOutputSize, expConfig.NUM_CHAN); // save the FFT transformed channels

        /* Zero-pad filter weights to the length of the signal                     */
        std::vector<float> paddedFilterWeights(paddedLength, 0.0f);
        std::copy(filterWeightsFloat.begin(), filterWeightsFloat.end(), paddedFilterWeights.begin());

        // Create frequency domain filter
        Eigen::VectorXcf filterFreq(fftOutputSize);
        fftwf_plan fftFilter = fftwf_plan_dft_r2c_1d(paddedLength, paddedFilterWeights.data(), reinterpret_cast<fftwf_complex *>(filterFreq.data()), FFTW_ESTIMATE);
        fftwf_execute(fftFilter);
        fftwf_destroy_plan(fftFilter);

        // Container for pulling bytes from buffer (dataBuffer)
        std::vector<uint8_t> dataBytes;

        // Create the FFTW plan with the correct strides
        expRuntime.forwardFFT = fftwf_plan_many_dft_r2c(1,                                                   // Rank of the transform (1D)
                                                &paddedLength,                                       // Pointer to the size of the transform
                                                4,                                                   // Number of transforms (channels)
                                                channelData.data(),                                  // Input data pointer
                                                nullptr,                                             // No embedding (we're not doing multi-dimensional transforms)
                                                1,                                                   // Stride between successive elements in input
                                                paddedLength,                                        // Stride between successive channels in input
                                                reinterpret_cast<fftwf_complex *>(savedFFTs.data()), // Output data pointer
                                                nullptr,                                             // No embedding
                                                1,                                                   // Stride between successive elements in output
                                                fftOutputSize,                                       // Stride between successive channels in output
                                                FFTW_ESTIMATE);                                       // Flag to measure and optimize the plan

        // Fill the matrices with zeros
        channelData.setZero();
        savedFFTs.setZero();

        Eigen::MatrixXd H = LoadHydrophonePositions(expRuntime.receiverPositions);
        
        // Precompute QR decomposition once
        //auto qrDecompH = precomputedQR(H);
        auto svd = SVD(H);
        int rankOfH = GetRank(H);
        // Precompute P and extract U
        Eigen::MatrixXd P = precomputeInverse(svd);
        Eigen::MatrixXd U = svd.matrixU();

        // set the frequency of file writes
        ObservationBuffer observationBuffer;
        observationBuffer._flushInterval = 30s;
        observationBuffer._bufferSizeThreshold = 1000; // Adjust as needed
        observationBuffer._lastFlushTime = std::chrono::steady_clock::now();
        std::cout << "Ready to process data..." << std::endl;
        while (!sess.errorOccurred)
        {

            sess.dataTimes.clear();
            sess.dataSegment.clear();
            sess.dataBytesSaved.clear();

            while (sess.dataSegment.size() < expConfig.DATA_SEGMENT_LENGTH)
            {

                auto startLoop = std::chrono::system_clock::now();

                while (true){
                    bool gotData = sess.popDataFromBuffer(dataBytes); // this function will send the thread to sleep until data is available 
                    if (gotData){
                        break;
                    }
                    ShouldFlushBuffer(observationBuffer, expRuntime, startLoop);
                    std::this_thread::sleep_for(std::chrono::milliseconds(15ms));
                }

                sess.dataBytesSaved.push_back(dataBytes); // save bytes in case they need to be saved to a file in case of error

                auto currentTimestamp = GenerateTimestamp(dataBytes, expConfig.NUM_CHAN);
                sess.dataTimes.push_back(currentTimestamp);
                bool dataError = CheckForDataErrors(sess, dataBytes, expConfig.MICRO_INCR, previousTimeSet, previousTime, expConfig.PACKET_SIZE);

                if (!dataError) [[likely]]{
                    ConvertAndAppend(sess.dataSegment, dataBytes, expConfig.DATA_SIZE, expConfig.HEAD_SIZE); // bytes data is decoded and appended to sess.dataSegment
                }

                if  ((expRuntime.detectionOutputFile).empty()) [[unlikely]] {
                    InitiateOutputFile(expRuntime.detectionOutputFile, currentTimestamp, expConfig.NUM_CHAN);
                }

            }

            /*
             *   Exited inner loop - dataSegment has been filled to 'DATA_SEGMENT_LENGTH' length
             *   now apply energy detector.
             */

            auto beforePtr = std::chrono::steady_clock::now();
            expConfig.ProcessFncPtr(sess.dataSegment, channelData, expConfig.NUM_CHAN);
            auto afterPtr = std::chrono::steady_clock::now();
            std::chrono::duration<double> durationPtr = afterPtr - beforePtr;
            //std::cout << "durationPtr: " << durationPtr.count() << std::endl;

            DetectionResult threshResult = ThresholdDetect(channelData.col(0), sess.dataTimes, expRuntime.ampDetThresh, expConfig.SAMPLE_RATE);
            if (threshResult.maxPeakIndex < 0){
                continue;
            }

            //auto beforeFilter = std::chrono::steady_clock::now();
            FrequencyDomainFIRFiltering(
                channelData,           // Zero-padded time-domain data
                filterFreq,            // Frequency domain filter (FIR taps in freq domain)
                expRuntime.forwardFFT, // FFT plan
                savedFFTs);            // Output of FFT transformed time-domain data
            //auto afterFilter = std::chrono::steady_clock::now();
            //std::chrono::duration<double> durationFilter = afterFilter - beforeFilter;

            
            DetectionResult detResult = ThresholdDetectFD(savedFFTs.col(0), sess.dataTimes, 
                                                          expRuntime.energyDetThresh, expConfig.SAMPLE_RATE);

            if (detResult.maxPeakIndex < 0)
            {             // if no pulse was detected (maxPeakIndex remains -1) stop processing
                continue; // get next dataSegment; return to loop
            }

            /*
             *  Pulse detected. Now process the channels filtering, TDOA & DOA estimation.
             */

            sess.detectionCounter++;
            //std::cout << "Filter runtime: " << durationFilter.count() << std::endl;

            //auto beforeGCCW = std::chrono::steady_clock::now();
            std::tuple<Eigen::VectorXf, Eigen::VectorXf> tdoasAndXCorrAmps = GCC_PHAT(savedFFTs, expRuntime.inverseFFT, expConfig.interp, paddedLength, expConfig.NUM_CHAN, expConfig.SAMPLE_RATE);
            //auto afterGCCW = std::chrono::steady_clock::now();
            //std::chrono::duration<double> durationGCCW = afterGCCW - beforeGCCW;
            //std::cout << "GCC time: " << durationGCCW.count() << std::endl;
            Eigen::VectorXf tdoaVector = std::get<0>(tdoasAndXCorrAmps);
            Eigen::VectorXf XCorrAmps = std::get<1>(tdoasAndXCorrAmps);
            //std::cout << "TDOAs: " << tdoaVector.transpose() << std::endl;
            //std::cout << "GCC time: " << durationGCCW.count() << std::endl;

            //auto beforeDOA = std::chrono::steady_clock::now();
            Eigen::VectorXf DOAs = TDOA_To_DOA_Optimized(P, U, expRuntime.speedOfSound, tdoaVector, rankOfH);
            Eigen::VectorXf AzEl = DOA_to_ElAz(DOAs);
            //auto afterDOA = std::chrono::steady_clock::now();
            //std::chrono::duration<double> durationDOA = afterDOA - beforeDOA;
            //std::cout << "DOA time: " << durationDOA.count() << std::endl;

            //Eigen::VectorXf DOAs = TDOA_To_DOA_VerticalArray(tdoaVector, 1500.0, chanSpacing);
            std::cout << "AzEl: " << AzEl.transpose() << std::endl;
            //std::cout << "Energy: " << detResult.peakAmplitude << std::endl;
            //std::cout << "tdoa: " << tdoaVector.transpose() << std::endl;

            // Write to buffers
            //Eigen::VectorXf combined(1 + DOAs.size() + tdoaVector.size() + XCorrAmps.size()); // + 1 for amplitude
            

            observationBuffer.Buffer.amps.push_back(detResult.peakAmplitude);
            observationBuffer.Buffer.DOA_x.push_back(DOAs[0]);
            observationBuffer.Buffer.DOA_y.push_back(DOAs[1]);
            observationBuffer.Buffer.DOA_z.push_back(DOAs[2]);
            observationBuffer.Buffer.tdoaVector.push_back(tdoaVector);
            observationBuffer.Buffer.XCorrAmps.push_back(XCorrAmps);
            observationBuffer.Buffer.peakTimes.push_back(threshResult.peakTimes);
            //observationBuffer.Buffer.amps.push_back(combined);

            // Normalize the input data
            /*
            if (expRuntime.onnxModel){
                std::vector<float> input_tensor_values = GetExampleClick();
                // Run inference
                auto beforeClass = std::chrono::steady_clock::now();
                std::vector<float> predictions = expRuntime.onnxModel->run_inference(input_tensor_values);
                auto afterClass = std::chrono::steady_clock::now();
                std::chrono::duration<double> durationClass = afterClass - beforeClass;
                std::cout << "classifier runtime: " << durationClass.count() << std::endl;
                for(int i = 0; i < predictions.size(); i++){
                    std::cout << predictions[i] << " ";
                }
                std::cout << std::endl;
            }
            */
        }
    }
    catch (const GCC_Value_Error &e)
    {

        std::stringstream msg; // compose message to dispatch
        msg << e.what() << std::endl;
        std::cerr << msg.str();

        try
        {
            WriteDataToCerr(sess.dataTimes, sess.dataBytesSaved);
        }
        catch (...)
        {
            std::cerr << "failed to write data to cerr \n";
        }
        sess.errorOccurred = true;
    }
    catch (const std::ios_base::failure &e)
    {
        std::stringstream msg; // compose message to dispatch
        msg << e.what() << std::endl;
        std::cerr << msg.str();
        std::exit(1);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error occured in data processor thread: \n";

        std::stringstream msg; // compose message to dispatch
        msg << e.what() << std::endl;
        std::cerr << msg.str();

        try
        {
            WriteDataToCerr(sess.dataTimes, sess.dataBytesSaved);
        }
        catch (...)
        {
            std::cerr << "failed to write data to cerr \n";
        }
        sess.errorOccurred = true;
        std::cerr << "End of catch statement\n";
    }
}
