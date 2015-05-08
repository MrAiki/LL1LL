#include "generate.h"

/* FIXME:長さは可変にしよう */
#define MAX_CODE_SIZE (500) /* コードの最大長 */

/* 命令の構造体 */
typedef struct {
  LVM_OpCode opcode;    /* オペコード */
  union {               /* オペランド */
    RelAddr address;    /* アドレス */
    LL1LL_Value value;  /* 即値 */
  } u;
} LVM_Instruction;

static LVM_Instruction code[MAX_CODE_SIZE]; /* 命令コードの配列 */
static int current_code_size = -1;          /* 現在のコードサイズ */

static void checkCodeSize(void);            /* コードサイズの確認と, コードサイズ増加 TODO:ここでrealloc */


