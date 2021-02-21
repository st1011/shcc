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

// 条件分岐などで連番を作成するために使用する
static int global_label_no = 0;

// スタックのpush/popで移動する量
static const int STACK_UNIT = 8;

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
static void gen_asm_func_head(FuncInfo *func)
{
    puts("");
    printf(".text\n");
    printf(".global %s\n", func->name);
    printf("%s:\n", func->name);

    // 呼び出し元のベースポインタを保存
    printf("  # function prologue begin\n");
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");

    // 引数の個数チェック
    assert(func->args->len <= NUMOF(arg_regs));

    // 引数をスタックに展開
    int args_stack = STACK_UNIT;
    for (int i = 0; i < func->args->len; i++)
    {
        // map_puti(vars, func->args->data[i], stack_offset);

        // ベースポインタとのオフセットを算出し、引数レジスタの値をオフセット位置へ格納する
        printf("  mov rax, rbp\n");
        printf("  sub rax, %d\n", args_stack);
        printf("  mov [rax], %s\n", arg_regs[i]);

        args_stack += STACK_UNIT;
    }

    // ここですでにこの関数が使用する最大のスタックサイズが分かるようになった(はず)ので
    // 一括でスタックをずらしておく
    // pushするとスタックポインタをずらす->格納としてくれるので意識しなくてよいが
    // prologueではRSPを手動でずらすことになる
    // RSPは使用済みのスタックを指しているので、上書きしないように少なくとも1単位はずらす必要がある
    int shift_stack_size = STACK_UNIT + func->stack_size;
    // 関数呼び出し時は16Bアラインされている必要があるので、ここで合わせておく
    shift_stack_size = ((shift_stack_size + (16 - 1)) / 16) * 16;

    printf("  sub rsp, %d\t\t# stack evacuation\n", shift_stack_size); // スタック待避
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
static void gen_asm_lval(VariableInfo *variable)
{
    if (variable->is_global)
    {
        printf("  lea rax, %s[rip]\n", variable->name);
        printf("  push rax\t\t# Address('%s')\n", variable->name);
    }
    else
    {
        printf("  mov rax, rbp\n");
        printf("  sub rax, %d\n", variable->offset + STACK_UNIT);
        printf("  push rax\t\t# Address('%s') = [RBP-%d]\n", variable->name, variable->offset + STACK_UNIT);
    }
}

// ローカル変数定義のアセンブリ出力
//  ≒ 変数用のスタックを確保するが、現在は特にすることはない
static void gen_asm_lvardef(VariableInfo *variable)
{
    assert(!variable->is_global);
    printf("  # New variable '%s' = [RBP-%d]\n", variable->name, variable->offset + STACK_UNIT);
    // 関数の先頭で一括スタック待避しているのでここでは実際の操作は行なわない
}

// グローバル変数定義のアセンブリ出力
static void gen_asm_gvardef(VariableInfo *variable)
{
    assert(variable->is_global);

    printf(".global %s\n", variable->name);
    // 今は明示的な初期化のない変数しかないので
    printf(".data\n");
    printf(".align %d\n", STACK_UNIT);
    printf(".size %s, %d\n", variable->name, STACK_UNIT);
    printf("%s:\n", variable->name);
    // とりあえずスタックサイズを確保してゼロクリア
    printf("  .zero %d\n", STACK_UNIT);
    puts("");
}

// 関数呼び出しのアセンブリ出力
static void gen_asm_func_call(FuncInfo *func)
{
    // 関数呼び出し
    // 今はとりあえず上限までレジスタ格納しておく

    // 引数の個数チェック
    assert(func->args->len <= NUMOF(arg_regs));

    // 引数の数
    if (func->args->len > 0)
    {
        printf("  mov rax, %d\n", func->args->len);
    }

    // 一度すべての計算結果をスタックに積む
    for (int i = (int)func->args->len - 1; i >= 0; i--)
    {
        gen_asm_expr(func->args->data[i]);
    }
    // スタックから取り出しながら引数レジスタに格納する
    for (int i = 0; i < func->args->len; i++)
    {
        printf("  pop %s\n", arg_regs[i]);
    }

    printf("  call %s\n", func->name);

    // 戻り値
    printf("  push rax\n");
}

// 式のアセンブリ出力
// 式の結果をpush
static void gen_asm_expr(Node *node)
{
    switch (node->ty)
    {
    case ND_NUM:
    {
        printf("  push %d\n", node->value);
        return;
    }
    case ND_CALL:
    {
        gen_asm_func_call(node->func);
        return;
    }
    case ND_ASSIGN:
    {
        // 代入式なら、必ず左辺は変数
        gen_asm_lval(node->lhs->variable);
        gen_asm_expr(node->rhs);
        printf("  pop rdi\n");
        printf("  pop rax\n");
        printf("  mov [rax], rdi\n");
        printf("  push rdi\n");
        return;
    }
    case ND_VARIABLE:
    {
        // ここは右辺値の識別子
        // 一度左辺値としてpushした値をpopして使う
        gen_asm_lval(node->variable);
        printf("  pop rax\n");
        printf("  mov rax, [rax]\n");
        printf("  push rax\n");
        return;
    }
    case ND_ADDR:
    {
        gen_asm_lval(node->lhs->variable);
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
        for (int j = 0; node->block_stmts->data[j]; j++)
        {
            Node *stmt = (Node *)node->block_stmts->data[j];
            gen_asm_stmt(stmt);
        }
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
        gen_asm_lvardef(node->variable);
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

    // 先にdataセクションだけ書き出してもよいのだけれど、
    // まあ不都合が出てきたら考える
    for (int i = 0; code->data[i]; i++)
    {
        Node *node = code->data[i];

        if (node->ty == ND_FUNCDEF)
        {
            gen_asm_func_head(node->func);
            gen_asm_stmt(node->func->body);
            gen_asm_func_tail();
        }
        else if (node->ty == ND_VARDEF)
        {
            gen_asm_gvardef(node->variable);
        }
        else
        {
            error("グローバル領域には存在しないはずのノードです。");
        }
    }

    // エピローグ
    gen_asm_epilog();
}
