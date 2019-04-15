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

    // プロローグ
    gen_asm_prologue();

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
