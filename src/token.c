#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "shcc.h"

Vector *tokens = 0;

static struct {
    TokenType_t ty;
    const char *name;
} symbols[ ] = {
    {TK_EQ, "=="}, {TK_NEQ, "!="},
    {TK_LESS_EQ, "<="}, {TK_GREATER_EQ, ">="},
};

// トークン一覧の出力
void dump_token_list(Vector *token_list)
{
    for (int i = 0; i < token_list->len; i++) {
        Token *tk = token_list->data[i];

        printf("# ty: ");
        switch (tk->ty) {
            case TK_NUM:            printf("NUM");      break;
            case TK_IDENT:          printf("ID");       break;
            case TK_RETURN:         printf("RET");      break;
            case TK_EQ:             printf("==");       break;
            case TK_NEQ:            printf("!=");       break;
            case TK_LESS_EQ:        printf("<=");       break;
            case TK_GREATER_EQ:     printf(">=");       break;
            case TK_EOF:            printf("EOF");      break;
            default: {
                printf("%c", tk->ty);
                break;
            }
        }
        printf("\n");
    }
}

// 一文字式か？
static bool is_oneop(char ch)
{
    // 数値以外で1文字式
    return ch == TK_PLUS || ch == TK_MINUS
        || ch == TK_MUL || ch == TK_DIV
        || ch == TK_MOD
        || ch == TK_PROPEN || ch == TK_PRCLOSE
        || ch == TK_ASSIGN
        || ch == TK_LESS || ch == TK_GREATER
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

// 複数文字のoperand
static int multi_symbols(Vector *tk, char *p)
{
    int len = 0;

    for (int i = 0; i < NUMOF(symbols); i++) {
        const char *name = symbols[i].name;

        if (strncmp(p, name, strlen(name)) == 0) {
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

    while (*p) {
        // skip space
        if (isspace(*p)) {
            p++;
            continue;
        }

        // 複数文字のoperand
        int next = multi_symbols(tk, p);
        if (next != 0) {
            p += next;
            continue;
        }

        // operand
        if (is_oneop(*p)) {
            vec_push_token(tk, *p, 0, p);
            p++;
            continue;
        }

        // 識別子
        // 変数も予約後もここで
        if (isalpha(*p) || *p == '_') {
            p = ident(tk, rwords, p);
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
