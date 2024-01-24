#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#define ARRAY_SIZE 20

extern void vcix_mvvw_asm(int m, void *input1, void *input2, void *input3, void *output1, void *output2, int m2);
void matrix_multiply_and_compare(int8_t *input1, int8_t *input2, int16_t *input3, int8_t *output1, int16_t *output2);

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: %s <input_file> <output_file>\n", argv[0]);
        return 1;
    }

    const char *input_file_path = argv[1];
    const char *output_file_path = argv[2];
    
    // assign memory
    int8_t *input_array1 = (int8_t *)malloc(sizeof(int8_t) * ARRAY_SIZE);
    int8_t *input_array2 = (int8_t *)malloc(sizeof(int8_t) * ARRAY_SIZE);
    int16_t *input_array3 = (int16_t *)malloc(sizeof(int16_t) * ARRAY_SIZE);

    int8_t *output_array1 = (int8_t *)malloc(sizeof(int8_t) * ARRAY_SIZE * 2);
    int16_t *output_array2 = (int16_t *)malloc(sizeof(int16_t) * ARRAY_SIZE * 2);

    // read experiment data from file
    FILE *input_file = fopen(input_file_path, "rb");
    fread(input_array1, sizeof(int8_t), ARRAY_SIZE, input_file);
    fread(input_array2, sizeof(int8_t), ARRAY_SIZE, input_file);
    fread(input_array3, sizeof(int16_t), ARRAY_SIZE, input_file);

    fclose(input_file);
    vcix_mvvw_asm(ARRAY_SIZE, input_array1, input_array2, input_array3, output_array1, output_array2, 2 * ARRAY_SIZE);
    matrix_multiply_and_compare(input_array1, input_array2, input_array3, output_array1, output_array2);

    free(input_array1);
    free(input_array2);
    free(input_array3);
    free(output_array1);
    free(output_array2);

    return 0;
}

void matrix_multiply_and_compare(int8_t *input1, int8_t *input2, int16_t *input3, int8_t *output1, int16_t *output2) {
    for(int i = 0; i < ARRAY_SIZE; ++i) {
        if(output1[i] != input1[i]) {
            printf("TEST FAILED in location (%d), output1 is %d but should be input1 %d!!\n", i, output1[i], input1[i]);
            for(int i = 0; i < ARRAY_SIZE; ++i) {
                printf("%d ", output1[i]);  
            }
            printf("\n");
            return;
        }
    }

    for(int i = 0; i < ARRAY_SIZE; ++i) {
        if(output1[i + ARRAY_SIZE] != input2[i]) {
            printf("TEST FAILED in location (%d), output1 is %d but should be input2 %d!!\n", i, output1[i + ARRAY_SIZE], input1[i]);
            for(int i = 0; i < ARRAY_SIZE; ++i) {
                printf("%d ", output1[i + ARRAY_SIZE]);  
            }
            printf("\n");
            return;
        }
    }
    
    for(int i = 0; i < ARRAY_SIZE; ++i) {
        if(output2[i + ARRAY_SIZE] != input3[i]) {
            printf("TEST FAILED in location (%d), output is %d but should be %d!!\n", i, output2[i + ARRAY_SIZE], input3[i]);
            for(int i = 0; i < ARRAY_SIZE; ++i) {
                printf("%d ", output2[i + ARRAY_SIZE]);  
            }
            printf("\n");
            return;
        }
    }
    printf("Test passed.\n");
}
