#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#define RSIZE 8
#define CSIZE 6

extern void vcix_mquant_asm(int len, int l2, uint64_t mtype, void *input_int, void *input_float, void *output, int amod);
void matrix_multiply_and_compare(int8_t *standard_output, int8_t *output);

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
    int32_t *input_int = (int32_t *)malloc(sizeof(int32_t) * RSIZE * CSIZE);

    float *input_float;

    int8_t *standard_output = (int8_t *)malloc(sizeof(int8_t) * RSIZE * CSIZE);
    if(amod == 0) {
        input_float = (float *)malloc(sizeof(float) * RSIZE);
    } else {
        input_float = (float *)malloc(sizeof(float) * CSIZE);
    }
    // read experiment data from file
    FILE *input_file = fopen(input_file_path, "rb");
    fread(input_int, sizeof(int32_t), RSIZE * CSIZE, input_file);
    if(amod == 1) {
        fread(input_float, sizeof(float), RSIZE, input_file);
    } else {
        fread(input_float, sizeof(float), CSIZE, input_file);
    }
    fread(standard_output, sizeof(int8_t), RSIZE * CSIZE, input_file);
    fclose(input_file);

    int8_t *o_c = (int8_t *)malloc(sizeof(int8_t) * RSIZE * CSIZE);
    if(amod == 1) {
        vcix_mquant_asm(RSIZE * CSIZE, RSIZE, assemble(0, RSIZE, CSIZE), input_int, input_float, o_c, amod);
    } else {
        vcix_mquant_asm(RSIZE * CSIZE, CSIZE, assemble(0, RSIZE, CSIZE), input_int, input_float, o_c, amod);
    }
    matrix_multiply_and_compare(standard_output, o_c);

    free(input_int);
    free(input_float);
    free(standard_output);
    free(o_c);

    return 0;
}

void matrix_multiply_and_compare(int8_t *standard_output, int8_t *output) {
    for(int i = 0; i < CSIZE * RSIZE; ++i) {
            if(abs(standard_output[i] - output[i]) != 0) {
                printf("TEST FAILED in location (%d), output is %d but should be %d!!\n", i, (int)output[i], (int)standard_output[i]);
                for(int i = 0; i < CSIZE * RSIZE; ++i) {
                        printf("%d ", (int)output[i]);
                }
                printf("\n");
                return;
            }
    }
    printf("Test passed.\n");
}
