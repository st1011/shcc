#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "shcc.h"

Vector *tokens = 0;

// 一文字式か？
static bool is_oneop(char ch)
{
    // 数値以外で1文字式
    return ch == TK_PLUS || ch == TK_MINUS
        || ch == TK_MUL || ch == TK_DIV
        || ch == TK_PROPEN || ch == TK_PRCLOSE
        || ch == TK_EQ
        || ch == TK_STMT;
}

// トークナイズの実行
// 出力はしない
Vector *tokenize(char *p)
{
    Vector *tk = new_vector();

    while (*p) {
        // skip space
        if (isspace(*p)) {
            p++;
            continue;
        }

        // キーワード解析
        static const char *ret = "return";
        if (strncmp(p, ret, strlen(ret)) == 0) {
            vec_push_token(tk, TK_RETURN, 0, p);
            p += strlen(ret);
            continue;
        }

        // variables
        if (*p >= 'a' && *p <= 'z') {
            vec_push_token(tk, TK_IDENT, 0, p);
            p++;
            continue;
        }

        // operand
        if (is_oneop(*p)) {
            vec_push_token(tk, *p, 0, p);
            p++;
            continue;
        }

        // number
        if (isdigit(*p)) {
            char *bp = p;
            vec_push_token(tk, TK_NUM, strtol(p, &p, 10), bp);
            continue;
        }

        fprintf(stderr, "トークナイズできません： %s\n", p);
        exit(1);
    }

    vec_push_token(tk, TK_EOF, 0, p);

    return tk;
}