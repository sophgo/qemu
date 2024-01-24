#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

extern void vcix_mimg2col_asm(int in_len, int out_len, uint64_t conv, uint64_t conv_shape, void *input, void *output);
void matrix_multiply_and_compare(int8_t *standard_output, int8_t *output, int out_len);

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: %s <input_file> <output_file>\n", argv[0]);
        return 1;
    }

    const char *input_file_path = argv[1];
    char *endptr1;
    int64_t conv = strtoll(argv[2], &endptr1, 10);
    char *endptr2;
    int64_t conv_shape = strtoll(argv[3], &endptr2, 10);
    int ow = conv & 0xFF;
    int oh = (conv >> 8) & 0xFF;
    int iw = (conv >> 16) & 0xFF;
    int ih = (conv >> 24) & 0xFF;

    int kw = (conv_shape >> 16) & 0xF;
    int kh = (conv_shape >> 20) & 0xF;

    // assign memory
    int8_t *input_matrix = (int8_t *)malloc(sizeof(int8_t)  * iw * ih);
    int8_t *standard_output = (int8_t *)malloc(sizeof(int8_t) * oh * ow * kh * kw);

    // read experiment data from file
    FILE *input_file = fopen(input_file_path, "rb");
    fread(input_matrix, sizeof(int8_t), iw * ih, input_file);
    fread(standard_output, sizeof(int8_t), oh * ow * kh * kw, input_file);
    fclose(input_file);

    int8_t *o_c = (int8_t *)malloc(sizeof(int8_t) * oh * ow * kh * kw);
    vcix_mimg2col_asm(iw * ih, oh * ow * kh * kw , conv, conv_shape, input_matrix, o_c);
    matrix_multiply_and_compare(standard_output, o_c, oh * ow * kh * kw);

    free(input_matrix);
    //free(standard_output);
    free(o_c);

    return 0;
}

void matrix_multiply_and_compare(int8_t *standard_output, int8_t *output, int out_len) {
    for(int i = 0; i < out_len; ++i) {
            if(abs(standard_output[i] - output[i]) != 0) {
                printf("TEST FAILED in location (%d), output is %d but should be %d!!\n", i, (int)output[i], (int)standard_output[i]);
                for(int i = 0; i < out_len; ++i) {
                        printf("%d ", (int)output[i]); 
                }
                printf("\n");
                return;
            }
    }
    printf("Test passed.\n");
}
