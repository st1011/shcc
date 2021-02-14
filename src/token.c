#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "shcc.h"

// 複数文字からなるトークン
static struct
{
    TokenType_t ty;
    const char *name;
} symbols[] = {
    {TK_EQ, "=="},
    {TK_NEQ, "!="},
    {TK_LESS_EQ, "<="},
    {TK_GREATER_EQ, ">="},
};

// 識別子の先頭になり得る文字か？
// [a-zA-Z_]
static bool is_idnet_head_char(char ch)
{
    return isalpha(ch) || ch == '_';
}

// 識別の構成要素になり得る文字か？
// [a-zA-Z0-9_]
static bool is_idnet_char(char ch)
{
    return is_idnet_head_char(ch) || isdigit(ch);
}

// 一文字式か？
static bool is_oneop(char ch)
{
    // 数値以外で1文字式
    return ch == TK_PLUS || ch == TK_MINUS || ch == TK_MUL || ch == TK_DIV || ch == TK_MOD || ch == TK_PROPEN || ch == TK_PRCLOSE || ch == TK_ASSIGN || ch == TK_LESS || ch == TK_GREATER || ch == ND_BRACE_OPEN || ch == ND_BRACE_CLOSE || ch == TK_STMT;
}

// 予約後のマップを生成・取得
static Map *get_reserved_words(void)
{
    Map *reserved_words = new_map();

    map_puti(reserved_words, "return", TK_RETURN);
    map_puti(reserved_words, "if", TK_IF);
    map_puti(reserved_words, "else", TK_ELSE);
    map_puti(reserved_words, "for", TK_FOR);

    return reserved_words;
}

// 識別子をトークナイズ
// 変数も予約語もここで処理する
static char *ident(Vector *tk, const Map *rwords, char *p)
{
    // キーワード文字列を抜き出す
    int len = 1;
    while (is_idnet_char(p[len]))
    {
        len++;
    }
    char *name = (char *)calloc(len + 1, sizeof(char));
    strncpy(name, p, len);
    name[len] = '\0';

    // 抜き出した文字列の識別子を調べる
    int ty = map_geti(rwords, name);
    if (ty == 0)
    {
        // 予約後ではないので、変数
        ty = TK_IDENT;
    }

    vec_push_token(tk, ty, 0, name);

    return p + len;
}

// 複数文字のoperand
static int multi_symbols(Vector *tk, char *p)
{
    int len = 0;

    for (int i = 0; i < NUMOF(symbols); i++)
    {
        const char *name = symbols[i].name;

        if (strncmp(p, name, strlen(name)) == 0)
        {
            vec_push_token(tk, symbols[i].ty, 0, p);
            len = strlen(name);
            break;
        }
    }

    return len;
}

// トークナイズの実行
// 出力はしない
Vector *tokenize(char *p)
{
    Vector *tk = new_vector();
    Map *rwords = get_reserved_words();

    while (*p)
    {
        // skip space
        if (isspace(*p))
        {
            p++;
            continue;
        }

        // 複数文字のoperand
        int next = multi_symbols(tk, p);
        if (next != 0)
        {
            p += next;
            continue;
        }

        // operand
        if (is_oneop(*p))
        {
            vec_push_token(tk, *p, 0, p);
            p++;
            continue;
        }

        // 識別子
        // 変数も予約後もここで
        if (is_idnet_head_char(*p))
        {
            p = ident(tk, rwords, p);
            continue;
        }

        // number
        if (isdigit(*p))
        {
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
