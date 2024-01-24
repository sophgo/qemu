#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


extern void vcix_mma8_asm(int m, int n, int k, void *a, void *b, void *d);

int main(void)
{
    unsigned char a[] = {1, 2, 3, 4, 5, 6};
    unsigned char b[] = {1, 4, 8, 5, 0, 1, 1, 1};
    int c[] = {9, 18, 2, 3, 19, 44, 4, 7, 29, 70, 6, 11};
    int d[12] = {0}; 
    int m = 3;
    int n = 4;
    int k = 2;
    vcix_mma8_asm(m, n, k, a, b, d);
    int i = 12;
    while(i) {
        printf("%d ", d[12 - i] - c[12 - i]);--i;
    }
    printf("\n");
    return EXIT_SUCCESS;
}
