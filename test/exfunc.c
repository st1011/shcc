// テストのために必要な、本コンパイラにはまだ未実装な処理を記述するためのファイルです。

// テストの実行にはこのファイルをコンパイルして作成されるオブジェクトファイルが必要です。
// テストではそのオブジェクトファイルと本コンパイラが出力したオブジェクトファイルをリンクして実行されます。

// 本ファイルのコンパイルは下記のコマンドで実行します。
// $ gcc -c exfunc.c

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

// int型の引数を表示する
void print_int(int a)
{
    printf("[%d]", a);
}
