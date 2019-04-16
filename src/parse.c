#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "shcc.h"

// ノードリスト
Vector *code = 0;

//次のトークン位置
int pos = 0;

static Node *term(void);
static Node *mul(void);
static Node *add(void);
static Node *equal(void);
static Node *assign(void);

// ノード作成のbody
static Node *new_node_body(NodeType_t ty, Node *lhs, Node *rhs, int val, const char *name)
{
    Node *node = calloc(1, sizeof(Node));
    node->ty = ty;
    node->lhs = lhs;
    node->rhs = rhs;
    node->val = val;
    node->name = name;
    node->args = new_vector();

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
static Node *new_node_ident(const char *name)
{
    return new_node_body(ND_IDENT, 0, 0, 0, name);
}

// 関数ノード
static Node *new_node_funccall(const char *name)
{
    return new_node_body(ND_CALL, 0, 0, 0, name);
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
        Node *node = equal();

        if (!consume(TK_PRCLOSE)) {
            error("対応する閉じ括弧がありません");
        }

        return node;
    }

    Token *tk = tokens->data[pos];
    if (consume(TK_NUM)) {
        Node *node = new_node_num(tk->val);

        return node;
    }
    if (consume(TK_IDENT)) {
        Node *node = NULL;

        if (consume(TK_PROPEN)) {
            // 関数呼び出し
            node = new_node_funccall(tk->input);

            // 引数
            while (!consume(TK_PRCLOSE)) {
                vec_push(node->args, assign());
            }
        }
        else {
            // 変数
            node = new_node_ident(tk->input);
        }

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

// 比較演算子
static Node *equal(void)
{
    Node *node = add();

    for (;;) {
        if (consume(TK_EQ)) {
            node = new_node(ND_EQ, node, add());
        }
        else if (consume(TK_NEQ)) {
            node = new_node(ND_NEQ, node, add());
        }
        else {    
            return node;
        }
    }
}

// 一つの式ノード
static Node *assign(void)
{
    Node *node = equal();

    while (consume(TK_ASSIGN)) {
        node = new_node(ND_ASSIGN, node, assign());
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
    code = new_vector();

    for (;;) {
        Token *tk = tokens->data[pos];

        if (tk->ty == TK_EOF) {
            break;
        }

        vec_push(code, stmt());
    }

    vec_push(code, NULL);
}

// [種類] [演算子] [結合規則]
// 上ほど優先順位高い

// 関数, 添字, 構造体メンバ参照,後置増分/減分	() [] . -> ++ --	左→右
// 前置増分/減分, 単項式※	++ -- ! ~ + - * & sizeof	左←右
// キャスト	(型名)
// 乗除余	* / %	左→右
// 加減	+ -
// シフト	<< >>
// 比較	< <= > >=
// 等値	== !=
// ビットAND	&
// ビットXOR	^
// ビットOR	|
// 論理AND	&&
// 論理OR	||
// 条件	?:	左←右
// 代入	= += -= *= /= %= &= ^= |= <<= >>=
// コンマ	,	左→右
