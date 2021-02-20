#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "shcc.h"

// vectorにtokenを追加
static void vec_push_token(Vector *vec, TokenType_t ty, int val, char *input)
{
    Token *tk = (Token *)calloc(1, sizeof(Token));

    tk->ty = ty;
    tk->val = val;
    tk->input = input;

    vec_push(vec, tk);
}

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

// 演算子のマップを生成・取得
static Map *get_operator_list(void)
{
    Map *operator_list = new_map();

    map_puti(operator_list, "==", TK_EQ);
    map_puti(operator_list, "!=", TK_NEQ);
    map_puti(operator_list, "<=", TK_LESS_EQ);
    map_puti(operator_list, ">=", TK_GREATER_EQ);
    map_puti(operator_list, "+=", TK_ADD_ASSIGN);
    map_puti(operator_list, "-=", TK_SUB_ASSIGN);
    map_puti(operator_list, "*=", TK_MUL_ASSIGN);
    map_puti(operator_list, "/=", TK_DIV_ASSIGN);
    map_puti(operator_list, "%=", TK_MOD_ASSIGN);

    map_puti(operator_list, "+", TK_PLUS);
    map_puti(operator_list, "-", TK_MINUS);
    map_puti(operator_list, "*", TK_MUL);
    map_puti(operator_list, "/", TK_DIV);
    map_puti(operator_list, "%", TK_MOD);
    map_puti(operator_list, "(", TK_PROPEN);
    map_puti(operator_list, ")", TK_PRCLOSE);
    map_puti(operator_list, "=", TK_ASSIGN);
    map_puti(operator_list, "<", TK_LESS);
    map_puti(operator_list, ">", TK_GREATER);
    map_puti(operator_list, "{", TK_BRACE_OPEN);
    map_puti(operator_list, "}", TK_BRACE_CLOSE);
    map_puti(operator_list, "&", TK_ADDR);
    map_puti(operator_list, "*", TK_DEREF);
    map_puti(operator_list, ";", TK_STMT);

    return operator_list;
}

// 予約後のマップを生成・取得
static Map *get_reserved_words(void)
{
    Map *reserved_words = new_map();

    map_puti(reserved_words, "return", TK_RETURN);
    map_puti(reserved_words, "if", TK_IF);
    map_puti(reserved_words, "else", TK_ELSE);
    map_puti(reserved_words, "for", TK_FOR);
    map_puti(reserved_words, "while", TK_WHILE);

    return reserved_words;
}

// オペレータのトークナイズ
static int consume_operator(Vector *tk, const Map *operator_list, const char *p)
{
    const int MAX_OPERATOR_LENGTH = 2;

    char *operator=(char *) calloc(MAX_OPERATOR_LENGTH + 1, sizeof(char));
    strncpy(operator, p, MAX_OPERATOR_LENGTH);

    for (int i = MAX_OPERATOR_LENGTH; i > 0; i--)
    {
        // 探索したい長さのオペレータではない
        if (operator[i - 1] == '\0')
        {
            continue;
        }
        operator[i] = '\0';

        TokenType_t ty = (TokenType_t)map_geti(operator_list, operator);
        if (ty != TK_INVALID)
        {
            vec_push_token(tk, ty, 0, operator);
            return i;
        }
    }

    // operatorが見つからなかった
    return 0;
}

// 引数の先頭からidentの構成要素の連続列の長さを取得する
static int get_ident_length(const char *p)
{
    if (!is_idnet_head_char(*p))
    {
        // identの先頭に数字は使えない
        return 0;
    }

    for (int len = 0;; len++)
    {
        if (!is_idnet_char(p[len]))
        {
            return len;
        }
    }
}

// 識別子をトークナイズ
// 変数も予約語もここで処理する
static int consume_ident(Vector *tk, const Map *rwords, const char *p)
{
    // キーワード文字列を抜き出す
    int len = get_ident_length(p);
    if (len == 0)
    {
        return 0;
    }

    char *name = (char *)calloc(len + 1, sizeof(char));
    strncpy(name, p, len);
    name[len] = '\0';

    // 抜き出した文字列の識別子を調べる
    TokenType_t ty = (TokenType_t)map_geti(rwords, name);
    if (ty == TK_INVALID)
    {
        // 予約後ではないので、変数
        ty = TK_IDENT;
    }

    vec_push_token(tk, ty, 0, name);

    return len;
}

// トークナイズの実行
// 出力はしない
Vector *tokenize(char *p)
{
    Vector *tk = new_vector();
    Map *operators = get_operator_list();
    Map *rwords = get_reserved_words();

    while (*p)
    {
        // skip space
        if (isspace(*p))
        {
            p++;
            continue;
        }

        // 演算子
        int operator_length = consume_operator(tk, operators, p);
        if (operator_length != 0)
        {
            p += operator_length;
            continue;
        }

        // 識別子
        // 変数も予約後もここで
        int ident_length = consume_ident(tk, rwords, p);
        if (ident_length != 0)
        {
            p += ident_length;
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
