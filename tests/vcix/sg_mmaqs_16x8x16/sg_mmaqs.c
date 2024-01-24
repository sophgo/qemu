#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MATRIX_SIZE 128

extern void vcix_mma8_asm(int m, int n, int k, void *a, void *b, void *c);
void matrix_multiply_and_compare(int8_t *a, int8_t *b, int32_t *c);

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: %s <input_file> <output_file>\n", argv[0]);
        return 1;
    }

    const char *input_file_path = argv[1];
    const char *output_file_path = argv[2];

    // assign memory
    int8_t *a = (int8_t *)malloc(sizeof(int8_t) * MATRIX_SIZE);
    int8_t *b = (int8_t *)malloc(sizeof(int8_t) * MATRIX_SIZE);
    int32_t *c = (int32_t *)malloc(sizeof(int32_t) * MATRIX_SIZE * 2);

    // read experiment data from file
    FILE *input_file = fopen(input_file_path, "rb");
    fread(a, sizeof(int8_t), MATRIX_SIZE, input_file);
    fread(b, sizeof(int8_t), MATRIX_SIZE, input_file);
    fread(c, sizeof(int32_t), MATRIX_SIZE * 2, input_file);
    fclose(input_file);

    matrix_multiply_and_compare(a, b, c);

    free(a);
    free(b);
    free(c);

    return 0;
}

void matrix_multiply_and_compare(int8_t *a, int8_t *b, int32_t *c) {
    int m = 16;
    int n = 16;
    int k = 8;
    int32_t *o_c = (int32_t *)malloc(sizeof(int32_t) * MATRIX_SIZE * 2);
    vcix_mma8_asm(m, n, k, a, b, o_c);
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
                free(o_c);
                return;
            }
        }
    }
    free(o_c);
    printf("Test passed.\n");
}
