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

int create_matrix(Matrix **matrix, MatrixType type);

int create_matrix_with_sizes(Matrix **matrix, uint32_t rows, uint32_t cols, MatrixType type);

int matrix_convolution(const Matrix *matrix, const Matrix *kernel, Matrix *res);

void destruct_matrix(Matrix *matrix);

int write_output_matrix(const char *filename, const Matrix *matrix, long elems_offset, long first_elem_offset);

int main(int argc, char **argv) {
    int rc = EXIT_SUCCESS;
    char *input_file = NULL, *output_file = NULL;
    rc = take_args_files(argc, argv, &input_file, "-i");
    rc = rc == EXIT_SUCCESS ? take_args_files(argc, argv, &output_file, "-o") : rc;
    Matrix *matrixA = NULL, *matrixB = NULL, *matrixC = NULL, *matrixD = NULL;
    rc = rc == EXIT_SUCCESS ? create_matrix(&matrixA, SIMPLE) : rc;
    rc = rc == EXIT_SUCCESS ? create_matrix(&matrixB, SIMPLE) : rc;
    rc = rc == EXIT_SUCCESS ? create_matrix(&matrixC, SIMPLE) : rc;
    rc = rc == EXIT_SUCCESS ? create_matrix(&matrixD, KERNEL) : rc;
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
    destruct_matrix(matrixA);
    destruct_matrix(matrixB);
    destruct_matrix(matrixC);
    destruct_matrix(matrixD);
    destruct_matrix(matrixARes);
    destruct_matrix(matrixBRes);
    destruct_matrix(matrixCRes);
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

    if (f == NULL) {
        rc = FILE_OPEN_ERROR;
    }

    if (rc == EXIT_SUCCESS) {
        if (fseek(f, start_offset, SEEK_SET) != 0)
            rc = FILE_READ_ERROR;
    }

    if (rc == EXIT_SUCCESS && matrix->type == SIMPLE) {
        if (fread(&matrix->rows, SIMPLE_MATRIX_SIZE_BYTES, 1, f) != 1 ||
            fread(&matrix->cols, SIMPLE_MATRIX_SIZE_BYTES, 1, f) != 1)
            rc = FILE_READ_ERROR;
    } else if (rc == EXIT_SUCCESS && matrix->type == KERNEL) {
        uint16_t buf = 0;
        if (fread(&buf, KERNEL_MATRIX_SIZE_BYTES, 1, f) != 1)
            rc = FILE_READ_ERROR;
        else
            matrix->rows = buf;
        if (rc == EXIT_SUCCESS) {
            if (fread(&buf, KERNEL_MATRIX_SIZE_BYTES, 1, f) != 1)
                rc = FILE_READ_ERROR;
            else
                matrix->cols = buf;
        }
    }

    if (rc == EXIT_SUCCESS) {
        matrix->data = calloc(matrix->rows, sizeof(*matrix->data));
        if (matrix->data == NULL)
            rc = MEMORY_ALLOCATION_ERROR;
    }

    if (rc == EXIT_SUCCESS) {
        if (fseek(f, first_elem_offset, SEEK_CUR) != 0)
            rc = FILE_READ_ERROR;
    }

    for (size_t i = 0; i < matrix->rows && rc == EXIT_SUCCESS; ++i) {
        matrix->data[i] = calloc(matrix->cols, sizeof(**matrix->data));
        if (matrix->data[i] == NULL)
            rc = MEMORY_ALLOCATION_ERROR;
    }

    for (size_t i = 0; i < matrix->rows && rc == EXIT_SUCCESS; ++i) {
        for (size_t j = 0; j < matrix->cols && rc == EXIT_SUCCESS; ++j) {
            if (matrix->type == SIMPLE) {
                uint8_t buf = 0;
                if (fread(&buf, 1, 1, f) != 1)
                    rc = FILE_READ_ERROR;
                else
                    matrix->data[i][j] = (int32_t) buf;
            } else {
                int8_t buf = 0;
                if (fread(&buf, 1, 1, f) != 1)
                    rc = FILE_READ_ERROR;
                else
                    matrix->data[i][j] = (int32_t) buf;
            }

            if (rc == EXIT_SUCCESS &&
                (i != matrix->rows - 1 || j != matrix->cols - 1)) {
                long offset = (matrix->type == SIMPLE)
                                  ? MATRIX_OFFSET_SIMPLE
                                  : MATRIX_OFFSET_KERNEL;
                if (fseek(f, offset, SEEK_CUR) != 0)
                    rc = FILE_READ_ERROR;
            }
        }
    }

    if (rc != EXIT_SUCCESS && matrix->data != NULL) {
        for (size_t i = 0; i < matrix->rows; ++i)
            free(matrix->data[i]);
        free(matrix->data);
        matrix->data = NULL;
    }
    if (f != NULL)
        fclose(f);
    return rc;
}

