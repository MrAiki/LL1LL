#include "generate.h"

static LVM_Instruction code[MAX_CODE_SIZE]; /* 命令コードの配列 */
static int current_code_size = -1;          /* 現在のコードサイズ */

static void checkCodeSize(void);            /* コードサイズの確認と, コードサイズ増加 TODO:ここでrealloc */

/* 次の命令語のアドレス(pc)を返す */
int nextCode(void)
{
  return current_code_size + 1;
}

/* オペランドに値(即値)をとる命令の生成 */
int genCodeValue(LVM_OpCode opcode, LL1LL_Value value)
{
  checkCodeSize();                          /* コードサイズ検査 */
  code[current_code_size].opcode  = opcode; /* オペコード */
  code[current_code_size].u.value = value;  /* オペランド */
  return current_code_size;                 /* 現在のコードサイズを返す */
}

/* オペランドに（変数）アドレスを取る命令の生成 */
int genCodeTable(LVM_OpCode opcode, int table_index)
{
  checkCodeSize();
  code[current_code_size].opcode    = opcode;
  code[current_code_size].u.address = getVarRelAddr(table_index); /* アドレスの取得 */
  return current_code_size;
}

/* 演算命令の生成 */
int genCodeCalc(LVM_OpCode opcode)
{
  checkCodeSize();
  code[current_code_size].opcode    = opcode;
  return current_code_size;
}

int genCodeReturn(void)
{
}

/* 現在のコードサイズをチェック.
 * 同時にコードサイズを増加 */
static void checkCodeSize(void)
{
  current_code_size++;
  if (current_code_size < MAX_CODE_SIZE) {
    return;
  } else {
    /* TODO:可変にして, メモリの限界まで使えるように */
    fprintf(stderr, "Too many codes... TODO:Code size should be expand \n");
    exit(1);
  }
}

/* バックパッチ. 引数のジャンプ命令の飛び先アドレスに
 * 次の命令アドレスをセットする */
void backPatch(int program_count)
{
  code[program_count].u.jump_pc = current_code_size + 1;
}

/* 命令列の最初の要素へのポインタを得る */
LVM_Instruction *get_instraction(void)
{
  return &(code[0]);
}
