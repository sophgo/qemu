#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define MATRIX_SIZE 128

extern void vcix_mmaf_asm(int m, int n, int k, void *a, void *b, void *c);
void matrix_multiply_and_compare(_Float16 *a, _Float16 *b, _Float16 *c);

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: %s <input_file> <output_file>\n", argv[0]);
        return 1;
    }

    const char *input_file_path = argv[1];
    const char *output_file_path = argv[2];

    // assign memory
    _Float16 *a = (_Float16 *)malloc(sizeof(_Float16) * MATRIX_SIZE);
    _Float16 *b = (_Float16 *)malloc(sizeof(_Float16) * MATRIX_SIZE);
    _Float16 *c = (_Float16 *)malloc(sizeof(_Float16) * MATRIX_SIZE * 2);

    // read experiment data from file
    FILE *input_file = fopen(input_file_path, "rb");
    fread(a, sizeof(_Float16), MATRIX_SIZE, input_file);
    fread(b, sizeof(_Float16), MATRIX_SIZE, input_file);
    fread(c, sizeof(_Float16), MATRIX_SIZE * 2, input_file);
    fclose(input_file);
    matrix_multiply_and_compare(a, b, c);

    free(a);
    free(b);
    free(c);

    return 0;
}

void matrix_multiply_and_compare(_Float16 *a, _Float16 *b, _Float16 *c) {
    int m = 16;
    int n = 16;
    int k = 8;
    _Float16 *o_c = (_Float16 *)malloc(sizeof(_Float16) * MATRIX_SIZE * 2);
    vcix_mmaf_asm(m, n, k, a, b, o_c);
    for(int i = 0; i < m; ++i) {
        for(int j = 0; j < n; ++j) {
            if(fabsf((float)(o_c[i * n + j] - c[i * n + j])) > 0.005) {
                printf("TEST FAILED in location (%d, %d), output is %f but should be %f!!\n", i, j,(float)o_c[i * n + j], (float)c[i * n + j]);
                for(int i = 0; i < m; ++i) {
                    for(int j = 0; j < n; ++j) {
                        printf("%f ", (float)o_c[i * n + j]);
                    }
                    printf("\n");
                }
                free(o_c);
                return;
            }
        }
    }
    free(o_c);
    printf("Test passed.\n");
}
