#ifndef SHCC_H_
#define SHCC_H_

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
    TK_RETURN,          // return
    TK_EOF,             // 終端
} TokenType_t;

// ノードの型を表す値
typedef enum {
    ND_PLUS = '+',
    ND_MINUS = '-',
    ND_MUL = '*',
    ND_DIV = '/',
    ND_EQ = '=',
    ND_STMT = ';',

    ND_NUM = 0x100, // 整数のノードの型
    ND_IDENT,       // 識別子のノードの型
    ND_RETURN,      // returnのノードの型
} NodeType_t;

// トークンの型
typedef struct {
    TokenType_t ty;         // 型
    int val;        // TK_NUMなら数値
    char *input;    // トークン文字列（エラーメッセージ用）
} Token;

// 抽象構文木ノード
typedef struct Node {
    NodeType_t ty;
    struct Node *lhs;
    struct Node *rhs;
    int val;                // ND_NUMの場合の数値
    char name;              // ND_IDENTの場合の名前
} Node;

// ノード用ベクター
typedef struct Vector {
    void **data;
    int capacity;
    int len;
} Vector;

// 連想配列用マップ
typedef struct {
    Vector *keys;
    Vector *vals;
} Map;


extern Vector *tokens;
extern Node *code[128];

Vector *new_vector(void);
void vec_push_token(Vector *vec, TokenType_t ty, int val, char *input);

Map *new_map(void);
void map_put(Map *map, char *key, void *val);
void *map_get(Map *map, char *key);

struct Vector *tokenize(char *p);

void program(void);

void gen_asm(Node *node);
void gen_asm_epilog(void);
void exit_with_asm(void);

#endif // ifndef SHCC_H_