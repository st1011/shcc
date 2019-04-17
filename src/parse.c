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

static Node *expr(void);
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
    node->block_stmts = new_vector();

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

// Block ノード
static Node *new_node_block(void)
{
    return new_node(ND_BLOCK, 0, 0);
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
        Node *node = expr();

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

// 前置増分/減分, 単項式
// ++ -- ! ~ +-（符号） * & sizeof()
static Node *monomial(void)
{
    Node *node = term();

    return node;
}

// キャスト演算子
static Node *cast(void)
{
    Node *node = monomial();

    return node;
}

// 乗除余演算子
static Node *mul(void)
{
    Node *node = cast();

    for (;;) {
        if (consume(TK_MUL)) {
            node = new_node(ND_MUL, node, mul());
        }
        else if (consume(TK_DIV)) {
            node = new_node(ND_DIV, node, mul());
        }
        else if (consume(TK_MOD)) {
            node = new_node(ND_MOD, node, mul());
        }
        else {
            return node;
        }
    }
}

// 加減演算子
static Node *add(void)
{
    Node *node = mul();

    for (;;) {
        if (consume(TK_PLUS)) {
            node = new_node(ND_PLUS, node, add());
        }
        else if (consume(TK_MINUS)) {
            node = new_node(ND_MINUS, node, add());
        }
        else {       
            return node;
        }
    }
}

// シフト演算子
static Node *shift(void)
{
    Node *node = add();

    return node;
}

// 比較演算子
static Node *comparison(void)
{
    Node *node = shift();

    for (;;) {
        if (consume(TK_LESS)) {
            node = new_node(ND_LESS, node, comparison());
        }
        else if (consume(TK_GREATER)) {
            node = new_node(ND_GREATER, node, comparison());
        }
        else if (consume(TK_LESS_EQ)) {
            node = new_node(ND_LESS_EQ, node, comparison());
        }
        else if (consume(TK_GREATER_EQ)) {
            node = new_node(ND_GREATER_EQ, node, comparison());
        }
        else {    
            return node;
        }
    }

}

// 等価演算子
static Node *equality(void)
{
    Node *node = comparison();

    for (;;) {
        if (consume(TK_EQ)) {
            node = new_node(ND_EQ, node, comparison());
        }
        else if (consume(TK_NEQ)) {
            node = new_node(ND_NEQ, node, comparison());
        }
        else {    
            return node;
        }
    }
}

// ビットAND
static Node *bit_and(void)
{
    Node *node = equality();

    return node;
}

// ビットXOR
static Node *bit_xor(void)
{
    Node *node = bit_and();

    return node;
}

// ビットOR
static Node *bit_or(void)
{
    Node *node = bit_xor();

    return node;
}

// 論理AND
static Node *logical_and(void)
{
    Node *node = bit_or();

    return node;
}

// 論理OR
static Node *logical_or(void)
{
    Node *node = logical_and();

    return node;
}

// 条件演算子
static Node *conditional(void)
{
    Node *node = logical_or();

    return node;
}

// 代入演算子
static Node *assign(void)
{
    Node *node = conditional();

    for (;;) {
        if (consume(TK_ASSIGN)) {
            node = new_node(ND_ASSIGN, node, assign());
        }
        else {    
            return node;
        }
    }
}

// 一つの式
static Node *expr(void)
{
    Node *node = assign();

    return node;
}

// ステートメントノード
static Node *stmt(void)
{
    Node *node;
    
    if (consume(TK_RETURN)) {
        node = new_node_return(expr());
    }
    else {
        node = expr();
    }

    if (!consume(TK_STMT)) {
        error("';'で終わらないトークンです");
    }

    return node;
}

// 複文 / ブロック（{}）
static Node *multi_stmt(void)
{
    Node *node = new_node_block();

    if (!consume(TK_BRACE_OPEN)) {
        error("'{'で始まらないトークンです");   
    }

    for (;;) {
        Token *tk = tokens->data[pos];
        if (tk->ty == TK_BRACE_OPEN) {
            vec_push(node->block_stmts, multi_stmt());
        }
        else if (consume(TK_BRACE_CLOSE)) {
            vec_push(node->block_stmts, NULL);

            return node;
        }
        else {
            vec_push(node->block_stmts, stmt());
        }
    }
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

        vec_push(code, multi_stmt());
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
