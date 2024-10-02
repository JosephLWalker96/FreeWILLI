#include "custom_types.h"
#include "process_data.h"
#include "TDOA_estimation.h"
#include "utils.h"
#include "pch.h"
#include "session.h"
#include "buffer_writter.h"


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
        //auto qrDecompH = precomputedPseudoInverse(H);
        /*
        for (int i = 0; i < qrDecompH.rows(); i++){
            for (int j = 0; j < qrDecompH.cols(); j++){
                std::cout << qrDecompH(i,j) << " ";
            }
            std::cout << std::endl; 
        }
        */

        // set the frequency of file writes
        BufferWriter bufferWriter;
        bufferWriter._flushInterval = 30s;
        bufferWriter._bufferSizeThreshold = 1000; // Adjust as needed
        bufferWriter._lastFlushTime = std::chrono::steady_clock::now();

        while (!sess.errorOccurred)
        {

            sess.dataTimes.clear();
            sess.dataSegment.clear();
            sess.dataBytesSaved.clear();

            while (sess.dataSegment.size() < expConfig.DATA_SEGMENT_LENGTH)
            {

                auto startLoop = std::chrono::system_clock::now();
                ShouldFlushBuffer(bufferWriter, sess, expRuntime, startLoop);
                // Check if program has run for specified time

                dataBytes = sess.popDataFromBuffer(); // this function will send the thread to sleep until data is available 

                sess.dataBytesSaved.push_back(dataBytes); // save bytes in case they need to be saved to a file in case of error

                //auto startTimestamps = std::chrono::steady_clock::now();
                bool timestampErrOccured = GenerateTimestamps(sess.dataTimes, dataBytes, expConfig.MICRO_INCR,
                                           previousTimeSet, previousTime, expRuntime.detectionOutputFile, expConfig.NUM_CHAN);
                //auto endTimestamps = std::chrono::steady_clock::now();
                //std::chrono::duration<double> durationGenerate = endTimestamps - startTimestamps;
                // std::cout << "durationGenerate: " << durationGenerate.count() << std::endl;
                

                if (timestampErrOccured){ // Check if the sequential timestamps differ by unexpected amount 
                    WriteDataToCerr(sess.dataTimes, sess.dataBytesSaved);
                    sess.dataTimes.clear();
                    sess.dataSegment.clear();
                    sess.dataBytesSaved.clear();
                    previousTime = std::chrono::time_point<std::chrono::system_clock>::min();
                    previousTimeSet = false;
                }
                else if (dataBytes.size() != expConfig.PACKET_SIZE){ // Check if the amount of bytes in packet is what is expected
                    std::stringstream msg; // compose message to dispatch
                    msg << "Error: incorrect number of bytes in packet: " << "PACKET_SIZE: " << expConfig.PACKET_SIZE << " dataBytes size: " << dataBytes.size() << std::endl;
                    throw std::runtime_error(msg.str());
                }
                else {
                    // Convert byte data to floats
                    //auto startCDTime = std::chrono::steady_clock::now();
                    ConvertAndAppend(sess.dataSegment, dataBytes, expConfig.DATA_SIZE, expConfig.HEAD_SIZE); // bytes data is decoded and appended to sess.dataSegment
                    //auto endCDTime = std::chrono::steady_clock::now();
                    //std::chrono::duration<double> durationCD = endCDTime - startCDTime;
                    // std::cout << "Convert data: " << durationCD.count() << std::endl;
                }
            }

            /*
             *   Exited inner loop - dataSegment has been filled to 'DATA_SEGMENT_LENGTH' length
             *   now apply energy detector.
             */

            //auto beforePtr = std::chrono::steady_clock::now();
            expConfig.ProcessFncPtr(sess.dataSegment, channelData, expConfig.NUM_CHAN);
            //auto afterPtr = std::chrono::steady_clock::now();
            //std::chrono::duration<double> durationPtr = afterPtr - beforePtr;

            //auto beforeFilter = std::chrono::steady_clock::now();
            FrequencyDomainFIRFiltering(
                channelData,   // Zero-padded time-domain data
                filterFreq,    // Frequency domain filter (FIR taps in freq domain)
                expRuntime.forwardFFT, // FFT plan
                savedFFTs);    // Output of FFT transformed time-domain data
            //auto afterFilter = std::chrono::steady_clock::now();
            //std::chrono::duration<double> durationFilter = afterFilter - beforeFilter;

            // DetectionResult detResult = ThresholdDetect(invFFT.col(0), sess.dataTimes, expConfig.energyDetThresh, expConfig.SAMPLE_RATE);
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

            // Eigen::VectorXf DOAs = DOA_EstimateVerticalArray(resultMatrix, expConfig.speedOfSound, expConfig.chanSpacing);
            //auto beforeDOA = std::chrono::steady_clock::now();
            //Eigen::VectorXf DOAs = TDOA_To_DOA_SVD(qrDecompH, expRuntime.speedOfSound, tdoaVector);
            Eigen::VectorXf DOAs = TDOA_To_DOA_Optimized(P, U, expRuntime.speedOfSound, tdoaVector, rankOfH);
            //auto afterDOA = std::chrono::steady_clock::now();
            //std::chrono::duration<double> durationDOA = afterDOA - beforeDOA;
            //std::cout << "DOA time: " << durationDOA.count() << std::endl;

            //std::vector<float> chanSpacing = {1.0, 2.0, 3.0, 1.0, 2.0, 1.0};
            //Eigen::VectorXf DOAs = TDOA_To_DOA_VerticalArray(tdoaVector, 1500.0, chanSpacing);
            //std::cout << "DOAs: " << DOAs.transpose() << std::endl;

            // Write to buffers
            Eigen::VectorXf combined(1 + DOAs.size() + tdoaVector.size() + XCorrAmps.size()); // + 1 for amplitude
            combined << detResult.peakAmplitude, DOAs, tdoaVector, XCorrAmps;
            sess.peakTimesBuffer.push_back(detResult.peakTimes);
            sess.Buffer.push_back(combined);



            // Normalize the input data
            /*
            std::vector<float> input_tensor_values = {
    92.1948, 96.1963, 101.122, 104.773, 107.151, 108.565, 109.293, 109.541, 109.451, 109.168, 
    108.921, 108.999, 109.475, 110.056, 110.395, 110.349, 109.977, 109.466, 109.018, 108.68, 
    108.28, 107.571, 106.516, 105.596, 105.651, 106.584, 107.503, 108.037, 108.182, 107.898, 
    107.157, 105.957, 104.561, 103.774, 104.144, 104.945, 105.38, 105.203, 104.43, 103.139, 
    101.439, 99.5627, 97.9943, 97.3363, 97.4403, 97.6794, 97.7522, 97.6871, 97.5253, 97.2252, 
    96.8271, 96.475, 96.1248, 95.6222, 94.9328, 94.1597, 93.6609, 93.7281, 94.0786, 94.1562, 
    93.6164, 92.3879, 90.4895, 88.4291, 87.6331, 88.309, 88.8974, 88.7232, 88.0315, 87.4041, 
    86.8748, 85.9485, 84.4688, 82.9816, 81.7751, 79.9937, 78.3869, 79.4843, 80.6382, 80.135, 
    78.6906, 78.3676, 80.49, 83.8795, 86.785, 88.5948, 89.3266, 89.1901, 88.2955, 86.7553, 
    84.8847, 83.6774, 84.5187, 86.541, 88.1887, 89.0825, 89.3545, 89.3343, 89.3493, 89.5171, 
    89.751, 89.7528, 89.2266, 88.0493, 86.4237, 85.5187, 86.7017, 88.6813, 90.1444, 90.944, 
    91.2794, 91.3301, 91.2617, 91.0691, 90.6381, 89.8405, 88.4528, 86.3709, 84.3802, 84.9515, 
    87.2957, 89.235, 90.4691, 91.233, 91.6813, 91.9263, 92.2548, 92.8246, 93.2115, 92.905, 
    91.6683, 89.3315, 85.8135, 81.2775, 76.8763, 77.3899, 81.2361, 84.1679, 85.5111, 85.1557, 
    82.9623, 81.2194, 84.8278, 88.5298, 90.6789, 91.8252, 92.4424, 92.7654, 92.9029, 92.9495, 
    92.9146, 92.6751, 92.1435, 91.2272, 90.0295, 89.3202, 89.9183, 91.1307, 91.9888, 92.2035, 
    91.9298, 91.4551, 91.174, 91.2775, 91.702, 92.2776, 92.9613, 93.6848, 94.2458, 94.4455, 
    94.2079, 93.6266, 92.9577, 92.4459, 92.1938, 92.2134, 92.6957, 93.7765, 95.1637, 96.4559, 
    97.3677
};
            
            
            auto beforeClass = std::chrono::steady_clock::now();
            expConfig.onnxModel->normalize_data(input_tensor_values);
            // Run inference
            std::vector<float> predictions = expConfig.onnxModel->run_inference(input_tensor_values);
            auto afterClass = std::chrono::steady_clock::now();
            std::chrono::duration<double> durationClass = afterClass - beforeClass;
            std::cout << "classifier runtime: " << durationClass.count() << std::endl;
            */
            /**
            for(int i = 0; i < predictions.size(); i++){
                std::cout << predictions[i] << " ";
            }
            std::cout << std::endl;
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
