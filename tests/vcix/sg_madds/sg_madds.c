#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#define R_SIZE 8
#define C_SIZE 6

extern void vcix_madds_asm(uint64_t mtype, int size, void *input1, void *input2, void *output_add);
void matrix_multiply_and_compare(int8_t *out_mul_ref, int8_t *out_add_ref);

int main(int argc, char **argv) {
    const char *input_file_path = argv[1];
    char *endptr;
    int64_t mtype = strtoll(argv[2], &endptr, 10);
    // assign memory
    int8_t *input1 = (int8_t *)malloc(sizeof(int8_t) * C_SIZE * R_SIZE);
    int8_t *input2 = (int8_t *)malloc(sizeof(int8_t) * C_SIZE * R_SIZE);
    int8_t *output_add = (int8_t *)malloc(sizeof(int8_t) * C_SIZE * R_SIZE);
    int8_t *out_add_ref = (int8_t *)malloc(sizeof(int8_t) * C_SIZE * R_SIZE);
    // read experiment data from file
    FILE *input_file = fopen(input_file_path, "rb");
    fread(input1, sizeof(int8_t), C_SIZE * R_SIZE, input_file);
    fread(input2, sizeof(int8_t), C_SIZE * R_SIZE, input_file);
    fread(output_add, sizeof(int8_t), C_SIZE * R_SIZE, input_file);
    fclose(input_file);

    vcix_madds_asm(mtype, R_SIZE * C_SIZE, input1, input2, out_add_ref);
    matrix_multiply_and_compare(output_add, out_add_ref);

    free(input1);
    free(input2);
    free(output_add);
    free(out_add_ref);

    return 0;
}

void matrix_multiply_and_compare(int8_t *output_add, int8_t *out_add_ref) {
    for(int i = 0; i < R_SIZE * C_SIZE; ++i) {
            if(output_add[i] != out_add_ref[i]) {
                printf("===============ADD================\n");
                printf("TEST FAILED in location (%d), output is %d but should be %d!!\n", i, (int)out_add_ref[i], (int)output_add[i]);
                for(int i = 0; i < R_SIZE * C_SIZE; ++i) {
                    printf("%d ", (int)out_add_ref[i]);
                }
                printf("\n");
                printf("==================================\n");
                return;
            }
    }
    printf("Test passed.\n");
}
