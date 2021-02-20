#ifndef SHCC_H_
#define SHCC_H_

#define NUMOF(ary) (sizeof(ary) / sizeof((ary)[0]))

// トークンの型を表す値
typedef enum
{
    TK_INVALID = 0,
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
    TK_ADDR = '&',
    TK_DEREF = '*',
    TK_STMT = ';',

    TK_NUM = 0x100, // 整数
    TK_IDENT,       // 識別子
    TK_INT,         // int
    TK_RETURN,      // return
    TK_IF,          // if
    TK_ELSE,        // else
    TK_FOR,         // for
    TK_WHILE,       // while
    TK_EQ,          // ==
    TK_NEQ,         // !=
    TK_LESS_EQ,     // <=
    TK_GREATER_EQ,  // >=
    TK_ADD_ASSIGN,  // +=
    TK_SUB_ASSIGN,  // -=
    TK_MUL_ASSIGN,  // *=
    TK_DIV_ASSIGN,  // /=
    TK_MOD_ASSIGN,  // %=
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
    ND_VARIABLE,    // 識別子
    ND_VARDEF,      // 変数定義
    ND_CALL,        // 関数呼び出し
    ND_FUNCDEF,     // 関数定義
    ND_BLOCK,       // 複文（{}）
    ND_RETURN,      // return
    ND_IF,          // if
    ND_ELSE,        // else
    ND_FOR,         // for
    ND_WHILE,       // while
    ND_EQ,          // ==
    ND_NEQ,         // !=
    ND_LESS_EQ,     // <=
    ND_GREATER_EQ,  // >=
    ND_ADD_ASSIGN,  // +=
    ND_SUB_ASSIGN,  // -=
    ND_MUL_ASSIGN,  // *=
    ND_DIV_ASSIGN,  // /=
    ND_MOD_ASSIGN,  // %=
    ND_ADDR,        // &
    ND_DEREF,       // *

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
    int value;      // TK_NUMなら数値
    char *input;    // トークン文字列
} Token;

// 変数
typedef struct VariableInfo
{
    const char *name; // 変数名
    int offset;       // RBPからのオフセット
} VariableInfo;

// 関数
typedef struct FuncInfo
{
    const char *name;  // 関数名
    struct Node *body; // ND_FUNCDEFの定義となるブロック
    Vector *args;      // 引数
    int stack_size;    // この関数が最大で使用するスタックサイズ
} FuncInfo;

// 抽象構文木ノード
typedef struct Node
{
    NodeType_t ty;
    struct Node *lhs;         // 二項演算子の左辺、または単項演算子の被演算子
    struct Node *rhs;         // 二項演算子の右辺
    int value;                // ND_NUMの場合の数値
    Vector *block_stmts;      // ND_BLOCKを構成する式群
    struct Node *condition;   // if / for / while の条件式
    struct Node *then;        // if / for / while で条件を満たすときに実行される文
    struct Node *elsethen;    // if-else で条件を満たさないときに実行される文
    struct Node *initializer; // for の初期化処理
    struct Node *loopexpr;    // for のループ終了時の処理
    FuncInfo *func;           // 関数情報
    VariableInfo *variable;   // 型情報
} Node;

Vector *new_vector(void);
void vec_push(Vector *vec, void *elem);

Map *new_map(void);
void map_put(Map *map, const char *key, void *val);
void map_puti(Map *map, const char *key, int val);
void *map_get(const Map *map, const char *key);
int map_geti(const Map *map, const char *key);

Vector *tokenize(char *p);

Vector *program(Vector *token_list);

void gen_asm(Vector *code);

// ダンプ関係
void initialize_dump_env(void);
void dump_token_list(Vector *token_list);
void dump_node_list(Vector *code);

#endif // ifndef SHCC_H_