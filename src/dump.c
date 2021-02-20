#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

#include "shcc.h"

// トークンタイプのうち文字と対応していない列挙値の変換用
static char *token_map[0x200] = {0};

// トークン一覧の出力
void dump_token_list(Vector *token_list)
{
    printf("# tokens: ");
    for (int i = 0; i < token_list->len; i++)
    {
        Token *tk = token_list->data[i];

        if (token_map[tk->ty] != 0)
        {
            printf("%s", token_map[tk->ty]);
        }
        else
        {
            printf("%c", tk->ty);
        }

        printf(" ");
    }

    printf("\n");
}

// ノードタイプのうち文字と対応していない列挙値の変換用
static char *node_map[0x200] = {0};

// ノードタイプのテキスト出力
static void dump_node_type(NodeType_t ty)
{
    if (node_map[ty] != 0)
    {
        printf("%s ", node_map[ty]);
    }
    else
    {
        printf("%c ", ty);
    }
}

// block内のアセンブリ出力
static void dump_node_block(Vector *block_stmts)
{
    for (int j = 0; block_stmts->data[j]; j++)
    {
        Node *node = (Node *)block_stmts->data[j];

        if (node->ty == ND_BLOCK)
        {
            dump_node_block((Vector *)node->block_stmts);
        }
        else
        {
            dump_node_type(node->ty);
        }
    }
}

// ノードの関数とその1改装下のノード一覧
void dump_node_list(Vector *code)
{
    printf("# nodes: ");
    // 1ループ1関数定義
    for (int i = 0; code->data[i]; i++)
    {
        Node *node = code->data[i];
        dump_node_type(node->ty);

        if (node->ty == ND_FUNCDEF)
        {
            dump_node_block(node->func->body->block_stmts);
        }
    }

    puts("");
}

// ダンプ用の環境初期化
void initialize_dump_env(void)
{
    // トークン用のマップ
    {
        token_map[TK_NUM] = "Num";
        token_map[TK_IDENT] = "Ident";
        token_map[TK_RETURN] = "Ret";
        token_map[TK_IF] = "If";
        token_map[TK_ELSE] = "Else";
        token_map[TK_EQ] = "==";
        token_map[TK_NEQ] = "!=";
        token_map[TK_LESS_EQ] = "<=";
        token_map[TK_GREATER_EQ] = ">=";
        token_map[TK_EOF] = "EOF";
    }

    // ノード用のマップ
    {
        node_map[ND_NUM] = "Num";
        node_map[ND_VARIABLE] = "Ident";
        node_map[ND_RETURN] = "Ret";
        node_map[ND_IF] = "If";
        node_map[ND_ELSE] = "Else";
        node_map[ND_EQ] = "==";
        node_map[ND_NEQ] = "!=";
        node_map[ND_LESS_EQ] = "<=";
        node_map[ND_GREATER_EQ] = ">=";
        node_map[ND_CALL] = "Call";
        node_map[ND_FUNCDEF] = "FnDef";
        node_map[ND_BLOCK] = "Blk";
    }
}