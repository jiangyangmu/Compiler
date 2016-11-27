#include <stdio.h>
#include <assert.h>

/*
void verify_add()
{
    unsigned int u1, u2, u3;
    int i1, i2, i3;
    for (unsigned int i = 0; i <= INT_MAX; ++i)
    {
        u1 = u2 = i;
        i1 = i2 = i;

        u3 = u1 * u2;
        i3 = i1 * i2;
    }
}
*/

int main(void)
{
    // ******* Integer Conversion ******

    // sign conversion
    // 1000 0000 0000 0000 : -32768
    short snint1 = -32768;
    // 1000 0000 0000 0001 : -32767
    short snint2 = -32767;
    // 1111 1111 1111 1111
    short snint3 = -1;
    short snint4 = 0;

    // 1111 .... 1111 1111 : -1
    // signed -> unsigned
    unsigned int i1 = -1;
    // 1111 .... 1111 1110 : 4294967294
    // unsigned -> signed
    int i2 = 4294967294;
    printf("%u\n", (unsigned short)snint1); // output: 32768
    printf("%u\n", (unsigned short)snint2); // output: 32769
    printf("%u\n", (unsigned short)snint3); // output: 65535
    printf("%u\n", (unsigned short)snint4); // output: 0
    printf("%u\n", i1); // output: 4294967295
    printf("%d\n", i2); // output: -2

    if (10u - 100u >= 0) printf("10u >= 100u\n");
    else printf("10u < 100u\n");

    // promotion & demotion
    // 1111 0000 0000 0000 : -4096
    short sint = -4096;
    // 1111 0000 0001 0000 : -4096+16
    short sint2 = -4096+16;
    // 1111 0000 0001 0000 : -4096+127
    short sint3 = -4096+127;
    // 1111 0000 1000 0000 : -4096+128
    short sint4 = -4096+128;
    // 1111 0000 1000 0001 : -4096+129
    short sint5 = -4096+129;

    printf("%d\n", (signed char)(sint)); // output: 0
    printf("%d\n", (signed char)(sint2)); // output: 16
    printf("%d\n", (signed char)(sint3)); // output: 127
    printf("%d\n", (signed char)(sint4)); // output: -128
    printf("%d\n", (signed char)(sint5)); // output: -127

    // 0000 1000 .... 0000
    int mul1 = (1 << 27);
    // 0000 0000 .... 1000
    int mul2 = (1 << 3);
    int mul3 = (1 << 4);
    int mul4 = (1 << 5);
    printf("%d\n", (mul1 * mul2));
    printf("%d\n", (mul1 * mul3));
    printf("%d\n", (mul1 * mul4));

    int ov1 = 1000000001;
    int ov2 = ov1 * 3;
    int ov3 = ov2 / 3;
    printf("%d %d %d\n", ov1, ov2, ov3);

    unsigned short e = 65530U;
    for (unsigned short s1 = 0; s1 <= e; ++s1)
    {
        for (unsigned short s2 = 0; s2 <= e; ++s2)
        {
            unsigned short s3 = s1 + s2;
            unsigned short ori_s1 = s3 - s2;
            unsigned short ori_s2 = s3 - s1;
            assert( s1 == ori_s1 && s2 == ori_s2 );
        }
        printf("\r%d", s1);
    }


    return 0;
}
