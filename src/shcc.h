#ifndef SHCC_H_
#define SHCC_H_

#define NUMOF(ary) (sizeof(ary) / sizeof((ary)[0]))

// トークンの型を表す値
typedef enum
{
    TK_PLUS = '+',
    TK_MINUS = '-',
    TK_MUL = '*',
    TK_DIV = '/',
    TK_MOD = '%',
    TK_PROPEN = '(',
    TK_PRCLOSE = ')',
    TK_ASSIGN = '=',
    TK_LESS = '<',
    TK_GREATER = '>',
    TK_BRACE_OPEN = '{',
    TK_BRACE_CLOSE = '}',
    TK_STMT = ';',

    TK_NUM = 0x100, // 整数
    TK_IDENT,       // 識別子
    TK_RETURN,      // return
    TK_EQ,          // ==
    TK_NEQ,         // !=
    TK_LESS_EQ,     // <=
    TK_GREATER_EQ,  // >=
    TK_EOF,         // 終端
} TokenType_t;

// ノードの型を表す値
typedef enum
{
    ND_PLUS = '+',
    ND_MINUS = '-',
    ND_MUL = '*',
    ND_DIV = '/',
    ND_MOD = '%',
    ND_ASSIGN = '=',
    ND_LESS = '<',
    ND_GREATER = '>',
    ND_BRACE_OPEN = '{',
    ND_BRACE_CLOSE = '}',
    ND_STMT = ';',

    ND_NUM = 0x100, // 整数
    ND_IDENT,       // 識別子
    ND_CALL,        // 関数呼び出し
    ND_FUNCTION,    // 関数定義
    ND_BLOCK,       // 複文（{}）
    ND_RETURN,      // return
    ND_EQ,          // ==
    ND_NEQ,         // !=
    ND_LESS_EQ,     // <=
    ND_GREATER_EQ,  // >=

} NodeType_t;

// ノード用ベクター
typedef struct Vector
{
    void **data;
    int capacity;
    int len;
} Vector;

// 連想配列用マップ
typedef struct
{
    Vector *keys;
    Vector *vals;
} Map;

// トークンの型
typedef struct
{
    TokenType_t ty; // トークンの型
    int val;        // TK_NUMなら数値
    char *input;    // トークン文字列
} Token;

// 抽象構文木ノード
typedef struct Node
{
    NodeType_t ty;
    struct Node *lhs;
    struct Node *rhs;
    int val;                 // ND_NUMの場合の数値
    const char *name;        // ND_IDENTの場合の名前
    Vector *args;            // ND_CALLの引数
    Vector *block_stmts;     // ND_BLOCKを構成する式群
    struct Node *func_block; // ND_FUNCTIONの定義となるブロック
} Node;

Vector *new_vector(void);
void vec_push(Vector *vec, void *elem);
void vec_push_token(Vector *vec, TokenType_t ty, int val, char *input);

Map *new_map(void);
void map_put(Map *map, const char *key, void *val);
void map_puti(Map *map, const char *key, int val);
void *map_get(const Map *map, const char *key);
int map_geti(const Map *map, const char *key);

Vector *tokenize(char *p);
void dump_token_list(Vector *token_list);

Vector *program(Vector *token_list);

void gen_asm(Vector *code);

#endif // ifndef SHCC_H_