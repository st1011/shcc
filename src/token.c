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

// 特殊キーワードのマップを生成・取得
static Map *get_keywords(void)
{
    Map *keywords = new_map();

    map_puti(keywords, "return", TK_RETURN);

    return keywords;
}

// キーワード文字列を抜き出す
// キーワードが見つからなければfalse returnで何もしない
static bool keyword(Vector *tk, const Map *keywords, char **pp)
{
    int len = 1;
    char *p = *pp;
    while (isalpha(p[len]) || isdigit(p[len]) || p[len] == '_') {
        len++;
    }
    char *name = (char *)calloc(len + 1, sizeof(char));
    strncpy(name, p, len);
    name[len] = '\0';

    int ty = map_geti(keywords, name);

    // キーワードではなかった
    if (ty == 0) {
        return false;
    }

    vec_push_token(tk, ty, 0, name);
    p += len;

    *pp = p;

    return true;
}


// トークナイズの実行
// 出力はしない
Vector *tokenize(char *p)
{
    Vector *tk = new_vector();
    Map *keywords = get_keywords();

    while (*p) {
        // skip space
        if (isspace(*p)) {
            p++;
            continue;
        }

        // キーワード解析
        if (isalpha(*p) || *p == '_') {
            if (keyword(tk, keywords, &p)) {
                continue;
            }
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