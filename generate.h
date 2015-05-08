#ifndef LVM_H_INCLUDED
#define LVM_H_INCLUDED

#include <stdio.h>
#include <string.h>

#include "share.h"
#include "table.h"

typedef enum {
  /* プッシュ・ポップ */
  LVM_PUSH_IMMEDIATE,   /* オペランドの値をスタックにプッシュ */
  LVM_PUSH_VALUE,       /* 指定したアドレスの値をスタックにプッシュ */
  LVM_POP_VARIABLE,     /* スタックトップの値をアドレスの変数にポップ */
  LVM_POP,              /* スタックトップの値を捨てる */
  LVM_DUPLICATE,        /* スタックトップの値を複製してプッシュ */
  LVM_INC_STACK_P,      /* スタックポインタの値をオペランド分だけ動かす */
  /* ジャンプ */
  LVM_JUMP,             /* オペランドの指すpcへジャンプ */
  LVM_JUMP_IF_TRUE,     /* トップがTRUEならば, トップの値を捨て, pcへジャンプ */
  LVM_JUMP_IF_FALSE,    /* トップがTRUEならば, トップの値を捨て, pcへジャンプ */
  /* 関数呼び出し・リターン */
  LVM_INVOKE,           /* 関数を呼び出す. ディスプレイと戻り先をスタックにプッシュし, pcを関数の先頭番地に書き換える */
  LVM_RETURN,           /* スタックトップの値を戻り値として, 関数から返る. スタックトップとディスプレイを復帰し, pcを関数を呼んだ後の状態にする */
  /* 演算 */
  LVM_MINUS,            /* トップの値の符号を反転 */
  LVM_LOGICAL_NOT,      /* トップの値の論理否定をとる */
  LVM_ADD,              /* (一つ下)=(一つ下)+(トップ) */
  LVM_SUB,              /* (一つ下)=(一つ下)-(トップ) */
  LVM_MUL,              /* (一つ下)=(一つ下)*(トップ) */
  LVM_DIV,              /* (一つ下)=(一つ下)/(トップ) */
  LVM_MOD,              /* (一つ下)=(一つ下)%(トップ) */
  LVM_INCREMENT,        /* トップの値を1増加(整数) */
  LVM_DECREMENT,        /* トップの値を1減少(整数) */
  /* 比較 */
  LVM_EQUAL,            /* (一つ下) == (トップ) : トップと一つ下の値が等しいかチェックし, 等しければ, 一つ下にTRUE, 等しくなければFALSEをセット */
  LVM_NOT_EQUAL,        /* (一つ下) != (トップ) */
  LVM_GREATER,          /* (一つ下)  > (トップ) */
  LVM_GREATER_EQUAL,    /* (一つ下) >= (トップ) */
  LVM_LESSTHAN,         /* (一つ下)  < (トップ) */
  LVM_LESSTHAN_EQUAL,   /* (一つ下) <= (トップ) */
} LVM_OpCode;

/* 命令生成 返り値は現在のプログラムカウンタ(pc) */
int genCodeValue(LVM_OpCode opcode, LL1LL_Value);     /* オペランドには値. */
int genCodeTable(LVM_OpCode opcode, int table_index); /* オペランドには記号表のインデックス */
int genCodeCalc(LVM_OpCode opcode);                   /* 演算命令の生成 */
int genCodeReturn(void);                              /* return命令の生成 */
void backPatch(int program_count);                    /* 引数のプログラムカウンタの命令をバックパッチ. 飛び先はこの関数を呼んだ次の命令. */
int nextCode(void);                                   /* 次のプログラムカウンタを返す */

void execute(void);                                   /* 仮想言語を実行 */

#endif /* LVM_H_INCLUDED */
