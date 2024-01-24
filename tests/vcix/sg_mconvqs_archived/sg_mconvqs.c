#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

extern void vcix_mconvqs_asm(int ih, int iw, int oh, int ow, uint64_t conv, void *input, void *filter, void* output);
void matrix_multiply_and_compare(int32_t *c, int32_t *o_c, int m, int n);

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: %s <input_file> <output_file>\n", argv[0]);
        return 1;
    }

    const char *input_file_path = argv[1];
    const char *output_file_path = argv[2];
    char *endptr;
    int64_t conv = strtoll(argv[3], &endptr, 10);

    uint64_t mask4 = 0xF;  // 4-bit mask
    uint64_t mask8 = 0xFF; // 8-bit mask

    int ih = (conv >> 24) & mask8;
    int iw = (conv >> 16) & mask8;
    int oh = (conv >> 8) & mask8;
    int ow = conv & mask8;
    int kh = (conv >> 60) & mask4;
    int kw = (conv >> 56) & mask4;
    // assign memory
    int8_t *input_matrix = (int8_t *)malloc(sizeof(int8_t) * (int)ih * (int)iw);
    int8_t *kernel = (int8_t *)malloc(sizeof(int8_t) * (int)kh * (int)kw);
    int32_t *output_matrix = (int32_t *)malloc(sizeof(int32_t) * (int)ow * (int)oh);

    // read experiment data from file
    FILE *input_file = fopen(input_file_path, "rb");
    fread(input_matrix, sizeof(int8_t), (int)ih * (int)iw, input_file);
    fread(kernel, sizeof(int8_t), (int)kh * (int)kw, input_file);
    fread(output_matrix, sizeof(int32_t), (int)ow * (int)oh, input_file);
    fclose(input_file);

    int32_t *o_c = (int32_t *)malloc(sizeof(int32_t) * (int)ow * (int)oh);
    
    printf("ih: %d, iw: %d, kh: %d, kw: %d, oh: %d, ow: %d\n", ih, iw, kh, kw, oh, ow);
    vcix_mconvqs_asm(ih, iw, oh, ow, conv, input_matrix, kernel, o_c);
    matrix_multiply_and_compare(output_matrix, o_c, oh, ow);

    free(input_matrix);
    free(kernel);
    free(output_matrix);
    free(o_c);

    return 0;
}

void matrix_multiply_and_compare(int32_t *c, int32_t *o_c, int m, int n) {
    for(int i = 0; i < m; ++i) {
        for(int j = 0; j < n; ++j) {
            if(o_c[i * n + j] - c[i * n + j] != 0) {
                printf("TEST FAILED in location (%d, %d), output is %d but should be %d!!\n", i, j, o_c[i * n + j], c[i * n + j]);
                for(int i = 0; i < m; ++i) {
                    for(int j = 0; j < n; ++j) {
                        printf("%d ", o_c[i * n + j]);
                    }
                    printf("\n");
                }
                return;
            }
        }
    }
    printf("Test passed.\n");
}
