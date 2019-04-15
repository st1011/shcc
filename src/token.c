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

// 予約後のマップを生成・取得
static Map *get_reserved_words(void)
{
    Map *reserved_words = new_map();

    map_puti(reserved_words, "return", TK_RETURN);

    return reserved_words;
}

// 識別子をトークナイズ
// 変数も予約語もここで処理する
static char *ident(Vector *tk, const Map *rwords, char *p)
{
    // キーワード文字列を抜き出す
    int len = 1;
    while (isalpha(p[len]) || isdigit(p[len]) || p[len] == '_') {
        len++;
    }
    char *name = (char *)calloc(len + 1, sizeof(char));
    strncpy(name, p, len);
    name[len] = '\0';

    // 抜き出した文字列の識別子を調べる
    int ty = map_geti(rwords, name);
    if (ty == 0) {
        // 予約後ではないので、変数
        ty = TK_IDENT;
    }

    vec_push_token(tk, ty, 0, name);

    return p + len;
}


// トークナイズの実行
// 出力はしない
Vector *tokenize(char *p)
{
    Vector *tk = new_vector();
    Map *rwords = get_reserved_words();

    while (*p) {
        // skip space
        if (isspace(*p)) {
            p++;
            continue;
        }

        // 識別子
        // 変数も予約後もここで
        if (isalpha(*p) || *p == '_') {
            p = ident(tk, rwords, p);
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