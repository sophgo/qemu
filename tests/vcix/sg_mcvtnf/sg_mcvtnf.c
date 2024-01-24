#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#define MATRIX_SIZE 64

extern void vcix_mcvtnf_asm(uint64_t mtype, int size, void *input, void *output);
void matrix_multiply_and_compare(_Float16 *output_ref, _Float16 *output);

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: %s <input_file> <output_file>\n", argv[0]);
        return 1;
    }

    const char *input_file_path = argv[1];
    const char *output_file_path = argv[2];
    uint64_t mtype = 34359740424;
    // assign memory
    float *input_matrix = (float *)malloc(sizeof(float) * MATRIX_SIZE);
    _Float16 *output_matrix = (_Float16 *)malloc(sizeof(_Float16) * MATRIX_SIZE);
    _Float16 *output_matrix_ref = (_Float16 *)malloc(sizeof(_Float16) * MATRIX_SIZE);

    // read experiment data from file
    FILE *input_file = fopen(input_file_path, "rb");
    fread(input_matrix, sizeof(float), MATRIX_SIZE, input_file);
    fread(output_matrix, sizeof(_Float16), MATRIX_SIZE, input_file);
    fclose(input_file);

    vcix_mcvtnf_asm(mtype, MATRIX_SIZE, input_matrix, output_matrix_ref);
    matrix_multiply_and_compare(output_matrix_ref, output_matrix);

    free(input_matrix);
    free(output_matrix);
    free(output_matrix_ref);

    return 0;
}

void matrix_multiply_and_compare(_Float16 *output_ref, _Float16 *output) {
    for(int i = 0; i < MATRIX_SIZE; ++i) {
            if(output[i] != output_ref[i]) {
                printf("TEST FAILED in location (%d), output is %f but should be %f!!\n", i, (float)output_ref[i], (float)output[i]);
                for(int i = 0; i < MATRIX_SIZE; ++i) {
                    printf("%f ", (float)output_ref[i]);
                }
                printf("\n");  
                return;
            }
    }
    printf("Test passed.\n");
}
