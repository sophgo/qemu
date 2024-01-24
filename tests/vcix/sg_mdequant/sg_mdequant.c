#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#define RSIZE 8
#define CSIZE 6

extern void vcix_mdequant_asm(int len, int l2, int ol,uint64_t mtype, void *input_int, void *input_float, void *output);
void matrix_multiply_and_compare(_Float16 *standard_output, _Float16 *output);

uint64_t assemble(uint64_t mstride, uint64_t rsize, uint64_t csize) {
    uint64_t mtype = 0;
    mtype |= ((mstride & 0xFFFFFFFF) << 32);
    mtype |= ((rsize & 0xFFFF) << 8);
    mtype |= ((csize) & 0xFF);
    return mtype;
}

int main(int argc, char **argv) {
    const char *input_file_path = argv[1];
    int amod = atoi(argv[2]);
    // assign memory
    int8_t *input_int = (int8_t *)malloc(sizeof(int8_t) * RSIZE * CSIZE);
    _Float16 *input_float = (_Float16 *)malloc(sizeof(_Float16) * RSIZE);
    _Float16 *standard_output = (_Float16 *)malloc(sizeof(_Float16) * RSIZE * 2 * CSIZE);

    // read experiment data from file
    FILE *input_file = fopen(input_file_path, "rb");
    fread(input_int, sizeof(int8_t), RSIZE * CSIZE, input_file);
    fread(input_float, sizeof(_Float16), RSIZE * 2, input_file);
    fread(standard_output, sizeof(_Float16), RSIZE * CSIZE * 2, input_file);
    fclose(input_file);

    _Float16 *o_c = (_Float16 *)malloc(sizeof(_Float16) * RSIZE * CSIZE * 2);
    vcix_mdequant_asm(RSIZE * CSIZE, RSIZE * 2, RSIZE * CSIZE * 2,assemble(0, RSIZE, CSIZE), input_int, input_float, o_c);

    matrix_multiply_and_compare(standard_output, o_c);

    free(input_int);
    free(input_float);
    free(standard_output);
    free(o_c);

    return 0;
}

void matrix_multiply_and_compare(_Float16 *standard_output, _Float16 *output) {
    for(int i = 0; i < 2 * CSIZE * RSIZE; ++i) {
            if(fabsf(standard_output[i] - output[i]) != 0) {
                printf("TEST FAILED in location (%d), output is %f but should be %f!!\n", i, (float)output[i], (float)standard_output[i]);
                for(int i = 0; i < CSIZE * RSIZE * 2; ++i) {
                        printf("%f ", (float)output[i]);
                }
                printf("\n");
                return;
            }
    }
    printf("Test passed.\n");
}