int create_matrix(Matrix **matrix, MatrixType type) {
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
    if (*matrix == NULL) {
        rc = MEMORY_ALLOCATION_ERROR;
    } else {
        (*matrix)->type = type;
        (*matrix)->rows = rows;
        (*matrix)->cols = cols;
        (*matrix)->data = calloc(rows, sizeof(*(*matrix)->data));

        if ((*matrix)->data == NULL) {
            rc = MEMORY_ALLOCATION_ERROR;
        }

        for (size_t i = 0; i < rows && rc == EXIT_SUCCESS; ++i) {
            (*matrix)->data[i] = calloc(cols, sizeof(**(*matrix)->data));
            if ((*matrix)->data[i] == NULL)
                rc = MEMORY_ALLOCATION_ERROR;
        }

        if (rc != EXIT_SUCCESS && (*matrix)->data != NULL) {
            for (size_t i = 0; i < rows; ++i)
                free((*matrix)->data[i]);
            free((*matrix)->data);
            (*matrix)->data = NULL;
        }
    }

    return rc;
}

int matrix_convolution(const Matrix *matrix, const Matrix *kernel, Matrix *res) {
    int rc = EXIT_SUCCESS;
    if (matrix == NULL || kernel == NULL || res == NULL) {
        rc = FUNC_ARGS_ERROR;
    } else if (res->rows != matrix->rows || res->cols != matrix->cols) {
        rc = FUNC_ARGS_ERROR;
    } else {
        const int kh = (int) kernel->rows;
        const int kw = (int) kernel->cols;
        const int ch = kh / 2;
        const int cw = kw / 2;
        const int mh = (int) matrix->rows;
        const int mw = (int) matrix->cols;

        for (int i = 0; i < mh; ++i) {
            for (int j = 0; j < mw; ++j) {
                int32_t sum = 0;
                for (int ki = 0; ki < kh; ++ki) {
                    int ii = i - ch + ki;
                    if (ii >= 0 && ii < mh) {
                        for (int kj = 0; kj < kw; ++kj) {
                            int jj = j - cw + kj;
                            if (jj >= 0 && jj < mw) {
                                sum += matrix->data[ii][jj] * kernel->data[ki][kj];
                            }
                        }
                    }
                }
                if (sum > 255)
                    sum %= UNSIGNED_MODULE;
                else if (sum < 0)
                    sum = (-sum) % SIGNED_MODULE;
                res->data[i][j] = sum;
            }
        }
    }
    return rc;
}

int write_output_matrix(const char *filename, const Matrix *matrix,
                        long elems_offset, long first_elem_offset) {
    int rc = EXIT_SUCCESS;
    FILE *f = fopen(filename, "r+b");

    if (f == NULL) {
        f = fopen(filename, "wb");
    }

    if (f == NULL) {
        rc = FILE_OPEN_ERROR;
    }

    if (rc == EXIT_SUCCESS) {
        if (fseek(f, 0, SEEK_SET) != 0 ||
            fwrite(&matrix->rows, SIMPLE_MATRIX_SIZE_BYTES, 1, f) != 1 ||
            fwrite(&matrix->cols, SIMPLE_MATRIX_SIZE_BYTES, 1, f) != 1)
            rc = FILE_WRITE_ERROR;
    }

    if (rc == EXIT_SUCCESS) {
        if (fseek(f, 2 * SIMPLE_MATRIX_SIZE_BYTES + first_elem_offset, SEEK_SET) != 0)
            rc = FILE_WRITE_ERROR;
    }

    for (size_t i = 0; i < matrix->rows && rc == EXIT_SUCCESS; ++i) {
        for (size_t j = 0; j < matrix->cols && rc == EXIT_SUCCESS; ++j) {
            uint8_t buf = (uint8_t) matrix->data[i][j];
            if (fwrite(&buf, 1, 1, f) != 1) {
                rc = FILE_WRITE_ERROR;
            } else if (i != matrix->rows - 1 || j != matrix->cols - 1) {
                if (fseek(f, elems_offset, SEEK_CUR) != 0)
                    rc = FILE_WRITE_ERROR;
            }
        }
    }

    if (f != NULL) {
        if (fclose(f) != 0 && rc == EXIT_SUCCESS)
            rc = FILE_WRITE_ERROR;
    }

    return rc;
}

void destruct_matrix(Matrix *matrix) {
    if (matrix == NULL) return;
    if (matrix->data != NULL) {
        for (size_t i = 0; i < matrix->rows; ++i) {
            free(matrix->data[i]);
        }
        free(matrix->data);
    }
    free(matrix);
}
