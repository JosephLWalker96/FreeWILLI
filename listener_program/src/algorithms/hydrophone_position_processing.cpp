#include "hydrophone_position_processing.h"
#include "linear_algebra_utils.h"

/**
 * @brief Loads hydrophone positions from a CSV file into an Eigen matrix.
 *
 * This function reads a CSV file containing hydrophone positions and stores the data in an Eigen matrix.
 *
 * @param filename The name (or path) of the CSV file containing hydrophone positions.
 * @return Eigen::MatrixXf A matrix containing the hydrophone positions.
 * @throws std::ios_base::failure If the file cannot be opened or read.
 */
Eigen::MatrixXf loadHydrophonePositionsFromFile(const std::string &filename)
{
    std::ifstream inputFile(filename);

    if (!inputFile.is_open())
    {
        std::stringstream msg; // compose message to dispatch
        msg << "Error: Unable to open hydrophone position file '" << filename << "'." << std::endl;
        throw std::ios_base::failure(msg.str());
    }

    std::cout << "Reading hydrophone positions..." << std::endl;

    std::vector<std::vector<float>> tempPositions;
    std::string line;
    std::string token;

    // First, read the data into a temporary vector of vectors to determine dimensions
    int numRows = 0;
    int numCols = 0;
    while (std::getline(inputFile, line))
    {
        std::stringstream ss(line);
        std::vector<float> rowData;
        while (std::getline(ss, token, ','))
        {
            rowData.push_back(std::stof(token)); // Convert token to double
        }
        if (numCols == 0)
        {
            numCols = rowData.size(); // Set number of columns based on the first row
        }
        tempPositions.push_back(rowData);
        numRows++;
    }

    // Now create the Eigen matrix with determined dimensions
    Eigen::MatrixXf positions(numRows, numCols);

    // Populate the Eigen matrix with the data from the temp_positions vector
    for (int i = 0; i < numRows; ++i)
    {
        for (int j = 0; j < numCols; ++j)
        {
            positions(i, j) = tempPositions[i][j];
        }
    }
    return positions;
}

/**
 * @brief Computes relative positions between hydrophones.
 *
 * This function calculates relative positions between all pairs of hydrophones based on their absolute positions.
 *
 * @param positions An Eigen matrix containing the absolute positions of hydrophones.
 * @return Eigen::MatrixXf A matrix containing the relative positions between hydrophones.
 */
Eigen::MatrixXf calculateRelativePositions(const Eigen::MatrixXf &positions)
{
    int numRows = positions.rows();
    int numCols = positions.cols();
    int numRelativePositions = numRows * (numRows - 1) / 2;

    Eigen::MatrixXf relativePositions(numRelativePositions, numCols);

    int index = 0;
    for (int i = 0; i < numRows; ++i)
    {
        for (int j = i + 1; j < numRows; ++j)
        {
            for (int k = 0; k < numCols; ++k)
            {
                relativePositions(index, k) = positions(j, k) - positions(i, k);
            }
            index++;
        }
    }

    return relativePositions;
}

/**
 * @brief Loads hydrophone positions from a CSV file and calculates relative positions.
 *
 * This function reads a CSV file containing hydrophone positions, converts the data into an Eigen matrix,
 * and calculates relative positions between all hydrophone pairs.
 *
 * @param filename The name (or path) of the CSV file containing the hydrophone positions.
 * @return Eigen::MatrixXf A matrix containing the relative positions between hydrophones.
 * @throws std::ios_base::failure If the file cannot be opened or read.
 */
Eigen::MatrixXf getHydrophoneRelativePositions(const std::string &filename)
{
    Eigen::MatrixXf positions = loadHydrophonePositionsFromFile(filename);

    return calculateRelativePositions(positions);
}

/**
 * @brief Precomputes hydrophone-related matrices required for TDOA and DOA estimation.
 *
 * @param receiverPositionsPath Path to the file containing hydrophone positions.
 * @param precomputedP Reference to a matrix for storing the precomputed P matrix.
 * @param basisMatrixU Reference to a matrix for storing the U matrix from SVD decomposition.
 * @param rankOfHydrophoneMatrix Reference to an integer for storing the rank of the hydrophone position matrix.
 */
auto hydrophoneMatrixDecomposition(const Eigen::MatrixXf& hydrophonePositions) -> std::tuple<Eigen::MatrixXf, Eigen::MatrixXf, int>
{
    // Compute SVD and rank
    auto svdDecomposition = computeSvd(hydrophonePositions);
    int rankOfHydrophoneMatrix = computeRank(hydrophonePositions, 1e-6);

    // Precompute matrices
    Eigen::MatrixXf precomputedP = precomputePseudoInverse(svdDecomposition);
    Eigen::MatrixXf basisMatrixU = svdDecomposition.matrixU();
    return std::make_tuple(precomputedP, basisMatrixU, rankOfHydrophoneMatrix);
}