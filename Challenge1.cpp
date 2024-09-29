#include <Eigen/Dense>
#include <iostream>
#include <random>
#include <unsupported/Eigen/SparseExtra>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

using namespace Eigen;
typedef Eigen::Triplet<double> T;

/******************This is the defined functions field like convolution, filter, export matrix, etc***********************/
// Definite the convolution function by matrix production, return the sparsematrix mn*mn
SparseMatrix<double, RowMajor> convolutionMatrix(const Matrix<double, Dynamic, Dynamic, RowMajor> &kernel, int height, int width)
{
    const int kernel_size = kernel.rows();
    const int m = height;
    const int n = width;
    const int mn = m * n;
    SparseMatrix<double, RowMajor> A(mn, mn);

    std::vector<T> hav2TripletList;
    hav2TripletList.reserve(mn * kernel_size * kernel_size);

    for (int i = 0; i < m; i++)
    {
        for (int j = 0; j < n; j++)
        {
            int index_Ai = i * n + j; // row number of A
            // ki, kj are respectively row index and column index of kernel, central point is (0,0)
            for (int ki = -kernel_size / 2; ki <= kernel_size / 2; ki++)
            {
                for (int kj = -kernel_size / 2; kj <= kernel_size / 2; kj++)
                {
                    int ci = i + ki; // contribute to n(width) shift each time when there is a vertical moving for convolution or inside the kernel
                    int cj = j + kj; // contribute just 1 shift each when there is a horizontal moving for convilution or inside the kernel
                    if (ci >= 0 && ci < m && cj >= 0 && cj < n)
                    {
                        int index_Aj = ci * n + cj; // column number of A
                        hav2TripletList.push_back(Triplet<double>(index_Ai, index_Aj, kernel(ki + kernel_size / 2, kj + kernel_size / 2)));
                    }
                }
            }
        }
    }
    // get the sparsematrix from tripletlist
    A.setFromTriplets(hav2TripletList.begin(), hav2TripletList.end());
    return A;
}

// Define the filter function by passing the convolutionMatrix type and vector data parameters, finally output it
void filterImage(const SparseMatrix<double, RowMajor> convolutionMatrix, const VectorXd vectorData, int height, int width, const std::string path)
{
    // Perform a matrix production for sharpening and convert the vector to matrix
    VectorXd filtered_image_vector = convolutionMatrix * vectorData;
    Matrix<double, Dynamic, Dynamic, RowMajor> sharpen_image_matrix(height, width);
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            sharpen_image_matrix(i, j) = filtered_image_vector(i * width + j);
        }
    }

    // Convert the sharpened image to grayscale and export it using stbi_write_png
    Matrix<unsigned char, Dynamic, Dynamic, RowMajor> sharpen_image_output = sharpen_image_matrix.unaryExpr(
        [](double pixel)
        { return static_cast<unsigned char>(std::max(0.0, std::min(255.0, pixel * 255))); }); // ensure range [0,255]
    stbi_write_png(path.c_str(), width, height, 1, sharpen_image_output.data(), width);
}

// Export the vector
void exportVector(VectorXd data, const std::string path)
{
    FILE *out = fopen(path.c_str(), "w");
    fprintf(out, "%%%%Vector Image Data Matrix coordinate real general\n");
    fprintf(out, "size:%d\n", data.size());
    for (int i = 0; i < data.size(); i++)
    {
        fprintf(out, "%f ", data(i));
    }
    fclose(out);
}

