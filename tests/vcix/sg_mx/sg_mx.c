#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define TRANS_NUM 115
extern void vcix_mx_asm(int m, void *output, int num);
void matrix_multiply_and_compare(int8_t x, int8_t *output);

int main(int argc, char **argv) {
    char *endptr;
    int8_t x = (int8_t)strtoll(argv[1], &endptr, 10);
    // assign memory
    int8_t *output = (int8_t *)malloc(sizeof(int8_t) * TRANS_NUM);
    vcix_mx_asm(x, output, TRANS_NUM);
    matrix_multiply_and_compare(x, output);

    free(output);

    return 0;
}

void matrix_multiply_and_compare(int8_t x, int8_t *output) {
    for(int i = 0; i < TRANS_NUM; ++i) {
        if(output[i] != x) {
            printf("TEST FAILED in location (%d), output is %d but should be %d!!\n", i, output[i], x);
            for(int i = 0; i < TRANS_NUM; ++i) {
                printf("%d ", output[i]);  
            }
            printf("\n");
            return;
        }
    }
    printf("Test passed.\n");
}
