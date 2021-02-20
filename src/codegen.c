#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

#include "shcc.h"

static void gen_asm_expr(Node *node);
static void gen_asm_stmt(Node *node);

static Map *vars = 0;
static int stack_offset = 0;
// 条件分岐などで連番を作成するために使用する
static int global_label_no = 0;

static const int stack_unit = 8;

// 引数に使うレジスタ
static const char *arg_regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

// プロローグアセンブリ出力
void gen_asm_prologue(void)
{
    // アセンブリ 前半出力
    printf(".intel_syntax noprefix\n");
    printf("# body start\n");
}

// エピローグアセンブリ出力
void gen_asm_epilog(void)
{
    printf("# body end\n");
}

// 関数プロローグ
static void gen_asm_func_head(Node *func)
{
    puts("");
    printf(".global %s\n", func->name);
    printf("%s:\n", func->name);

    // 呼び出し元のベースポインタを保存
    printf("  # function prologue begin\n");
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");

    // pushするとスタックポインタをずらす->格納としてくれるので意識しなくてよいが
    // prologueではRSPを手動でずらすことになる
    // RSPは使用済みのスタックを指しているので、上書きしないように少なくとも1単位はずらす必要がある
    stack_offset += stack_unit;

    // 引数の個数チェック
    assert(func->args->len <= NUMOF(arg_regs));

    // 引数をスタックに展開
    for (int i = 0; i < func->args->len; i++)
    {
        map_puti(vars, func->args->data[i], stack_offset);

        // ベースポインタとのオフセットを算出し、引数レジスタの値をオフセット位置へ格納する
        printf("  mov rax, rbp\n");
        printf("  sub rax, %d\n", stack_offset);
        printf("  mov [rax], %s\n", arg_regs[i]);

        stack_offset += stack_unit;
    }

    printf("  sub rsp, %d\t\t# stack evacuation\n", stack_offset); // スタック待避
    printf("  # function prologue end\n");
    puts("");
}

// 関数エピローグ
static void gen_asm_func_tail(void)
{
    // 呼び出し元のベースポインタを復帰
    printf("  # function epilog begin\n");
    printf("  leave\n");
    // 下記はleaveと等価なコード
    // printf("  mov rsp, rbp\n");
    // printf("  pop rbp\n");
    printf("  ret\n");
    printf("  # function epilog end\n");
    puts("");
}

// ノード解析失敗エラー
static void error(const char *msg)
{
    fprintf(stderr, "%s\n", msg);

    exit(1);
}

// 左辺値のアセンブリ出力
// 該当アドレスをpush
static void gen_asm_lval(Node *node)
{
    if (node->ty != ND_IDENT)
    {
        error("代入の左辺値が変数ではありません");
    }

    int *v = (int *)map_get(vars, node->name);
    if (v == NULL)
    {
        char *msg = calloc(256, sizeof(char));
        snprintf(msg, 256, "未定義の変数'%s'です", node->name);
        error(msg);
    }

    int offset = *v;
    printf("  #%s = [RBP-%d]\n", node->name, offset);
    printf("  mov rax, rbp\n");
    printf("  sub rax, %d\n", offset);
    printf("  push rax\t\t# var addr\n");
}

// 変数を定義する
//  ≒ 変数用のスタックを確保する
static void gen_asm_vardef(Node *node)
{
    if (node->ty != ND_VARDEF)
    {
        error("変数定義ではありません");
    }

    int offset = stack_offset;
    map_puti(vars, node->name, stack_offset);

    printf("  # New variable '%s' = [RBP-%d]\n", node->name, offset);
    printf("  sub rsp, %d\n", stack_unit); // スタック待避
    stack_offset += stack_unit;
}

