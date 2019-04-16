// このファイルからオブジェクトファイルを作っておく
// $ gcc -c exfunc.c
// 出力したオブジェクトとこのファイルをリンクしてテストする

#include <stdio.h>

// 引数も戻り値もない関数
void exfunc1(void)
{
    printf("[%s]\n", __func__);
    printf("\tCall OK\n");
}

// 引数を一つとる関数
void exfunc2(int a)
{
    printf("[%s]\n", __func__);
    printf("\targ1: %d\n", a);
    printf("\tCall OK\n");
}

// 引数を複数とる関数
void exfunc3(int a, int b)
{
    printf("[%s]\n", __func__);
    printf("\targ1: %d, arg2: %d\n", a, b);
    printf("\tCall OK\n");
}

// 引数はないが戻り値がある関数
int exfunc4(void)
{
    printf("[%s]\n", __func__);
    printf("\tCall OK\n");

    return 0;
}

// 引数も戻り値もある関数
int exfunc5(int a, int b)
{
    printf("[%s]\n", __func__);
    printf("\tCall OK\n");

    return a + b;
}
