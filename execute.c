#include "execute.h"

static LL1LL_Value stack[MAX_EXE_STACK_SIZE]; /* 実行時スタック */
static int top;                               /* スタックトップ(プッシュ・ポップの対象となるスタックのインデックス) */

/* スタックトップを得る */
int get_stack_top(void)
{
  return top;
}

/* スタックを指すポインタを得る */
LL1LL_Value *get_stack_pointer(void)
{
  return &(stack[0]);
}

/* 命令列の実行 */
void execute(void)
{
  int pc; /* プログラムカウンタ */
  LVM_Instruction *code = get_instraction();  /* 命令列 */

}