// 式のアセンブリ出力
// 式の結果をpush
static void gen_asm_expr(Node *node)
{
    switch (node->ty)
    {
    case ND_NUM:
    {
        printf("  push %d\n", node->val);
        return;
    }
    case ND_CALL:
    {
        // 関数呼び出し
        // 今はとりあえず上限までレジスタ格納しておく

        // 引数の個数チェック
        assert(node->args->len <= NUMOF(arg_regs));

        // 引数の数
        if (node->args->len > 0)
        {
            printf("  mov rax, %d\n", node->args->len);
        }

        // 一度すべての計算結果をスタックに積む
        for (int i = (int)node->args->len - 1; i >= 0; i--)
        {
            gen_asm_expr(node->args->data[i]);
        }
        // スタックから取り出しながら引数レジスタに格納する
        for (int i = 0; i < node->args->len; i++)
        {
            printf("  pop %s\n", arg_regs[i]);
        }

        int align = stack_offset % 16;
        if (align != 0)
        {
            // 関数呼び出し時のRSPは16Bアラインされている必要がある
            printf("  sub rsp, 8\t\t# 16B align\n");
            stack_offset += 8;
        }

        printf("  call %s\n", node->name);
        if (align != 0)
        {
            // 呼び出し時にアラインのためずらしたRSPを戻す
            printf("  add rsp, 8\t\t# restore 16B align\n");
            stack_offset -= 8;
        }

        // 戻り値
        printf("  push rax\n");
        return;
    }
    case ND_ASSIGN:
    {
        // 代入式なら、必ず左辺は変数
        gen_asm_lval(node->lhs);
        gen_asm_expr(node->rhs);
        printf("  pop rdi\n");
        printf("  pop rax\n");
        printf("  mov [rax], rdi\n");
        printf("  push rdi\n");
        return;
    }
    case ND_IDENT:
    {
        // ここは右辺値の識別子
        // 一度左辺値としてpushした値をpopして使う
        gen_asm_lval(node);
        printf("  pop rax\n");
        printf("  mov rax, [rax]\n");
        printf("  push rax\n");
        return;
    }
    case ND_ADDR:
    {
        gen_asm_lval(node->lhs);
        return;
    }
    case ND_DEREF:
    {
        gen_asm_expr(node->lhs);
        printf("  pop rax\n");
        printf("  mov rax, [rax]\n");
        printf("  push rax\n");
        return;
    }
    default:
    {
        break;
    }
    }

    gen_asm_expr(node->lhs);
    gen_asm_expr(node->rhs);

    printf("  pop rdi\n"); // 右辺の値
    printf("  pop rax\n"); // 左辺の値

    switch (node->ty)
    {
    case ND_PLUS:
    {
        printf("  add rax, rdi\n");
        break;
    }
    case ND_MINUS:
    {
        printf("  sub rax, rdi\n");
        break;
    }
    case ND_MUL:
    {
        printf("  mul rdi\n");
        break;
    }
    case ND_DIV:
    {
        // div命令は rax =  ((rdx << 64) | rax) / rdi
        printf("  mov rdx, 0\n");
        printf("  div rdi\n");
        break;
    }
    case ND_MOD:
    {
        // div命令は rax =  ((rdx << 64) | rax) / rdi
        printf("  mov rdx, 0\n");
        printf("  div rdi\n");
        printf("  mov rax, rdx\n");
        break;
    }

    case ND_EQ:
    {
        printf("  cmp rax, rdi\n");
        printf("  sete al\n");
        printf("  movzb rax, al\n");
        break;
    }
    case ND_NEQ:
    {
        printf("  cmp rax, rdi\n");
        printf("  setne al\n");
        printf("  movzb rax, al\n");
        break;
    }
    case ND_LESS:
    {
        printf("  cmp rax, rdi\n");
        printf("  setl al\n");
        printf("  movzb rax, al\n");
        break;
    }
    case ND_LESS_EQ:
    {
        printf("  cmp rax, rdi\n");
        printf("  setle al\n");
        printf("  movzb rax, al\n");
        break;
    }
    case ND_GREATER:
    {
        printf("  cmp rax, rdi\n");
        printf("  setg al\n");
        printf("  movzb rax, al\n");
        break;
    }
    case ND_GREATER_EQ:
    {
        printf("  cmp rax, rdi\n");
        printf("  setge al\n");
        printf("  movzb rax, al\n");
        break;
    }
    default:
    {
        error("未対応のノード形式です");
        break;
    }
    }

    printf("  push rax\n");
}