// Export a sparse matrix by saveMarket
void exprotSparsematrix(SparseMatrix<double, RowMajor> data, const std::string path)
{
    if (saveMarket(data, path))
    {
        std::cout << "Sparse matrix A2 saved to " << path << std::endl;
    }
    else
    {
        std::cerr << "Error: Could not save sparse matrix A2 to " << path << std::endl;
    }
}
/*-------------------------------------------------Main()-------------------------------------------------------*/
int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <image_path>" << std::endl;
        return 1;
    }

    const char *input_image_path = argv[1];

    /*****************************Load the image using stb_image****************************/
    int width, height, channels;
    // for greyscale images force to load only one channel
    unsigned char *image_data = stbi_load(input_image_path, &width, &height, &channels, 1);
    if (!image_data)
    {
        std::cerr << "Error: Could not load image " << input_image_path << std::endl;
        return 1;
    }

    std::cout << "Image loaded: " << width << "x" << height << " with " << channels << " channels." << std::endl;

    /****************Convert the image_data to MatrixXd form, each element value [0,1]*******************/
    // Attention! if use MatrixXd here its columnmajor by default, we prefere rowmajor
    Matrix<double, Dynamic, Dynamic, RowMajor> original_image_matrix(height, width);

    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            int index = (i * width + j) * channels;
            original_image_matrix(i, j) = static_cast<double>(image_data[index]) / 255;
        }
    }

    // Report the size of the matrix
    std::cout << "The Image Matrix Size Is: " << original_image_matrix.rows() << "*" << original_image_matrix.cols()
              << "=" << original_image_matrix.size() << std::endl;

    /***********************************Introduce noise and export****************************************/
    Matrix<double, Dynamic, Dynamic, RowMajor> noised_image_matrix(height, width);
    // Uniform random number generator for noise
    std::default_random_engine generator;
    std::uniform_int_distribution<int> distribution(-50, 50);

    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            int noise = distribution(generator);
            double noisedData = original_image_matrix(i, j) + static_cast<double>(noise) / 255;
            noised_image_matrix(i, j) = std::max(0.0, std::min(1.0, noisedData)); // ensure the value in [0,1]
        }
    }

    // Convert noise image to grayscale and export it by using stbi_write_png
    Matrix<unsigned char, Dynamic, Dynamic, RowMajor> noised_image_output = noised_image_matrix.unaryExpr(
        [](double pixel)
        { return static_cast<unsigned char>(pixel * 255); });
    const std::string noised_image_path = "NoisedImage.png";
    stbi_write_png(noised_image_path.c_str(), width, height, 1, noised_image_output.data(), width);

    /********************By map creat a vector reference to memeory without copying data***********************/
    // It is columnmajor by default, however, we've already declared our data rowmajor so here rowmajor as well.
    Map<VectorXd> v(original_image_matrix.data(), original_image_matrix.size());
    Map<VectorXd> w(noised_image_matrix.data(), noised_image_matrix.size());

    // Verify the size of the vectors
    std::cout << "Original image vector v's size: " << v.size() << std::endl;
    std::cout << "Noisy image vector w's size: " << w.size() << std::endl;
    std::cout << "Euclidean norm of v is: " << v.norm() << std::endl;

    /****************************Create different kernels and perform convolution*****************************/
    const int kernel_size = 3;

    // Create the kernel H_{av2}
    const double hav2_value = 1.0 / (kernel_size * kernel_size);
    Matrix<double, kernel_size, kernel_size, RowMajor> hav2;
    hav2.setConstant(hav2_value);
    // Perform convolution function
    SparseMatrix<double, RowMajor> A1 = convolutionMatrix(hav2, height, width);
    // Check the nonzero numbers
    std::cout << "A1 nonzero numbers is " << A1.nonZeros() << std::endl;
    // Smooth the noise image by using this filterImage function and passing data and path into it
    const std::string smooth_image_path = "smoothedImage.png";
    filterImage(A1, w, height, width, smooth_image_path);

    // Create the kernel H_{sh2}}
    Matrix<double, kernel_size, kernel_size, RowMajor> hsh2;
    hsh2 << 0.0, -3.0, 0.0,
        -1.0, 9.0, -3.0,
        0.0, -1.0, 0.0;
    // Perform convolution function
    SparseMatrix<double, RowMajor> A2 = convolutionMatrix(hsh2, height, width);
    // Check the nonzero numbers
    std::cout << "A2 nonzero numbers is " << A2.nonZeros() << std::endl;
    // Verify if the matrix is symmetric, attention that A2.tranpose() should be type declaration
    double norm_diff_A2 = (A2 - SparseMatrix<double, RowMajor>(A2.transpose())).norm();
    std::cout << "A2 row:" << A2.rows() << "\tcolumns:" << A2.cols() << std::endl;
    std::cout << "Check if A2 is symmectric by norm value of its difference with transpose:"
              << norm_diff_A2 << std::endl;
    // Sharpen the original image
    const std::string sharpen_image_path = "sharpenedImage.png";
    filterImage(A2, v, height, width, sharpen_image_path);

    // Create the kernel H_{sh2}}
    Matrix<double, kernel_size, kernel_size, RowMajor> hlap;
    hlap << 0.0, -1.0, 0.0,
        -1.0, 4.0, -1.0,
        0.0, -1.0, 0.0;
    // Perform convolution function
    SparseMatrix<double, RowMajor> A3 = convolutionMatrix(hlap, height, width);
    // Check if A3 is symmectic
    double norm_diff_A3 = (A3 - SparseMatrix<double, RowMajor>(A3.transpose())).norm();
    std::cout << "A2 row:" << A3.rows() << "\tcolumns:" << A3.cols() << std::endl;
    std::cout << "Check if A3 is symmectric by norm value of its difference with transpose:"
              << norm_diff_A3 << std::endl;
    // Edge detection of the original image
    const std::string edgeDetection_image_path = "edgeDetectionImage.png";
    filterImage(A3, v, height, width, edgeDetection_image_path);

    // Export the sparse matrix A1 A2 A3
    const std::string sparse_matrixA1_path = "./A1.mtx";
    exprotSparsematrix(A1, sparse_matrixA1_path);
    const std::string sparse_matrixA2_path = "./A2.mtx";
    exprotSparsematrix(A2, sparse_matrixA2_path);
    const std::string sparse_matrixA3_path = "./A3.mtx";
    exprotSparsematrix(A3, sparse_matrixA3_path);

    // Export vector v and w
    const std::string vpath = "./v.mtx";
    exportVector(v, vpath);
    const std::string wpath = "./w.mtx";
    exportVector(w, wpath);

    // Free memory
    stbi_image_free(image_data);

    return 0;
}