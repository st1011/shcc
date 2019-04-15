#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "shcc.h"

// 新規vector作成
Vector *new_vector(void)
{
    Vector *vec = calloc(1, sizeof(Vector));

    vec->data = calloc(16, sizeof(void *));
    vec->capacity = 16;
    vec->len = 0;

    return vec;
}

// vectorへの要素追加
// 空きがなければ2倍に拡張する
void vec_push(Vector *vec, void *elem)
{
    if (vec->capacity == vec->len) {
        vec->capacity *= 2;
        vec->data = realloc(vec->data, sizeof(void *) * vec->capacity);
    }

    vec->data[vec->len++] = elem;
}

// vectorにtokenを追加
void vec_push_token(Vector *vec, TokenType_t ty, int val, char *input)
{
    Token *tk = (Token *)calloc(1, sizeof(Token));

    tk->ty = ty;
    tk->val = val;
    tk->input = input;

    vec_push(vec, tk);
}

// 新しいマップの作成
Map *new_map(void)
{
    Map *map = calloc(1, sizeof(Map));

    map->keys = new_vector();
    map->vals = new_vector();

    return map;
}

// マップへの値追加
void map_put(Map *map, const char *key, void *val)
{
    vec_push(map->keys, (char *)key);
    vec_push(map->vals, val);
}

// マップへの整数値追加
// 一応メモリを割り当てる
void map_puti(Map *map, const char *key, int val)
{
    int *v = (int *)calloc(1, sizeof(int));

    *v = val;
    map_put(map, key, v);
}

// マップからの値取得
void *map_get(const Map *map, const char *key)
{
    // 末尾から探すので、古いデータを上書きする必要が無い
    for (int i = map->keys->len - 1; i >= 0; i--) {
        if (strcmp(map->keys->data[i], key) == 0) {
            return map->vals->data[i];
        }
    }

    return NULL;
}

// マップからの値取得
int map_geti(const Map *map, const char *key)
{
    int *v = map_get(map, key);

    if (v != NULL) {
        return *v;
    }

    return 0;
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

#define EXPECT(exp, act)        expect(__LINE__, (exp), (act))

// vector関係のテスト
static void test_vector(void)
{
    Vector *vec = new_vector();
    EXPECT(0, vec->len);

    for (int i = 0; i < 100; i++) {
        vec_push(vec, (void *)(intptr_t)i);
    }

    EXPECT(100, vec->len);
    EXPECT(0, (intptr_t)vec->data[0]);
    EXPECT(50, (intptr_t)vec->data[50]);
    EXPECT(99, (intptr_t)vec->data[99]);
}

// map関係のテスト
static void test_map(void)
{
    Map *map = new_map();
    EXPECT(0, (intptr_t)map_get(map, "foo"));

    map_put(map, "foo", (void *)(intptr_t)2);
    EXPECT(2, (intptr_t)map_get(map, "foo"));

    map_put(map, "bar", (void *)(intptr_t)4);
    EXPECT(4, (intptr_t)map_get(map, "bar"));
    EXPECT(2, (intptr_t)map_get(map, "foo"));

    map_put(map, "foo", (void *)(intptr_t)6);
    EXPECT(6, (intptr_t)map_get(map, "foo"));

    map_puti(map, "foobar", 8);
    EXPECT(8, map_geti(map, "foobar"));

}

// vector用テスト
void runtest(void)
{
    test_vector();
    test_map();

    printf("    runtest OK\n");
}
