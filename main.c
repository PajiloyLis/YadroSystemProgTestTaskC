#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef enum {
    PROG_ARGS_ERROR = 10,
    FILE_OPEN_ERROR = 11,
    FILE_READ_ERROR = 12,
    MEMORY_ALLOCATION_ERROR = 13,
    FUNC_ARGS_ERROR = 14,
    FILE_WRITE_ERROR = 15,
} Errors;

typedef enum {
    SIMPLE_MATRIX_SIZE_BYTES = 4,
    KERNEL_MATRIX_SIZE_BYTES = 2,
} MatrixSizeBytes;

typedef enum {
    SIMPLE_MATRIX_ELEM_BYTES = 1,
    KERNEL_MATRIX_ELEM_BYTES = 1,
} MatrixElementBytes;

typedef enum {
    MATRIX_OFFSET_SIMPLE = 2,
    MATRIX_OFFSET_KERNEL = 0,
} MatrixFileOffset;

typedef enum {
    SIMPLE,
    KERNEL,
} MatrixType;

typedef struct {
    int32_t **data;
    uint32_t rows, cols;
    MatrixType type;
} Matrix;

typedef enum {
    SIGNED_MODULE = 241,
    UNSIGNED_MODULE = 251,
} Modules;

int take_args_files(int argc, char **argv, char **file, const char *file_key);

int read_input_matrix(const char *filename, Matrix *matrix, long start_offset,
                      long first_elem_offset);

int create_matrx(Matrix **matrix, MatrixType type);

int create_matrix_with_sizes(Matrix **matrix, uint32_t rows, uint32_t cols, MatrixType type);

int matrix_convolution(const Matrix *matrix, const Matrix *kernel, Matrix *res);


int write_output_matrix(const char *filename, Matrix *matrix, long elems_offset, long first_elem_offset);

int main(int argc, char **argv) {
    int rc = EXIT_SUCCESS;
    char *input_file = NULL, *output_file = NULL;
    rc = take_args_files(argc, argv, &input_file, "-i");
    rc = rc == EXIT_SUCCESS ? take_args_files(argc, argv, &output_file, "-o") : rc;
    Matrix *matrixA = NULL, *matrixB = NULL, *matrixC = NULL, *matrixD = NULL;
    rc = rc == EXIT_SUCCESS ? create_matrx(&matrixA, SIMPLE) : rc;
    rc = rc == EXIT_SUCCESS ? create_matrx(&matrixB, SIMPLE) : rc;
    rc = rc == EXIT_SUCCESS ? create_matrx(&matrixC, SIMPLE) : rc;
    rc = rc == EXIT_SUCCESS ? create_matrx(&matrixD, KERNEL) : rc;
    rc = rc == EXIT_SUCCESS ? read_input_matrix(input_file, matrixA, 0, 0) : rc;
    rc = rc == EXIT_SUCCESS ? read_input_matrix(input_file, matrixB, 0, 1) : rc;
    rc = rc == EXIT_SUCCESS ? read_input_matrix(input_file, matrixC, 0, 2) : rc;
    rc = rc == EXIT_SUCCESS
             ? read_input_matrix(input_file, matrixD,
                                 matrixA->cols * matrixA->rows * 3 * SIMPLE_MATRIX_ELEM_BYTES
                                 + SIMPLE_MATRIX_SIZE_BYTES * 2,
                                 0)
             : rc;
    Matrix *matrixARes = NULL, *matrixBRes = NULL, *matrixCRes = NULL;
    rc = rc == EXIT_SUCCESS
             ? create_matrix_with_sizes(&matrixARes, matrixA->rows, matrixA->cols, SIMPLE)
             : rc;
    rc = rc == EXIT_SUCCESS
             ? create_matrix_with_sizes(&matrixBRes, matrixB->rows, matrixB->cols, SIMPLE)
             : rc;
    rc = rc == EXIT_SUCCESS
             ? create_matrix_with_sizes(&matrixCRes, matrixC->rows, matrixC->cols, SIMPLE)
             : rc;
    rc = rc == EXIT_SUCCESS ? matrix_convolution(matrixA, matrixD, matrixARes) : rc;
    rc = rc == EXIT_SUCCESS ? matrix_convolution(matrixB, matrixD, matrixBRes) : rc;
    rc = rc == EXIT_SUCCESS ? matrix_convolution(matrixC, matrixD, matrixCRes) : rc;
    rc = rc == EXIT_SUCCESS ? write_output_matrix(output_file, matrixARes, MATRIX_OFFSET_SIMPLE, 0) : rc;
    rc = rc == EXIT_SUCCESS ? write_output_matrix(output_file, matrixBRes, MATRIX_OFFSET_SIMPLE, 1) : rc;
    rc = rc == EXIT_SUCCESS ? write_output_matrix(output_file, matrixCRes, MATRIX_OFFSET_SIMPLE, 2) : rc;
    return rc;
}

