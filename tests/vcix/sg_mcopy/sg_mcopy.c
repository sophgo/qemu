#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MATRIX_SIZE 30

extern void vcix_mcopy_asm(uint64_t mtype, void *input, void* output);
void matrix_multiply_and_compare(int8_t *origin_matrix, int8_t *output_matrix);

int main(int argc, char **argv) {

    const char *input_file_path = argv[1];
    char *endptr;
    int64_t mtype = strtoll(argv[2], &endptr, 10);

    // assign memory
    int8_t *origin_matrix = (int8_t *)malloc(sizeof(int8_t) * MATRIX_SIZE);
    int8_t *output_matrix = (int8_t *)malloc(sizeof(int8_t) * MATRIX_SIZE);

    // read experiment data from file
    FILE *input_file = fopen(input_file_path, "rb");
    fread(origin_matrix, sizeof(int8_t), MATRIX_SIZE, input_file);
    fclose(input_file);
    vcix_mcopy_asm(mtype, origin_matrix, output_matrix);
    matrix_multiply_and_compare(origin_matrix, output_matrix);

    free(origin_matrix);
    free(output_matrix);

    return 0;
}

void matrix_multiply_and_compare(int8_t *origin_matrix, int8_t *output_matrix) {
    for(int i = 0; i < MATRIX_SIZE; ++i) {
            if(origin_matrix[i] - output_matrix[i ] != 0) {
                printf("TEST FAILED in location (%d), output is %d but should be %d!!\n", i, output_matrix[i], origin_matrix[i]);
                for(int i = 0; i < MATRIX_SIZE; ++i) {
                        printf("%d ", output_matrix[i]);
                }
                printf("\n");
                return;
            }
    }
    printf("Test passed.\n");
}