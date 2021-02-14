#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "shcc.h"

void runtest(void);

// main
int main(int argc, char **argv)
{
    bool needs_dump_token_list = false;
    bool needs_dump_node_list = false;
    char *source_code = NULL;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[1], "-test") == 0)
        {
            runtest();
            return 0;
        }
        else if (strcmp(argv[i], "-dumptoken") == 0)
        {
            needs_dump_token_list = true;
        }
        else if (strcmp(argv[i], "-dumpnode") == 0)
        {
            needs_dump_node_list = true;
        }
        else if (source_code == NULL)
        {
            source_code = argv[i];
        }
        else
        {
            // コードに相当する引数が複数ある（未定義のコマンド）
            fprintf(stderr, "引数が正しくありません\n");
            return 1;
        }
    }

    // ソースコードが見つからなかった
    if (source_code == NULL)
    {
        fprintf(stderr, "引数が正しくありません\n");
        return 1;
    }

    if (needs_dump_token_list || needs_dump_node_list)
    {
        initialize_dump_env();
    }

    // トークナイズ
    Vector *tokens = tokenize(source_code);
    if (needs_dump_token_list)
    {
        dump_token_list(tokens);
    }

    // パース
    Vector *code = program(tokens);
    if (needs_dump_node_list)
    {
        dump_node_list(code);
    }

    // アセンブリ出力
    gen_asm(code);

    return 0;
}
