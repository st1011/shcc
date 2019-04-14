#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

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

// ノード用ベクター
typedef struct {
    void **data;
    int capacity;
    int len;
} Vector;

Vector *tokens = 0;
//次のトークン位置
int pos = 0;

// 新規vector作成
static Vector *new_vector(void)
{
    Vector *vec = calloc(1, sizeof(Vector));

    vec->data = calloc(16, sizeof(void *));
    vec->capacity = 16;
    vec->len = 0;

    return vec;
}

// vectorへの要素追加
// 空きがなければ2倍に拡張する
static void vec_push(Vector *vec, void *elem)
{
    if (vec->capacity == vec->len) {
        vec->capacity *= 2;
        vec->data = realloc(vec->data, sizeof(void *) * vec->capacity);
    }

    vec->data[vec->len++] = elem;
}

// vectorにtokenを追加
static void vec_push_token(Vector *vec, int ty, int val, char *input)
{
    Token *tk = (Token *)calloc(1, sizeof(Token));

    tk->ty = ty;
    tk->val = val;
    tk->input = input;

    vec_push(vec, tk);
}

// 簡易テスト用
static int expect(int line, int expected, int actual)
{
    if (expected == actual) {
        return 0;
    }

    fprintf(stderr, "%d: %d expected, but got %d\n", 
        line, expected, actual);

    exit(1);
}

// vector用テスト
void runtest(void)
{
    Vector *vec = new_vector();
    expect(__LINE__, 0, vec->len);

    for (int i = 0; i < 100; i++) {
        vec_push(vec, (void *)(intptr_t)i);
    }

    expect(__LINE__, 100, vec->len);
    expect(__LINE__, 0, (intptr_t)vec->data[0]);
    expect(__LINE__, 50, (intptr_t)vec->data[50]);
    expect(__LINE__, 99, (intptr_t)vec->data[99]);

    printf("OK\n");
}



static Node *term(void);
static Node *mul(void);
static Node *add(void);

// ノード作成のbody
static Node *new_node_body(int ty, Node *lhs, Node *rhs, int val)
{
    Node *node = calloc(1, sizeof(Node));
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

// トークン解析失敗エラー
static void error(const char *msg, const Token *tk)
{
    fprintf(stderr, "%s: %s\n", msg, tk->input);

    exit(1);
}

// 次トークンがtyか確認し、tyの時のみトークンを一つ進める
static int consume(int ty)
{
    Token *tk = tokens->data[pos];

    if (tk->ty != ty) {
        return 0;
    }
    pos++;

    return 1;
}

// 末尾ノード 括弧か数値
static Node *term(void)
{
    Token *tk = tokens->data[pos];

    if (consume(TK_PROPEN)) {
        Node *node = add();

        if (!consume(TK_PRCLOSE)) {
            error("対応する閉じ括弧がありません", tk);
        }

        return node;
    }

    if (tk->ty == TK_NUM) {
        Node *node = new_node_num(tk->val);
        pos++;

        return node;
    }

    error("数値でも開き括弧でもないトークンです", tk);

    return 0;
}

// かけ算、足し算ノード
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

// 先頭ノード 足し算、引き算
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
static void tokenize(char *p, Vector *tk)
{
    while (*p) {
        // skip space
        if (isspace(*p)) {
            p++;
            continue;
        }

        if (is_oneop(*p)) {
            vec_push_token(tk, *p, 0, p);
            p++;
            continue;
        }

        if (isdigit(*p)) {
            char *bp = p;
            vec_push_token(tk, TK_NUM, strtol(p, &p, 10), bp);
            continue;
        }

        fprintf(stderr, "トークナイズできません： %s\n", p);
        exit(1);
    }

    vec_push_token(tk, TK_EOF, 0, p);
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

// 適当なアセンブリを出力して終了する
static void exit_with_asm(void)
{
    // アセンブリ 前半出力
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    printf("  mov rax, 42\n");
    printf("    ret\n");

    exit(1);
}

// main
int main(int argc, char **argv)
{
    // exit_with_asm are not used
    (void)exit_with_asm;

    if (argc == 2 && (strcmp(argv[1], "-test") == 0)) {
        runtest();
        return 0;
    }

    if (argc != 2) {
        fprintf(stderr, "引数の個数が正しくありません\n");
        return 1;
    }

    // initialize
    tokens = new_vector();

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
