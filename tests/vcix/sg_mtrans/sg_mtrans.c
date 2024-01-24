#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#define R_SIZE 8
#define C_SIZE 6

extern void vcix_mtrans_asm(uint64_t mtype, int size, void *input, void *output);
void matrix_multiply_and_compare(int32_t *out_mul_ref, int32_t *out_add_ref);

int main(int argc, char **argv) {
    const char *input_file_path = argv[1];
    char *endptr;
    int64_t mtype = strtoll(argv[2], &endptr, 10);
    // assign memory
    int32_t *input = (int32_t *)malloc(sizeof(int32_t) * C_SIZE * R_SIZE);
    int32_t *output = (int32_t *)malloc(sizeof(int32_t) * C_SIZE * R_SIZE);
    int32_t *out_ref = (int32_t *)malloc(sizeof(int32_t) * C_SIZE * R_SIZE);
    // read experiment data from file
    FILE *input_file = fopen(input_file_path, "rb");
    fread(input, sizeof(int32_t), C_SIZE * R_SIZE, input_file);
    fread(output, sizeof(int32_t), C_SIZE * R_SIZE, input_file);
    fclose(input_file);

    vcix_mtrans_asm(mtype, R_SIZE * C_SIZE, input, out_ref);
    matrix_multiply_and_compare(output, out_ref);

    free(input);
    free(out_ref);
    free(output);

    return 0;
}

void matrix_multiply_and_compare(int32_t *output_add, int32_t *out_add_ref) {
    for(int i = 0; i < C_SIZE; ++i) {
        for(int j = 0; j < R_SIZE; ++j) {
            if(output_add[i * R_SIZE + j] != out_add_ref[i * R_SIZE + j]) {
                printf("TEST FAILED in location (%d, %d), output is %d but should be %d!!\n", i, j, (int)out_add_ref[i * R_SIZE + j], (int)output_add[i * R_SIZE + j]);
                for(int i = 0; i < R_SIZE * C_SIZE; ++i) {
                    printf("%d ", (int)out_add_ref[i]);
                }
                printf("\n");
                return;
            }
        }
    }
    printf("Test passed.\n");
}
