#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "shcc.h"

// ノードリスト
Node *code[128] = {0};

//次のトークン位置
int pos = 0;

static Node *term(void);
static Node *mul(void);
static Node *add(void);

// ノード作成のbody
static Node *new_node_body(NodeType_t ty, Node *lhs, Node *rhs, int val, char name)
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
static Node *new_node(NodeType_t ty, Node *lhs, Node *rhs)
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
    return new_node_body(ND_IDENT, 0, 0, 0, name);
}

// returnノード
static Node *new_node_return(Node *lhs)
{
    return new_node(ND_RETURN, lhs, 0);
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
    if (tk->ty == TK_IDENT) {
        Node *node = new_node_ident(tk->input[0]);
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
            node = new_node(ND_MUL, node, term());
        }
        else if (consume(TK_DIV)) {
            node = new_node(ND_DIV, node, term());
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
            node = new_node(ND_PLUS, node, mul());
        }
        else if (consume(TK_MINUS)) {
            node = new_node(ND_MINUS, node, mul());
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
        node = new_node(ND_EQ, node, assign());
    }

    return node;
}

// ステートメントノード
static Node *stmt(void)
{
    Node *node;
    
    if (consume(TK_RETURN)) {
        node = new_node_return(assign());
    }
    else {
        node = assign();
    }

    if (!consume(TK_STMT)) {
        error("';'で終わらないトークンです");
    }

    return node;
}

// プログラム全体のノード作成
void program(void)
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