int take_args_files(const int argc, char **argv, char **file, const char *file_key) {
    if (argc != 5)
        return PROG_ARGS_ERROR;
    int rc = EXIT_SUCCESS;
    for (size_t i = 0; i < (size_t) (argc - 1); ++i) {
        if (strcmp(argv[i], file_key) == 0)
            *file = argv[i + 1];
    }
    if (*file == NULL)
        rc = PROG_ARGS_ERROR;
    return rc;
}

int read_input_matrix(const char *filename, Matrix *matrix, long start_offset,
                      long first_elem_offset) {
    int rc = EXIT_SUCCESS;
    FILE *f = fopen(filename, "rb");
    if (f == NULL)
        rc = FILE_OPEN_ERROR;
    else {
        size_t n;
        n = fseek(f, start_offset, SEEK_SET);
        rc = n ? FILE_READ_ERROR : rc;
        if (rc == EXIT_SUCCESS) {
            if (matrix->type == SIMPLE) {
                n = fread(&matrix->rows, SIMPLE_MATRIX_SIZE_BYTES, 1, f);
                rc = (n != 1 ? FILE_READ_ERROR : rc);
                if (rc == EXIT_SUCCESS) {
                    n = fread(&matrix->cols, SIMPLE_MATRIX_SIZE_BYTES, 1, f);
                    rc = n != 1 ? FILE_READ_ERROR : rc;
                }
            } else if (matrix->type == KERNEL) {
                uint16_t buf;
                n = fread(&buf, KERNEL_MATRIX_SIZE_BYTES, 1, f);
                rc = (n != 1 ? FILE_READ_ERROR : rc);
                matrix->rows = rc == EXIT_SUCCESS ? buf : matrix->rows;
                if (rc == EXIT_SUCCESS) {
                    n = fread(&buf, KERNEL_MATRIX_SIZE_BYTES, 1, f);
                    rc = (n != 1 ? FILE_READ_ERROR : rc);
                    matrix->cols = rc == EXIT_SUCCESS ? buf : matrix->cols;
                }
            }
        }
        matrix->data = calloc(matrix->rows, sizeof(*matrix->data));
        if (matrix->data == NULL)
            rc = MEMORY_ALLOCATION_ERROR;
        n = fseek(f, first_elem_offset, SEEK_CUR);
        rc = n ? FILE_READ_ERROR : rc;
        size_t i = 0;
        for (; i < matrix->rows && rc == EXIT_SUCCESS; ++i) {
            matrix->data[i] = malloc(sizeof(**matrix->data) * matrix->cols);
            if (matrix->data[i] == NULL) {
                rc = MEMORY_ALLOCATION_ERROR;
            }
            for (size_t j = 0; j < matrix->cols && rc == EXIT_SUCCESS; ++j) {
                if (rc == EXIT_SUCCESS) {
                    if (matrix->type == SIMPLE) {
                        uint8_t buf;
                        n = fread(&buf, SIMPLE_MATRIX_ELEM_BYTES, 1, f);
                        rc = (n != 1 ? FILE_READ_ERROR : rc);
                        matrix->data[i][j] = rc == EXIT_SUCCESS ? buf : matrix->data[i][j];
                    } else if (matrix->type == KERNEL) {
                        int8_t buf;
                        n = fread(&buf, KERNEL_MATRIX_ELEM_BYTES, 1, f);
                        rc = n != 1 ? FILE_READ_ERROR : rc;
                        matrix->data[i][j] = rc == EXIT_SUCCESS ? buf : matrix->data[i][j];
                    }
                }
                if (i != matrix->rows - 1 && j != matrix->cols - 1) {
                    long offset;
                    if (matrix->type == SIMPLE) {
                        offset = MATRIX_OFFSET_SIMPLE;
                    } else if (matrix->type == KERNEL) {
                        offset = MATRIX_OFFSET_KERNEL;
                    }
                    n = fseek(f, offset, SEEK_CUR);
                    rc = (n ? FILE_READ_ERROR : rc);
                }
            }
        }
        fclose(f);
        if (rc == MEMORY_ALLOCATION_ERROR && i > 0) {
            for (size_t j = 0; j < i; ++j)
                free(matrix->data[i]);
            free(matrix->data);
        }
    }
    return rc;
}

int create_matrx(Matrix **matrix, MatrixType type) {
    int rc = EXIT_SUCCESS;
    *matrix = malloc(sizeof(**matrix));
    if (*matrix == NULL)
        rc = MEMORY_ALLOCATION_ERROR;
    else {
        (*matrix)->type = type;
        (*matrix)->data = NULL;
        (*matrix)->rows = 0;
        (*matrix)->cols = 0;
    }
    return rc;
}


