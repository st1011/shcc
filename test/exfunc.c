// このファイルからオブジェクトファイルを作っておく
// $ gcc -c exfunc.c
// 出力したオブジェクトとこのファイルをリンクしてテストする

#include <stdio.h>

// 引数も戻り値もない関数
void exfunc1(void)
{
    printf("\n[%s]\n", __func__);
}

// 引数を一つとる関数
void exfunc2(int a)
{
    printf("\n[%s]\n", __func__);
    printf("\targ1: %d\n", a);
}

// 引数を複数とる関数
void exfunc3(int a, int b)
{
    printf("\n[%s]\n", __func__);
    printf("\targ1: %d, arg2: %d\n", a, b);
}

// 引数はないが戻り値がある関数
int exfunc4(void)
{
    printf("\n[%s]\n", __func__);

    return 42;
}

// 引数も戻り値もある関数
int exfunc5(int a, int b)
{
    printf("\n[%s]\n", __func__);

    return a + b;
}