// 文のアセンブリ出力
static void gen_asm_stmt(Node *node)
{
    switch (node->ty)
    {
    case ND_BLOCK:
    {
        int len = vars->keys->len;

        for (int j = 0; node->block_stmts->data[j]; j++)
        {
            Node *stmt = (Node *)node->block_stmts->data[j];
            gen_asm_stmt(stmt);
        }

        // 無理矢理ブロック突入時の変数状態に戻す
        vars->keys->len = len;
        vars->vals->len = len;
        return;
    }
    case ND_RETURN:
    {
        gen_asm_expr(node->lhs);
        printf("  pop rax\n");
        gen_asm_func_tail();
        return;
    }
    case ND_IF:
    {
        int label_no = global_label_no++;
        gen_asm_expr(node->condition);
        printf("  pop rax\n");
        printf("  cmp rax, 0\n");

        // elseが無くてもラベルを作っている
        // こちらの方がコードはスマートになる
        printf("  je .Lelse%d\n", label_no);
        gen_asm_stmt(node->then);
        printf("  jmp .Lend%d\n", label_no);
        printf(".Lelse%d:\n", label_no);
        if (node->elsethen)
        {
            gen_asm_stmt(node->elsethen);
        }
        printf(".Lend%d:\n", label_no);
        return;
    }
    case ND_FOR:
    {
        int label_no = global_label_no++;
        if (node->initializer)
        {
            gen_asm_expr(node->initializer);
            // 式の評価結果としてpushされた値が一つあるが、使わないのでここでpopする
            printf("  pop rax\t\t# remove before expr result\n");
        }
        printf(".Lbegin%d:\n", label_no);
        if (node->condition)
        {
            gen_asm_expr(node->condition);
            printf("  pop rax\n");
            printf("  cmp rax, 0\n");
            printf("  je .Lend%d\n", label_no);
        }
        // thenが無いとパースで失敗しているはず
        gen_asm_stmt(node->then);
        if (node->loopexpr)
        {
            gen_asm_expr(node->loopexpr);
            // 式の評価結果としてpushされた値が一つあるが、使わないのでここでpopする
            printf("  pop rax\t\t# remove before expr result\n");
        }
        printf("  jmp .Lbegin%d\n", label_no);
        printf(".Lend%d:\n", label_no);
        return;
    }
    case ND_WHILE:
    {
        int label_no = global_label_no++;
        printf(".Lbegin%d:\n", label_no);
        gen_asm_expr(node->condition);
        printf("  pop rax\n");
        printf("  cmp rax, 0\n");
        printf("  je .Lend%d\n", label_no);
        gen_asm_stmt(node->then);
        printf("  jmp .Lbegin%d\n", label_no);
        printf(".Lend%d:\n", label_no);
        return;
    }
    case ND_VARDEF:
    {
        // 変数の領域確保
        gen_asm_vardef(node);
        return;
    }
    case ND_STMT:
    {
        // 空文なので何もしなくて良いはずだが、何もしないとここがきちんと処理されているか分からないので
        // nopを出力する
        printf("  nop\n");
        return;
    }
    default:
    {
        gen_asm_expr(node);
        // 式の評価結果としてpushされた値が一つあるが、使わないのでここでpopする
        printf("  pop rax\t\t# remove before expr result\n");
        break;
    }
    }
}

// アセンブリ出力
void gen_asm(Vector *code)
{
    // プロローグ
    gen_asm_prologue();

    // 1ループ1関数定義
    for (int i = 0; code->data[i]; i++)
    {
        Node *funcdef = code->data[i];
        assert(funcdef->ty == ND_FUNCDEF);

        // 関数ごとにスタックや変数一覧はクリアされる
        stack_offset = 0;
        vars = new_map();

        gen_asm_func_head(funcdef);
        gen_asm_stmt(funcdef->func_body);
        gen_asm_func_tail();
    }

    // エピローグ
    gen_asm_epilog();
}
