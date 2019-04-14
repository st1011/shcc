#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "shcc.h"

void runtest(void);

// main
int main(int argc, char **argv)
{
    if (argc == 2 && (strcmp(argv[1], "-test") == 0)) {
        runtest();
        return 0;
    }

    if (argc != 2) {
        fprintf(stderr, "引数の個数が正しくありません\n");
        return 1;
    }

    // トークナイズ
    tokens = tokenize(argv[1]);

    // パース
    program();

    // アセンブリ 前半出力
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    // アセンブリ プロローグ
    printf("  push rbp\n");     // 呼び出し元のベースポインタを保存
    printf("  mov rbp, rsp\n");
    // 現在の言語仕様に存在する変数領域を一括で待避する
    printf("  sub rsp, %d\n", ('z' - 'a' + 1) * 8);

    // 先頭の式から順にコード生成
    for (int i = 0; code->data[i]; i++) {
        gen_asm((Node *)code->data[i]);

        // 式の評価結果としてpushされた値が一つあるので
        // スタックがあふれないようにpopする
        printf("  pop rax\n");
    }

    // エピローグ
    // 戻り値は最後の評価結果のrax値
    // ポインタを戻す
    gen_asm_epilog();

    return 0;
}
