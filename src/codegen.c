#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "shcc.h"

// エピローグアセンブリ出力
void gen_asm_epilog(void)
{
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
}

// ノード解析失敗エラー
static void error(const char *msg)
{
    fprintf(stderr, "%s\n", msg);

    exit(1);
}

// 左辺値のアセンブリ出力
// 該当アドレスをpush
static void gen_asm_lval(Node *node)
{
    if (node->ty != ND_IDENT) {
        error("代入の左辺値が変数ではありません");
    }

    int offset = ('z' - node->name + 1) * 8;
    printf("  mov rax, rbp\n");
    printf("  sub rax, %d\n", offset);
    printf("  push rax\n");
}

// アセンブリ出力
void gen_asm(Node *node)
{
    if (node->ty == ND_RETURN) {
        gen_asm(node->lhs);
        printf("  pop rax\n");
        gen_asm_epilog();
        return;
    }
    if (node->ty == ND_NUM) {
        printf("  push %d\n", node->val);
        return;
    }

    if (node->ty == ND_IDENT) {
        // ここは右辺値の識別子
        // 一度左辺値としてpushした値をpopして使う
        gen_asm_lval(node);
        printf("  pop rax\n");
        printf("  mov rax, [rax]\n");
        printf("  push rax\n");
        return;
    }

    if (node->ty == ND_EQ) {
        // 代入式なら、必ず左辺は変数
        gen_asm_lval(node->lhs);
        gen_asm(node->rhs);
        printf("  pop rdi\n");
        printf("  pop rax\n");
        printf("  mov [rax], rdi\n");
        printf("  push rdi\n");
        return;
    }

    gen_asm(node->lhs);
    gen_asm(node->rhs);

    printf("  pop rdi\n");  // 右辺の値
    printf("  pop rax\n");  // 左辺の値

    switch (node->ty) {
        case ND_PLUS: {
            printf("  add rax, rdi\n");
            break;
        }
        case ND_MINUS: {
            printf("  sub rax, rdi\n");
            break;
        }
        case ND_MUL: {
            printf("  mul rdi\n");
            break;
        }
        case ND_DIV: {
            // div命令は rax =  ((rdx << 64) | rax) / rdi
            printf("  mov rdx, 0\n");
            printf("  div rdi\n");
            break;
        }

        default: {
            exit(1);
            break;
        }
    }

    printf("  push rax\n");
}

// 適当なアセンブリを出力して終了する
void exit_with_asm(void)
{
    // アセンブリ 前半出力
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    printf("  mov rax, 42\n");
    printf("  ret\n");

    exit(1);
}