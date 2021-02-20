#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "shcc.h"

typedef struct
{
    Vector *tokens;
    int pos;

    // とりあえず定義確認用に
    Map *variables;
} Tokens;

static Node *expr(Tokens *tks);
static Node *assign(Tokens *tks);
static Node *stmt(Tokens *tks);
static Node *multi_stmt(Tokens *tks);

// 現在のトークンを返す
static Token *current_token(Tokens *tks)
{
    return tks->tokens->data[tks->pos];
}

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
    node->func_body = 0;

    node->condition = 0;
    node->then = 0;
    node->elsethen = 0;
    node->initializer = 0;
    node->loopexpr = 0;

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

// 単項"-"ノード
static Node *new_node_negative(Node *value)
{
    // 0から引くアセンブリを吐くようにする
    return new_node(ND_MINUS, new_node_num(0), value);
}

// 識別子ノード
static Node *new_node_ident(const char *name)
{
    return new_node_body(ND_IDENT, 0, 0, 0, name);
}

// 識別子ノード
static Node *new_node_vardef(const char *name)
{
    return new_node_body(ND_VARDEF, 0, 0, 0, name);
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

// 空文 ノード
static Node *new_node_empty_stmt(void)
{
    return new_node(ND_STMT, 0, 0);
}

// Block ノード
static Node *new_node_block(void)
{
    return new_node(ND_BLOCK, 0, 0);
}

// 関数定義ノード
static Node *new_node_funcdef(const char *name)
{
    return new_node_body(ND_FUNCDEF, 0, 0, 0, name);
}

// if-elseノード
static Node *new_node_ifelse(Node *condition, Node *then, Node *elsethen)
{
    Node *node = new_node(ND_IF, 0, 0);
    node->condition = condition;
    node->then = then;
    node->elsethen = elsethen;
    return node;
}

// forノード
static Node *new_node_for(Node *initializer, Node *condition, Node *loopexpr, Node *then)
{
    Node *node = new_node(ND_FOR, 0, 0);
    node->initializer = initializer;
    node->condition = condition;
    node->loopexpr = loopexpr;
    node->then = then;
    return node;
}

// whileノード
static Node *new_node_while(Node *condition, Node *then)
{
    Node *node = new_node(ND_WHILE, 0, 0);
    node->condition = condition;
    node->then = then;
    return node;
}

// トークン解析失敗エラー
static void error(Tokens *tks, const char *msg)
{
    Token *tk = current_token(tks);
    fprintf(stderr, "%s: %s\n", msg, tk->input);

    exit(1);
}

// 次トークンがtyか確認する
// consumeの先読み版
static bool is_match_next_token(Tokens *tks, int ty)
{
    Token *tk = current_token(tks);

    return tk->ty == ty;
}

// 次トークンがtyか確認し、tyの時のみトークンを一つ進める
static bool consume(Tokens *tks, int ty)
{
    bool is_match = is_match_next_token(tks, ty);
    if (is_match)
    {
        tks->pos++;
    }

    return is_match;
}

// 末尾ノード 括弧か数値
static Node *term(Tokens *tks)
{
    if (consume(tks, TK_PROPEN))
    {
        Node *node = expr(tks);

        if (!consume(tks, TK_PRCLOSE))
        {
            error(tks, "対応する閉じ括弧がありません");
        }

        return node;
    }

    Token *tk = current_token(tks);
    if (consume(tks, TK_NUM))
    {
        Node *node = new_node_num(tk->val);

        return node;
    }
    if (consume(tks, TK_IDENT))
    {
        Node *node = NULL;

        if (consume(tks, TK_PROPEN))
        {
            // 関数呼び出し
            node = new_node_funccall(tk->input);

            // 引数
            while (!consume(tks, TK_PRCLOSE))
            {
                vec_push(node->args, assign(tks));
            }
        }
        else
        {
            // 変数
            if (map_get(tks->variables, tk->input) == NULL)
            {
                char *msg = calloc(256, sizeof(char));
                snprintf(msg, 256, "未定義の変数'%s'です", tk->input);
                error(tks, msg);
            }
            node = new_node_ident(tk->input);
        }

        return node;
    }

    error(tks, "数値でも開き括弧でもないトークンです");

    return 0;
}

// 前置増分/減分, 単項式
// ++ -- ! ~ +-（符号） * & sizeof()
static Node *monomial(Tokens *tks)
{
    if (consume(tks, TK_PLUS))
    {
        return term(tks);
    }
    else if (consume(tks, TK_MINUS))
    {
        Node *node = term(tks);
        return new_node_negative(node);
    }
    else if (consume(tks, TK_ADDR))
    {
        return new_node(ND_ADDR, monomial(tks), NULL);
    }
    else if (consume(tks, TK_DEREF))
    {
        return new_node(ND_DEREF, monomial(tks), NULL);
    }

    return term(tks);
}

// キャスト演算子
static Node *cast(Tokens *tks)
{
    Node *node = monomial(tks);

    return node;
}

// 乗除余演算子
static Node *mul(Tokens *tks)
{
    Node *node = cast(tks);

    for (;;)
    {
        if (consume(tks, TK_MUL))
        {
            node = new_node(ND_MUL, node, mul(tks));
        }
        else if (consume(tks, TK_DIV))
        {
            node = new_node(ND_DIV, node, mul(tks));
        }
        else if (consume(tks, TK_MOD))
        {
            node = new_node(ND_MOD, node, mul(tks));
        }
        else
        {
            return node;
        }
    }
}

// 加減演算子
static Node *add(Tokens *tks)
{
    Node *node = mul(tks);

    for (;;)
    {
        if (consume(tks, TK_PLUS))
        {
            node = new_node(ND_PLUS, node, add(tks));
        }
        else if (consume(tks, TK_MINUS))
        {
            node = new_node(ND_MINUS, node, add(tks));
        }
        else
        {
            return node;
        }
    }
}

// シフト演算子
static Node *shift(Tokens *tks)
{
    Node *node = add(tks);

    return node;
}

// 比較演算子
static Node *comparison(Tokens *tks)
{
    Node *node = shift(tks);

    for (;;)
    {
        if (consume(tks, TK_LESS))
        {
            node = new_node(ND_LESS, node, comparison(tks));
        }
        else if (consume(tks, TK_GREATER))
        {
            node = new_node(ND_GREATER, node, comparison(tks));
        }
        else if (consume(tks, TK_LESS_EQ))
        {
            node = new_node(ND_LESS_EQ, node, comparison(tks));
        }
        else if (consume(tks, TK_GREATER_EQ))
        {
            node = new_node(ND_GREATER_EQ, node, comparison(tks));
        }
        else
        {
            return node;
        }
    }
}

// 等価演算子
static Node *equality(Tokens *tks)
{
    Node *node = comparison(tks);

    for (;;)
    {
        if (consume(tks, TK_EQ))
        {
            node = new_node(ND_EQ, node, comparison(tks));
        }
        else if (consume(tks, TK_NEQ))
        {
            node = new_node(ND_NEQ, node, comparison(tks));
        }
        else
        {
            return node;
        }
    }
}

// ビットAND
static Node *bit_and(Tokens *tks)
{
    Node *node = equality(tks);

    return node;
}

// ビットXOR
static Node *bit_xor(Tokens *tks)
{
    Node *node = bit_and(tks);

    return node;
}

// ビットOR
static Node *bit_or(Tokens *tks)
{
    Node *node = bit_xor(tks);

    return node;
}

// 論理AND
static Node *logical_and(Tokens *tks)
{
    Node *node = bit_or(tks);

    return node;
}

// 論理OR
static Node *logical_or(Tokens *tks)
{
    Node *node = logical_and(tks);

    return node;
}

// 条件演算子
static Node *conditional(Tokens *tks)
{
    Node *node = logical_or(tks);

    return node;
}

// 代入演算子
static Node *assign(Tokens *tks)
{
    Node *node = conditional(tks);
    if (consume(tks, TK_ASSIGN))
    {
        node = new_node(ND_ASSIGN, node, assign(tks));
    }
    else if (consume(tks, TK_ADD_ASSIGN))
    {
        // [a += b;] = [a = a + b;]
        Node *expr = new_node(ND_PLUS, node, assign(tks));
        node = new_node(ND_ASSIGN, node, expr);
    }
    else if (consume(tks, TK_SUB_ASSIGN))
    {
        Node *expr = new_node(ND_MINUS, node, assign(tks));
        node = new_node(ND_ASSIGN, node, expr);
    }
    else if (consume(tks, TK_MUL_ASSIGN))
    {
        Node *expr = new_node(ND_MUL, node, assign(tks));
        node = new_node(ND_ASSIGN, node, expr);
    }
    else if (consume(tks, TK_DIV_ASSIGN))
    {
        Node *expr = new_node(ND_DIV, node, assign(tks));
        node = new_node(ND_ASSIGN, node, expr);
    }
    else if (consume(tks, TK_MOD_ASSIGN))
    {
        Node *expr = new_node(ND_MOD, node, assign(tks));
        node = new_node(ND_ASSIGN, node, expr);
    }

    return node;
}

// 一つの式
static Node *expr(Tokens *tks)
{
    Node *node = assign(tks);

    return node;
}

// if文
static Node *stmt_if(Tokens *tks)
{
    // ここに来る時点ではIFトークンは消費済み

    if (!consume(tks, TK_PROPEN))
    {
        error(tks, "if文には'('が必要です");
    }

    Node *condition = expr(tks);

    if (!consume(tks, TK_PRCLOSE))
    {
        error(tks, "if文には')'が必要です");
    }

    Node *then = stmt(tks);

    Node *elsethen = NULL;
    if (consume(tks, TK_ELSE))
    {
        elsethen = stmt(tks);
    }

    return new_node_ifelse(condition, then, elsethen);
}

// for文
static Node *stmt_for(Tokens *tks)
{
    // ここに来る時点ではFORトークンは消費済み

    if (!consume(tks, TK_PROPEN))
    {
        error(tks, "for文には'('が必要です");
    }

    Node *initializer = NULL;
    Node *condition = NULL;
    Node *loopexpr = NULL;
    if (!is_match_next_token(tks, TK_STMT))
    {
        initializer = expr(tks);
    }
    if (!consume(tks, TK_STMT))
    {
        error(tks, "for文には';'が必要です");
    }

    if (!is_match_next_token(tks, TK_STMT))
    {
        condition = expr(tks);
    }
    if (!consume(tks, TK_STMT))
    {
        error(tks, "for文には';'が必要です");
    }

    if (!is_match_next_token(tks, TK_PRCLOSE))
    {
        loopexpr = expr(tks);
    }

    if (!consume(tks, TK_PRCLOSE))
    {
        error(tks, "for文には')'が必要です");
    }

    Node *then = stmt(tks);

    return new_node_for(initializer, condition, loopexpr, then);
}

// while文
static Node *stmt_while(Tokens *tks)
{
    // ここに来る時点ではWHILEトークンは消費済み

    if (!consume(tks, TK_PROPEN))
    {
        error(tks, "while文には'('が必要です");
    }

    Node *condition = expr(tks);

    if (!consume(tks, TK_PRCLOSE))
    {
        error(tks, "while文には')'が必要です");
    }

    Node *then = stmt(tks);

    return new_node_while(condition, then);
}

// ステートメントノード
static Node *stmt(Tokens *tks)
{
    if (is_match_next_token(tks, TK_BRACE_OPEN))
    {
        return multi_stmt(tks);
    }

    Node *node;
    if (consume(tks, TK_RETURN))
    {
        node = new_node_return(expr(tks));
    }
    else if (consume(tks, TK_IF))
    {
        node = stmt_if(tks);
        return node;
    }
    else if (consume(tks, TK_FOR))
    {
        node = stmt_for(tks);
        return node;
    }
    else if (consume(tks, TK_WHILE))
    {
        node = stmt_while(tks);
        return node;
    }
    else if (consume(tks, TK_STMT))
    {
        return new_node_empty_stmt();
    }
    else if (consume(tks, TK_INT))
    {
        Token *tk = current_token(tks);

        // 変数定義
        if (!consume(tks, TK_IDENT))
        {
            error(tks, "変数名の必要があります。");
        }

        node = new_node_vardef(tk->input);
        map_puti(tks->variables, tk->input, sizeof(int));
    }
    else
    {
        node = expr(tks);
    }

    if (!consume(tks, TK_STMT))
    {
        error(tks, "';'で終わらないトークンです");
    }

    return node;
}

// 複文 / ブロック（{}）
static Node *multi_stmt(Tokens *tks)
{
    Node *node = new_node_block();

    if (!consume(tks, TK_BRACE_OPEN))
    {
        error(tks, "'{'で始まらないトークンです");
    }

    for (;;)
    {
        if (consume(tks, TK_BRACE_CLOSE))
        {
            break;
        }
        else if (is_match_next_token(tks, TK_EOF))
        {
            error(tks, "'}'で終わらないトークンです");
        }
        else
        {
            vec_push(node->block_stmts, stmt(tks));
        }
    }

    vec_push(node->block_stmts, NULL);
    return node;
}

// 関数定義
static Node *funcdef(Tokens *tks)
{
    Token *tk = current_token(tks);

    if (!consume(tks, TK_INT))
    {
        error(tks, "関数の戻り値が未定義です。");
    }

    tk = current_token(tks);
    if (!consume(tks, TK_IDENT))
    {
        error(tks, "top階層に関数が見つかりません");
    }

    Node *node = new_node_funcdef(tk->input);

    if (!consume(tks, TK_PROPEN))
    {
        error(tks, "top階層に関数が見つかりません");
    }

    // 仮引数
    while (!consume(tks, TK_PRCLOSE))
    {
        if (!consume(tks, TK_INT))
        {
            error(tks, "仮引数の型が未定義です。");
        }

        tk = current_token(tks);
        if (!consume(tks, TK_IDENT))
        {
            error(tks, "仮引数の宣言が不正です");
        }
        // TODO そろそろスコープを真面目に考えるときが……
        map_puti(tks->variables, tk->input, sizeof(int));
        vec_push(node->args, tk->input);
    }
    // 関数定義本体（ブレース内）
    node->func_body = multi_stmt(tks);

    return node;
}

// プログラム全体のノード作成
Vector *program(Vector *token_list)
{
    Vector *code = new_vector();
    Map *variables = new_map();

    Tokens tokens = {
        .tokens = token_list,
        .pos = 0,
        .variables = variables};

    for (;;)
    {
        if (is_match_next_token(&tokens, TK_EOF))
        {
            break;
        }

        vec_push(code, funcdef(&tokens));
    }

    vec_push(code, NULL);

    return code;
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
