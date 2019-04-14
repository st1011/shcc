#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// トークンの型を表す値
typedef enum {
    TK_PLUS = '+',
    TK_MINUS = '-',
    TK_MUL = '*',
    TK_DIV = '/',
    TK_PROPEN = '(',
    TK_PRCLOSE = ')',

    TK_NUM = 0x100,
    TK_EOF,
} TokenType_t;

// ノードの型を表す値
typedef enum {
    ND_NUM = 0x100, // 整数のノードの型
} NodeType_t;

// トークンの型
typedef struct {
    int ty;         // 型
    int val;        // TK_NUMなら数値
    char *input;    // トークン文字列（エラーメッセージ用）
} Token;

// 抽象構文木ノード
typedef struct Node {
    int ty;
    struct Node *lhs;
    struct Node *rhs;
    int val;
} Node;

static Node *term(void);
static Node *mul(void);
static Node *add(void);

// ノード作成のbody
static Node *new_node_body(int ty, Node *lhs, Node *rhs, int val)
{
    Node *node = malloc(sizeof(Node));
    node->ty = ty;
    node->lhs = lhs;
    node->rhs = rhs;
    node->val = val;

    return node;
}

// 通常ノード
static Node *new_node(int ty, Node *lhs, Node *rhs)
{
    return new_node_body(ty, lhs, rhs, 0);
}

// 数値ノード
static Node *new_node_num(int val)
{
    return new_node_body(ND_NUM, 0, 0, val);
}

Token tokens[128] = {0};
Token *current_token = 0;

// トークン解析失敗エラー
static void error(const char *msg, const Token *tk)
{
    fprintf(stderr, "%s: %s\n", msg, tk->input);

    exit(1);
}

// 次トークンがtyか確認し、tyの時のみトークンを一つ進める
static int consume(int ty)
{
    if (current_token->ty != ty) {
        return 0;
    }
    current_token++;

    return 1;
}

static Node *term(void)
{
    if (consume(TK_PROPEN)) {
        Node *node = add();

        if (!consume(TK_PRCLOSE)) {
            error("対応する閉じ括弧がありません", current_token);
        }

        return node;
    }

    if (current_token->ty == TK_NUM) {
        Node *node = new_node_num(current_token->val);
        current_token++;

        return node;
    }

    error("数値でも開き括弧でもないトークンです", current_token);
}

static Node *mul(void)
{
    Node *node = term();

    for (;;) {
        if (consume(TK_MUL)) {
            node = new_node(TK_MUL, node, term());
        }
        else if (consume(TK_DIV)) {
            node = new_node(TK_DIV, node, term());
        }
        else {
            return node;
        }
    }
}

static Node *add(void)
{
    Node *node = mul();

    for (;;) {
        if (consume(TK_PLUS)) {
            node = new_node(TK_PLUS, node, mul());
        }
        else if (consume(TK_MINUS)) {
            node = new_node(TK_MINUS, node, mul());
        }
        else {
            return node;
        }
    }
}

// パース用のラッパー
static Node *parse(void)
{
    return add();
}

// 一文字式か？
static bool is_oneop(char ch)
{
    // 数値以外で1文字式
    return ch == TK_PLUS || ch == TK_MINUS
        || ch == TK_MUL || ch == TK_DIV
        || ch == TK_PROPEN || ch == TK_PRCLOSE;
}

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

        if (is_oneop(*p)) {
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

// アセンブリ出力
static void gen_asm(Node *node)
{
    if (node->ty == ND_NUM) {
        printf("  push %d\n", node->val);
        return;
    }

    gen_asm(node->lhs);
    gen_asm(node->rhs);

    printf("  pop rdi\n");  // 右辺の値
    printf("  pop rax\n");  // 左辺の値

    switch (node->ty) {
        case TK_PLUS: {
            printf("  add rax, rdi\n");
            break;
        }
        case TK_MINUS: {
            printf("  sub rax, rdi\n");
            break;
        }
        case TK_MUL: {
            printf("  mul rdi\n");
            break;
        }
        case TK_DIV: {
            // div命令は rax =  ((rdx << 64) | rax) / rdi
            printf("mov rdx, 0\n");
            printf("  div rdi\n");
            break;
        }
    }

    printf("  push rax\n");
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "引数の個数が正しくありません\n");
        return 1;
    }

    // initialize
    current_token = tokens;

    // トークナイズ
    tokenize(argv[1], tokens);

    // パース
    Node *node = parse();

    // アセンブリ 前半出力
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    gen_asm(node);

    // スタックトップの値をraxにロードしてからreturnする
    printf("  pop rax\n");
    printf("    ret\n");

    return 0;
}