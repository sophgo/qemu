#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#define MATRIX_SIZE 48
#define M 6
#define N 8

extern void vcix_mldt_asm(int m, int n, int size, void *input, void *output);
void matrix_multiply_and_compare(int8_t *input, int8_t *output);

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: %s <input_file> <output_file>\n", argv[0]);
        return 1;
    }

    const char *input_file_path = argv[1];
    const char *output_file_path = argv[2];
    
    // assign memory
    int8_t *input_matrix = (int8_t *)malloc(sizeof(int8_t) * MATRIX_SIZE);
    int8_t *output_matrix = (int8_t *)malloc(sizeof(int8_t) * MATRIX_SIZE);
    
    // read experiment data from file
    FILE *input_file = fopen(input_file_path, "rb");
    fread(input_matrix, sizeof(int8_t), MATRIX_SIZE, input_file);
    fclose(input_file);

    vcix_mldt_asm(M, N, MATRIX_SIZE, input_matrix, output_matrix);
    matrix_multiply_and_compare(input_matrix, output_matrix);

    free(input_matrix);
    free(output_matrix);

    return 0;
}

void matrix_multiply_and_compare(int8_t *input, int8_t *output) {
    for(int i = 0; i < M; ++i) {
        for(int j = 0; j < N; ++j) {
            if(output[j * M + i] != input[i * N + j]) {
                printf("TEST FAILED in location (%d, %d), output is %d but should be %d!!\n", i, j, output[j * N + i], input[i * N + j]);
                for(int i = 0; i < N; ++i) {
                    for(int j = 0; j < M; ++j){
                        printf("%d ", output[i * M + j]);
                    }
                    printf("\n");
                }
                return;
            }
        }
    }
    printf("Test passed.\n");
}
