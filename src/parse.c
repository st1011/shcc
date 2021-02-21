#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "shcc.h"

// スタックというか、レジスタとのやりとりを8B単位でしかやっていないので
// 今はこの単位の変数しか作れない
// そのうち消せるはずなので、今の段階では余り気にしない
const int STACK_UNIT = 8;

typedef struct
{
    Vector *tokens;
    int pos;

    // 変数を管理するベクタ
    // [0]: グローバル変数
    // [1]: ローカル変数
    // 以降、スコープを一つ潜るたびにindexが増え、抜けるたびにindexが減る
    // このベクタはMAPを管理する
    // MAP<Key:変数名, Value:変数情報>
    Vector *variables;
    // RBPからのオフセット
    //   parserにRBPなんて言葉が出てくるのはあんまりよい気はしないが……
    int variable_offset;
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

// 変数情報を格納するインスタンスを生成
static VariableInfo *new_varinfo(VariableType_t ty, bool is_global, const char *name)
{
    VariableInfo *info = calloc(1, sizeof(VariableInfo));
    info->type = ty;
    info->is_global = is_global;
    info->name = name;
    return info;
}

// ローカル変数情報を格納するインスタンスを生成
static VariableInfo *new_local_varinfo(VariableType_t ty, const char *name, int offset)
{
    VariableInfo *info = new_varinfo(ty, false, name);
    info->offset = offset;
    return info;
}

// グローバル変数情報を格納するインスタンスを生成
static VariableInfo *new_global_varinfo(VariableType_t ty, const char *name)
{
    VariableInfo *info = new_varinfo(ty, true, name);
    return info;
}

// ノード生成
static Node *new_node(NodeType_t ty)
{
    Node *node = calloc(1, sizeof(Node));
    node->ty = ty;

    return node;
}

// 単項演算子用のノード生成
static Node *new_node_unary_operator(NodeType_t ty, Node *operand)
{
    Node *node = new_node(ty);
    node->lhs = operand;

    return node;
}

// 二項演算子用のノード生成
static Node *new_node_binary_operator(NodeType_t ty, Node *lhs, Node *rhs)
{
    Node *node = new_node(ty);
    node->lhs = lhs;
    node->rhs = rhs;

    return node;
}

// 数値ノード
static Node *new_node_num(int value)
{
    Node *node = new_node(ND_NUM);
    node->value = value;

    return node;
}

// 単項"-"ノード
static Node *new_node_negative(Node *value)
{
    // 0から引くアセンブリを吐くようにする
    return new_node_binary_operator(ND_MINUS, new_node_num(0), value);
}

// 変数ノード
static Node *new_node_variable(VariableInfo *info)
{
    Node *node = new_node(ND_VARIABLE);
    node->variable = info;
    return node;
}

// 変数定義ノード
static Node *new_node_vardef(VariableInfo *info)
{
    Node *node = new_node(ND_VARDEF);
    node->variable = info;
    return node;
}

// 関数ノード
static Node *new_node_funccall(const char *name)
{
    FuncInfo *info = calloc(1, sizeof(FuncInfo));
    info->name = name;
    info->args = new_vector();

    Node *node = new_node(ND_CALL);
    node->func = info;
    return node;
}

// 関数定義ノード
static Node *new_node_funcdef(const char *name)
{
    FuncInfo *info = calloc(1, sizeof(FuncInfo));
    info->name = name;
    info->args = new_vector();

    Node *node = new_node(ND_FUNCDEF);
    node->func = info;
    return node;
}

// returnノード
static Node *new_node_return(Node *node)
{
    return new_node_unary_operator(ND_RETURN, node);
}

// 空文 ノード
static Node *new_node_empty_stmt(void)
{
    return new_node(ND_STMT);
}

// Block ノード
static Node *new_node_block(void)
{
    Node *node = new_node(ND_BLOCK);
    node->block_stmts = new_vector();

    return node;
}

// if-elseノード
static Node *new_node_ifelse(Node *condition, Node *then, Node *elsethen)
{
    Node *node = new_node(ND_IF);
    node->condition = condition;
    node->then = then;
    node->elsethen = elsethen;
    return node;
}

// forノード
static Node *new_node_for(Node *initializer, Node *condition, Node *loopexpr, Node *then)
{
    Node *node = new_node(ND_FOR);
    node->initializer = initializer;
    node->condition = condition;
    node->loopexpr = loopexpr;
    node->then = then;
    return node;
}

// whileノード
static Node *new_node_while(Node *condition, Node *then)
{
    Node *node = new_node(ND_WHILE);
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

// 変数情報を取得する
static VariableInfo *get_variable_info(Tokens *tks, const char *name)
{
    // よりローカルな方のスコープから探索すればCっぽいスコープ管理になる
    for (int i = tks->variables->len - 1; i >= 0; i--)
    {
        VariableInfo *info = (VariableInfo *)map_get((const Map *)tks->variables->data[i], name);
        if (info != NULL)
        {
            return info;
        }
    }

    return NULL;
}

// 変数を宣言可能か？
static bool can_declaration_variable(Tokens *tks, const char *name)
{
    // 一番ローカルなスコープになければOK
    void *value = map_get((const Map *)tks->variables->data[tks->variables->len - 1], name);

    return value == NULL;
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
        Node *node = new_node_num(tk->value);

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
                vec_push(node->func->args, assign(tks));
            }
        }
        else
        {
            VariableInfo *info = get_variable_info(tks, tk->input);
            if (info == NULL)
            {
                char *msg = calloc(256, sizeof(char));
                snprintf(msg, 256, "未定義の変数'%s'です", tk->input);
                error(tks, msg);
            }

            node = new_node_variable(info);
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
        return new_node_unary_operator(ND_ADDR, monomial(tks));
    }
    else if (consume(tks, TK_DEREF))
    {
        return new_node_unary_operator(ND_DEREF, monomial(tks));
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
            node = new_node_binary_operator(ND_MUL, node, mul(tks));
        }
        else if (consume(tks, TK_DIV))
        {
            node = new_node_binary_operator(ND_DIV, node, mul(tks));
        }
        else if (consume(tks, TK_MOD))
        {
            node = new_node_binary_operator(ND_MOD, node, mul(tks));
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
            node = new_node_binary_operator(ND_PLUS, node, add(tks));
        }
        else if (consume(tks, TK_MINUS))
        {
            node = new_node_binary_operator(ND_MINUS, node, add(tks));
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
            node = new_node_binary_operator(ND_LESS, node, comparison(tks));
        }
        else if (consume(tks, TK_GREATER))
        {
            node = new_node_binary_operator(ND_GREATER, node, comparison(tks));
        }
        else if (consume(tks, TK_LESS_EQ))
        {
            node = new_node_binary_operator(ND_LESS_EQ, node, comparison(tks));
        }
        else if (consume(tks, TK_GREATER_EQ))
        {
            node = new_node_binary_operator(ND_GREATER_EQ, node, comparison(tks));
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
            node = new_node_binary_operator(ND_EQ, node, comparison(tks));
        }
        else if (consume(tks, TK_NEQ))
        {
            node = new_node_binary_operator(ND_NEQ, node, comparison(tks));
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
        node = new_node_binary_operator(ND_ASSIGN, node, assign(tks));
    }
    else if (consume(tks, TK_ADD_ASSIGN))
    {
        // [a += b;] = [a = a + b;]
        Node *expr = new_node_binary_operator(ND_PLUS, node, assign(tks));
        node = new_node_binary_operator(ND_ASSIGN, node, expr);
    }
    else if (consume(tks, TK_SUB_ASSIGN))
    {
        Node *expr = new_node_binary_operator(ND_MINUS, node, assign(tks));
        node = new_node_binary_operator(ND_ASSIGN, node, expr);
    }
    else if (consume(tks, TK_MUL_ASSIGN))
    {
        Node *expr = new_node_binary_operator(ND_MUL, node, assign(tks));
        node = new_node_binary_operator(ND_ASSIGN, node, expr);
    }
    else if (consume(tks, TK_DIV_ASSIGN))
    {
        Node *expr = new_node_binary_operator(ND_DIV, node, assign(tks));
        node = new_node_binary_operator(ND_ASSIGN, node, expr);
    }
    else if (consume(tks, TK_MOD_ASSIGN))
    {
        Node *expr = new_node_binary_operator(ND_MOD, node, assign(tks));
        node = new_node_binary_operator(ND_ASSIGN, node, expr);
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

        if (!can_declaration_variable(tks, tk->input))
        {
            char *msg = calloc(256, sizeof(char));
            snprintf(msg, 256, "定義済みの変数'%s'です", tk->input);
            error(tks, msg);
        }

        VariableInfo *info = new_local_varinfo(VT_INT, tk->input, tks->variable_offset);
        map_put(tks->variables->data[tks->variables->len - 1], info->name, info);
        node = new_node_vardef(info);
        tks->variable_offset += STACK_UNIT;
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
    int current_scope_depth = tks->variables->len;

    // このスコープ用のローカル変数MAPをつくる
    Map *local_variables = new_map();
    vec_push(tks->variables, local_variables);

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

    tks->variables->len = current_scope_depth;
    vec_push(node->block_stmts, NULL);
    return node;
}

// 関数定義
static Node *funcdef(Tokens *tks, const char *name)
{
    int current_scope_depth = tks->variables->len;

    // この関数用のローカル変数MAPをつくる
    Map *local_variables = new_map();
    vec_push(tks->variables, local_variables);

    // 関数ごとに変数のオフセットはクリアする
    tks->variable_offset = 0;

    Node *node = new_node_funcdef(name);

    // 仮引数
    while (!consume(tks, TK_PRCLOSE))
    {
        if (!consume(tks, TK_INT))
        {
            error(tks, "仮引数の型が未定義です。");
        }

        Token *tk = current_token(tks);
        if (!consume(tks, TK_IDENT))
        {
            error(tks, "仮引数の宣言が不正です");
        }

        if (!can_declaration_variable(tks, tk->input))
        {
            char *msg = calloc(256, sizeof(char));
            snprintf(msg, 256, "定義済みの変数'%s'です", tk->input);
            error(tks, msg);
        }

        // うーん、引数もきちんとマッピングしておかないと後々困りそうな……
        VariableInfo *info = new_local_varinfo(VT_INT, tk->input, tks->variable_offset);
        map_put(local_variables, info->name, info);
        tks->variable_offset += STACK_UNIT;
        vec_push(node->func->args, tk->input);
    }

    // 関数定義本体（ブレース内）
    node->func->body = multi_stmt(tks);
    node->func->stack_size = tks->variable_offset;

    tks->variables->len = current_scope_depth;
    return node;
}

// グローバル領域の定義
// 関数と変数
static Node *global(Tokens *tks)
{
    Node *node = NULL;

    if (!consume(tks, TK_INT))
    {
        error(tks, "関数の戻り値または変数の型が未定義です。");
    }

    Token *tk = current_token(tks);
    if (!consume(tks, TK_IDENT))
    {
        error(tks, "関数名か変数名が見つかりません");
    }

    const char *name = tk->input;
    if (consume(tks, TK_PROPEN))
    {
        // 関数っぽい
        node = funcdef(tks, name);
    }
    else if (consume(tks, TK_STMT))
    {
        // 変数っぽい
        if (!can_declaration_variable(tks, name))
        {
            char *msg = calloc(256, sizeof(char));
            snprintf(msg, 256, "定義済みの変数'%s'です", name);
            error(tks, msg);
        }

        VariableInfo *info = new_global_varinfo(VT_INT, name);
        map_put(tks->variables->data[tks->variables->len - 1], info->name, info);
        node = new_node_vardef(info);
        // tks->variable_offset += STACK_UNIT;
    }
    else
    {
        // よく分からない
        error(tks, "グローバルの定義が不正です。");
    }

    return node;
}

// プログラム全体のノード作成
Vector *program(Vector *token_list)
{
    Vector *code = new_vector();
    Vector *variables = new_vector();
    // グローバル変数MAPをつくる
    Map *global_variables = new_map();
    vec_push(variables, global_variables);

    Tokens tokens = {
        .tokens = token_list,
        .pos = 0,
        .variables = variables,
        .variable_offset = 0};

    for (;;)
    {
        if (is_match_next_token(&tokens, TK_EOF))
        {
            break;
        }

        vec_push(code, global(&tokens));
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
