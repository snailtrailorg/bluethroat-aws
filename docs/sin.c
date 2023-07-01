#include <stdio.h>
#include <math.h>

void main(void) {

    int count = 30030;
    int bitwidth = 16;

    double mul = pow(2,bitwidth) / 2.0 - 1;

    int i, j;

    double x, y;
    printf("/* \n    The lookup table of the trigonometric function sin, \n    extends the independent variable from 0~2*pi to %d(0~%d) discrete data, \n    and extends the value from -1~+1 to a 16 bits signed integer.\n*/\n\n", count, count-1, bitwidth);
    printf("#include <stdint.h>\n\n");
    printf("const uint%d_t sin_table[] = {", bitwidth);

    for (i=0, j=0; i<count; i++, j++) {
        if (j%16 == 0) {
            printf("\n    /* 0x%8.8x: */ ", i*2);
        }
        x = M_PI * 2.0 * (double)i / (double)count;
        y = sin(x) * mul;

        switch (bitwidth) {
        case 32:
            printf("0x%8.8x, ", ((int)y) & 0xffffffff);
            break;
        case 28:
            printf("0x%7.7x, ", ((int)y) & 0x0fffffff);
            break;
        case 24:
            printf("0x%6.6x, ", ((int)y) & 0x00ffffff);
            break;
        case 20:
            printf("0x%5.5x, ", ((int)y) & 0x000fffff);
            break;
        case 16:
            printf("0x%4.4x, ", ((int)y) & 0x0000ffff);
            break;
        case 12:
            printf("0x%3.3x, ", ((int)y) & 0x00000fff);
            break;
        case 8:
            printf("0x%2.2x, ", ((int)y) & 0x000000ff);
            break;
        default:
            printf("0x%x, ", (long)y);
        }
    }

    printf("\n};\n");
}