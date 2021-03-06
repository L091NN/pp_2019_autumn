// Copyright 2019 Evdokimov Artem
#include <mpi.h>
#include <time.h>
#include <stdio.h>
#include <random>
#include <vector>
#include <algorithm>
#include "../../../modules/task_3/evdokimov_a_image_filtering/image_filtering.h"

std::vector <int> cernel() {
    std::vector <int> cernel = {
      -1, -1, -1,
      -1,  9, -1,
      -1, -1, -1
    };
    return cernel;
}

std::vector <int> transpose(std::vector<int> image, int rows, int columns) {
    if (rows <= 0 || columns <= 0) {
        throw "Error : count of columns and rows can't be negative or equals zero.";
    }
    std::vector<int> tempImg(rows * columns);
    int iterator = 0;
    for (auto colIterator = 0; colIterator < columns; colIterator++) {
        for (auto rowIterator = 0; rowIterator < rows; rowIterator++) {
            tempImg[iterator++] = image[columns * rowIterator + colIterator];
        }
    }
    return tempImg;
}

std::vector <int> generateImage(int rows, int columns) {
    if (rows <= 0 || columns <= 0) {
        throw "Error : count of columns and rows can't be negative or equals zero.";
    }
    std::mt19937 gen;
    gen.seed(static_cast<unsigned int>(time(0)));
    std::vector <int> image(rows * columns);
    for (auto i = 0; i < rows * columns; i++) {
        image[i] = static_cast<unsigned int>(gen() % 10);
    }
    return image;
}

std::vector <int> getTempImage(std::vector <int> image , int rows , int columns) {
    if (rows <= 0 || columns <= 0) {
        throw "Error : count of columns and rows can't be negative or equals zero.";
    }
    auto tmpRows = rows + 2;
    auto tmpColumns = columns + 2;
    std::vector <int> tmpImage(tmpRows * tmpColumns);
    // Filling tmpImage middle by image
    for (auto rowIterator = 1; rowIterator < rows + 1; rowIterator++) {
        for (auto colIterator = 1; colIterator < columns + 1; colIterator++) {
            tmpImage[rowIterator * tmpColumns + colIterator] = image[(rowIterator-1) * columns + (colIterator-1)];
        }
    }
    // Filling tmpImage corners
    tmpImage[0] = image[0];
    tmpImage[tmpColumns - 1] = image[columns-1];
    tmpImage[tmpRows * tmpColumns - tmpColumns] = image[rows * columns - columns];
    tmpImage[tmpRows * tmpColumns - 1] = image[rows * columns - 1];
    // Filling horizontal edges
    for (auto colIterator = 1; colIterator < columns + 1; colIterator++) {
        tmpImage[colIterator] = image[colIterator-1];
        tmpImage[tmpRows * tmpColumns - tmpColumns + colIterator] = image[rows * columns - columns - 1 + colIterator];
    }
    // Filling vertical edges
    for (auto rowIterator= 1; rowIterator < rows + 1; rowIterator++) {
        tmpImage[rowIterator * tmpColumns] = image[(rowIterator-1) * columns];
        tmpImage[(rowIterator + 1) * tmpColumns - 1] = image[rowIterator * columns - 1];
    }
  return tmpImage;
}

std::vector <int> imageFiltering(std::vector<int> tmpImage, std::vector<int> cernel, int rows, int columns) {
    if (rows <= 0 || columns <= 0) {
        throw "Error : count of columns and rows can't be negative or equals zero.";
    }
    std::vector <int> image(rows * columns);

    for (auto rowIterator = 1; rowIterator < rows + 1; rowIterator++) {
        for (auto colIterator = 1; colIterator < columns + 1; colIterator++) {
            auto result = 0, cernelIterator = 0;

            for (auto i = rowIterator - 1; i < rowIterator + 2; i++) {
                for (auto j = colIterator - 1; j < colIterator + 2; j++) {
                    result += tmpImage[i * (columns + 2) + j] * cernel[cernelIterator++];
                }
            }
            image[(rowIterator-1) * columns + (colIterator - 1)] = result;
        }
    }
    return image;
}

std::vector <int> imageFilteringMPI(std::vector <int> tmpImage, std::vector<int> cernel, int rows, int columns) {
    if (rows <= 0 || columns <= 0) {
        throw "Error : count of columns and rows can't be negative or equals zero.";
    }

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    const int delta = columns / size;
    const int rem = columns % size;
    int tmp = 0;
    for (auto i = 0; i < rank; i++) {
      tmp += delta + (i < rem ? 1 : 0);
    }

    int startCol = tmp + 1;
    int endCol = tmp + delta + (rank < rem ? 1 : 0) + 1;
    int elements_count = rows * (endCol - startCol);
    std::vector<int> image(elements_count);

    std::vector<int> sendcounts(size);
    std::vector<int> displs(size);

    sendcounts[rank] = elements_count;
    displs[rank] = rows * (startCol - 1);

    int tempDisls = rows * (startCol - 1);
    int tempSendCount = elements_count;

    MPI_Gather(&tempDisls, 1, MPI_INT, displs.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Gather(&tempSendCount, 1, MPI_INT, sendcounts.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);

    int imageIterator = 0;
    for (auto rowIterator = 1; rowIterator < rows + 1; rowIterator++) {
        for (auto colIterator = startCol ; colIterator < endCol ; colIterator++) {
            auto result = 0, cernelIterator = 0;
            for (auto i = rowIterator - 1; i < rowIterator + 2; i++) {
                for (auto j = colIterator - 1; j < colIterator + 2; j++) {
                    result += tmpImage[i * (columns + 2) + j] * cernel[cernelIterator++];
                }
            }
            image[imageIterator++] = result;
        }
    }


    if (endCol - startCol > 1) {
        image = transpose(image, rows, elements_count/rows);
    }

    std::vector<int> result(rows * columns);
    MPI_Gatherv(image.data(),
                elements_count, MPI_INT,
                result.data(),
                sendcounts.data(),
                displs.data(),
                MPI_INT,
                0,
                MPI_COMM_WORLD);
    if (rank == 0) {
        if (rows != columns) {
            result = transpose(result, columns, rows);
        } else {
            result = transpose(result, rows, columns);
        }
    }
    return result;
}
