#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#define NUMOF(ary)          (sizeof(ary) / sizeof((ary)[0]))

// トークンの型を表す値
typedef enum {
    TK_PLUS = '+',
    TK_MINUS = '-',
    TK_MUL = '*',
    TK_DIV = '/',
    TK_PROPEN = '(',
    TK_PRCLOSE = ')',
    TK_EQ = '=',
    TK_STMT = ';',

    TK_NUM = 0x100,     // 整数
    TK_IDENT,           // 識別子
    TK_EOF,             // 終端
} TokenType_t;

// トークンの型
typedef struct {
    int ty;         // 型
    int val;        // TK_NUMなら数値
    char *input;    // トークン文字列（エラーメッセージ用）
} Token;

// ノードの型を表す値
typedef enum {
    ND_NUM = 0x100, // 整数のノードの型
    ND_IDENT,       // 識別子のノードの型
} NodeType_t;

// 抽象構文木ノード
typedef struct Node {
    int ty;
    struct Node *lhs;
    struct Node *rhs;
    int val;                // ND_NUMの場合の数値
    char name;              // ND_IDENTの場合の名前
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
static Node *new_node_body(int ty, Node *lhs, Node *rhs, int val, char name)
{
    Node *node = calloc(1, sizeof(Node));
    node->ty = ty;
    node->lhs = lhs;
    node->rhs = rhs;
    node->val = val;
    node->name = name;

    return node;
}

// 通常ノード
static Node *new_node(int ty, Node *lhs, Node *rhs)
{
    return new_node_body(ty, lhs, rhs, 0, 0);
}

// 数値ノード
static Node *new_node_num(int val)
{
    return new_node_body(ND_NUM, 0, 0, val, 0);
}

// 識別子ノード
static Node *new_node_ident(char name)
{
    return new_node_body(ND_NUM, 0, 0, 0, name);
}

// トークン解析失敗エラー
static void error(const char *msg)
{
    Token *tk = tokens->data[pos];
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
    if (consume(TK_PROPEN)) {
        Node *node = add();

        if (!consume(TK_PRCLOSE)) {
            error("対応する閉じ括弧がありません");
        }

        return node;
    }

    Token *tk = tokens->data[pos];
    if (tk->ty == TK_NUM) {
        Node *node = new_node_num(tk->val);
        pos++;

        return node;
    }

    error("数値でも開き括弧でもないトークンです");

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

// 一つの式ノード
static Node *assign(void)
{
    Node *node = add();

    while (consume(TK_EQ)) {
        node = new_node(TK_EQ, node, assign());
    }

    return node;
}

// ステートメントノード
static Node *stmt(void)
{
    Node *node = assign();

    if (!consume(TK_STMT)) {
        error("';'で終わらないトークンです");
    }

    return node;
}

// ノードリスト
Node *code[128] = {0};

// プログラム全体のノード作成
static void program(void)
{
    Node **cd = code;

    for (;;) {
        Token *tk = tokens->data[pos];

        if (tk->ty == TK_EOF) {
            break;
        }

        *cd++ = stmt();
    }

    *cd = 0;
}

// 一文字式か？
static bool is_oneop(char ch)
{
    // 数値以外で1文字式
    return ch == TK_PLUS || ch == TK_MINUS
        || ch == TK_MUL || ch == TK_DIV
        || ch == TK_PROPEN || ch == TK_PRCLOSE
        || ch == TK_STMT;
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

        // variables
        if (*p >= 'a' && *p < 'z') {
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
}

// 左辺値のアセンブリ出力
// 該当アドレスをpush
void gen_asm_lval(Node *node)
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
static void gen_asm(Node *node)
{
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

    if (node->ty == TK_EQ) {
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
            printf("  mov rdx, 0\n");
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
    printf("  ret\n");

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
    program();

    // アセンブリ 前半出力
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    // アセンブリ プロローグ
    printf("  push rbp\n");     // 呼び出し元のベースポインタを保存
    printf("  mov rbp, rsp\n");
    // 現在の言語仕様に存在する変数領域を一括で待避する
    printf("  sub rsp, %d\n", ('z' - 'a' + 1) * 8);

    // 先頭の式から順にコード生成
    Node **cd = code;
    while (*cd) {
        gen_asm(*cd);

        // 式の評価結果としてpushされた値が一つあるので
        // スタックがあふれないようにpopする
        printf("  pop rax\n");
        cd++;
    }

    // エピローグ
    // 戻り値は最後の評価結果のrax値
    // ポインタを戻す
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");

    return 0;
}