int create_matrix_with_sizes(Matrix **matrix, uint32_t rows, uint32_t cols, MatrixType type) {
    int rc = EXIT_SUCCESS;
    *matrix = malloc(sizeof(**matrix));
    if (*matrix == NULL)
        rc = MEMORY_ALLOCATION_ERROR;
    else {
        (*matrix)->type = type;
        (*matrix)->rows = rows;
        (*matrix)->cols = cols;
        (*matrix)->data = calloc((*matrix)->rows, sizeof(*(*matrix)->data));
        if ((*matrix)->data == NULL)
            rc = MEMORY_ALLOCATION_ERROR;
        size_t i = 0;
        for (; i < (*matrix)->rows && rc == EXIT_SUCCESS; ++i) {
            (*matrix)->data[i] = calloc((*matrix)->cols, sizeof(**(*matrix)->data));
            if ((*matrix)->data[i] == NULL) {
                rc = MEMORY_ALLOCATION_ERROR;
            }
        }
        if (rc == MEMORY_ALLOCATION_ERROR && i > 0) {
            for (size_t j = 0; j < i; ++j)
                free((*matrix)->data[i]);
            free((*matrix)->data);
        }
    }
    return rc;
}

int matrix_convolution(const Matrix *matrix, const Matrix *kernel, Matrix *res) {
    if (matrix == NULL || kernel == NULL || res == NULL)
        return FUNC_ARGS_ERROR;
    for (size_t i = 0; i < matrix->rows; ++i) {
        for (size_t j = 0; j < matrix->cols; ++j) {
            res->data[i][j] = 0;
            const int up_i = (int) i - (int) kernel->rows / 2,
                    down_i = (int) i + (int) kernel->rows / 2,
                    left_j = (int) j - (int) kernel->cols / 2,
                    right_j = (int) j + (int) kernel->cols / 2;
            for (size_t k = (0 > up_i ? 0 : up_i); k < ((int) matrix->rows < down_i ? matrix->rows : down_i); ++k) {
                for (size_t l = (0 > left_j ? 0 : left_j); l < ((int) matrix->cols < right_j ? matrix->cols : right_j);
                     ++l) {
                    int32_t add_val = matrix->data[k][l] * kernel->data[k][l];
                    if (add_val > UINT8_MAX)
                        add_val %= UNSIGNED_MODULE;
                    else if (add_val < 0)
                        add_val = -add_val % SIGNED_MODULE;
                    res->data[k][l] += add_val;
                    if (res->data[k][l] > UINT8_MAX)
                        res->data[k][l] %= UNSIGNED_MODULE;
                }
            }
        }
    }
    return EXIT_SUCCESS;
}

int write_output_matrix(const char *filename, Matrix *matrix, long elems_offset, long first_elem_offset) {
    int rc = EXIT_SUCCESS;
    FILE *f = fopen(filename, "wb");
    if (f == NULL)
        rc = FILE_OPEN_ERROR;
    else {
        size_t n;
        if (matrix->type == SIMPLE) {
            n = fwrite(&matrix->rows, SIMPLE_MATRIX_SIZE_BYTES, 1, f);
            rc = n != 1 ? FILE_WRITE_ERROR : rc;
            if (rc == EXIT_SUCCESS) {
                n = fwrite(&matrix->cols, SIMPLE_MATRIX_SIZE_BYTES, 1, f);
                rc = n != 1 ? FILE_WRITE_ERROR : rc;
            }
        } else if (matrix->type == KERNEL) {
            uint16_t buf = matrix->rows;
            n = fwrite(&buf, KERNEL_MATRIX_SIZE_BYTES, 1, f);
            rc = n != 1 ? FILE_WRITE_ERROR : rc;
            if (rc == EXIT_SUCCESS) {
                buf = matrix->cols;
                n = fwrite(&buf, KERNEL_MATRIX_SIZE_BYTES, 1, f);
                rc = n != 1 ? FILE_WRITE_ERROR : rc;
            }
        }
        if (rc == EXIT_SUCCESS) {
            for (size_t i = 0; i < matrix->rows && rc == EXIT_SUCCESS; ++i) {
                for (size_t j = 0; j < matrix->cols && rc == EXIT_SUCCESS; ++j) {
                    n = fseek(f, first_elem_offset, SEEK_CUR);
                    rc = n != 1 ? FILE_WRITE_ERROR : rc;
                    if (matrix->type == SIMPLE) {
                        uint8_t buf = matrix->data[i][j];
                        n = fwrite(&buf, SIMPLE_MATRIX_ELEM_BYTES, 1, f);
                        rc = n != 1 ? FILE_WRITE_ERROR : rc;
                    } else if (matrix->type == KERNEL) {
                        int16_t buf = (int16_t) matrix->data[i][j];
                        n = fwrite(&buf, KERNEL_MATRIX_ELEM_BYTES, 1, f);
                        rc = n != 1 ? FILE_WRITE_ERROR : rc;
                    }
                    if (i != matrix->rows - 1 && j != matrix->cols - 1) {
                        n = fseek(f, elems_offset, SEEK_CUR);
                        rc = n != 1 ? FILE_WRITE_ERROR : rc;
                    }
                }
            }
        }
    }
    return rc;
}
