#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#define MATRIX_SIZE 64

extern void vcix_mcal_asm(uint64_t mtype,int size, void *input1, void *input2, void *output_mul, void *output_add);
void matrix_multiply_and_compare(_Float16 *out_mul_ref, _Float16 *out_add_ref, _Float16 *output_mul, _Float16 *output_add);

int main(int argc, char **argv) {
    const char *input_file_path = argv[1];
    char *endptr;
    int64_t mtype = strtoll(argv[2], &endptr, 10);
    // assign memory
    _Float16 *input1 = (_Float16 *)malloc(sizeof(_Float16) * MATRIX_SIZE);
    _Float16 *input2 = (_Float16 *)malloc(sizeof(_Float16) * MATRIX_SIZE);
    _Float16 *output_mul = (_Float16 *)malloc(sizeof(_Float16) * MATRIX_SIZE);
    _Float16 *output_add = (_Float16 *)malloc(sizeof(_Float16) * MATRIX_SIZE);

    _Float16 *out_mul_ref = (_Float16 *)malloc(sizeof(_Float16) * MATRIX_SIZE);
    _Float16 *out_add_ref = (_Float16 *)malloc(sizeof(_Float16) * MATRIX_SIZE);
    // read experiment data from file
    FILE *input_file = fopen(input_file_path, "rb");
    fread(input1, sizeof(_Float16), MATRIX_SIZE, input_file);
    fread(input2, sizeof(_Float16), MATRIX_SIZE, input_file);
    fread(output_mul, sizeof(_Float16), MATRIX_SIZE, input_file);
    fread(output_add, sizeof(_Float16), MATRIX_SIZE, input_file);
    fclose(input_file);

    vcix_mcal_asm(mtype,MATRIX_SIZE, input1, input2, out_mul_ref, out_add_ref);
    matrix_multiply_and_compare(out_mul_ref, out_add_ref, output_mul, output_add);

    free(input1);
    free(input2);
    free(output_mul);
    free(output_add);
    free(out_mul_ref);
    free(out_add_ref);

    return 0;
}

void matrix_multiply_and_compare(_Float16 *out_mul_ref, _Float16 *out_add_ref, _Float16 *output_mul, _Float16 *output_add) {
    for(int i = 0; i < MATRIX_SIZE; ++i) {
            if(output_add[i] != out_add_ref[i]) {
                printf("===============ADD================\n");
                printf("TEST FAILED in location (%d), output is %f but should be %f!!\n", i, (float)out_add_ref[i], (float)output_add[i]);
                for(int i = 0; i < MATRIX_SIZE; ++i) {
                    printf("%f ", (float)out_add_ref[i]);
                }
                printf("\n"); 
                printf("==================================\n"); 
                return;
            }
    }
    for(int i = 0; i < MATRIX_SIZE; ++i) {
            if(output_mul[i] != out_mul_ref[i]) {
                printf("===============MUL================\n");
                printf("TEST FAILED in location (%d), output is %f but should be %f!!\n", i, (float)out_mul_ref[i], (float)output_mul[i]);
                for(int i = 0; i < MATRIX_SIZE; ++i) {
                    printf("%f ", (float)out_mul_ref[i]);
                }
                printf("\n"); 
                printf("==================================\n"); 
                return;
            }
    }
    
    printf("Test passed.\n");
}
