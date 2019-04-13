#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// トークンの型を表す値
enum {
    TK_NUM = 0x100,
    TK_EOF,
};

// トークンの型
typedef struct {
    int ty;         // 型
    int val;        // TK_NUMなら数値
    char *input;    // トークン文字列（エラーメッセージ用）
} Token;

Token tokens[128];

// トークナイズの実行
// 出力はしない
static void tokenize(char *p, Token *tk)
{
    while (*p) {
        // skip space
        if (isspace(*p)) {
            p++;
            continue;
        }

        if (*p == '+' || *p == '-') {
            tk->ty = *p;
            tk->input = p;
            tk++;
            p++;
            continue;
        }

        if (isdigit(*p)) {
            tk->ty = TK_NUM;
            tk->input = p;
            tk->val = strtol(p, &p, 10);
            tk++;
            continue;
        }

        fprintf(stderr, "トークナイズできません： %s\n", p);
        exit(1);
    }

    tk->ty = TK_EOF;
    tk->input = p;
}

static void error(const Token *tk)
{
    fprintf(stderr, "予期しないトークンです: %s\n",
        tk->input);

    exit(1);
}

// トークンリストからアセンブリを出力
static void print_tokens(const Token *tk)
{
    // 式の最初は数値の必要がある
    if (tk->ty != TK_NUM) {
        error(0);
    }

    // 最初の命令
    printf("  mov rax, %d\n", tk->val);

    tk++;
    while (tk->ty != TK_EOF) {
        switch (tk->ty) {
            case '+': {
                tk++;
                if (tk->ty != TK_NUM) {
                    error(tk);
                }
                
                printf("  add rax, %d\n", tk->val);
                tk++;

                continue;
            }

            case '-': {
                tk++;
                if (tk->ty != TK_NUM) {
                    error(tk);
                }
                
                printf("  sub rax, %d\n", tk->val);
                tk++;

                continue;
            }

            default: {
                error(tk);
            }
        }
    }
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "引数の個数が正しくありません\n");
        return 1;
    }

    // トークナイズ
    tokenize(argv[1], tokens);

    // アセンブリ 前半出力
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    print_tokens(tokens);

    printf("    ret\n");

    return 0;
}