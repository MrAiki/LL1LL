#ifndef LVM_H_INCLUDED
#define LVM_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "share.h"
#include "table.h"

/* 命令語の種類 */
typedef enum {
  /* スタック操作系 */
  LVM_NOP,               /* 何もしない */
  LVM_MOVE_STACK_P,      /* スタックポインタの値をオペランド分だけ動かす */
  /* プッシュ・ポップ */
  LVM_PUSH_IMMEDIATE,   /* オペランドの値をスタックにプッシュ */
  LVM_PUSH_VALUE,       /* 指定したアドレスの値をスタックにプッシュ */
  LVM_POP_VARIABLE,     /* スタックトップの値をアドレスの変数にポップ */
  LVM_POP,              /* スタックトップの値を捨てる */
  LVM_DUPLICATE,        /* スタックトップの値を複製してプッシュ */
  /* ジャンプ */
  LVM_JUMP,             /* オペランドの指すpcへジャンプ */
  LVM_JUMP_IF_TRUE,     /* トップがTRUEならば, トップの値を捨て, pcへジャンプ */
  LVM_JUMP_IF_FALSE,    /* トップがFALSEならば, トップの値を捨て, pcへジャンプ */
  /* 関数呼び出し・リターン */
  LVM_INVOKE,           /* 関数を呼び出す. ディスプレイと戻り先をスタックにプッシュし, pcを関数の先頭番地に書き換える */
  LVM_RETURN,           /* スタックトップの値を戻り値として, 関数から返る. スタックトップとディスプレイを復帰し, pcを関数を呼んだ後の状態にする */
  /* 演算 */
  LVM_MINUS,            /* トップの値の符号を反転 */
  LVM_LOGICAL_NOT,      /* トップの値の論理否定をとる */
  LVM_INCREMENT,        /* トップの値を1増加(整数) */
  LVM_DECREMENT,        /* トップの値を1減少(整数) */
  LVM_ADD,              /* (一つ下)=(一つ下)+(トップ) */
  LVM_SUB,              /* (一つ下)=(一つ下)-(トップ) */
  LVM_MUL,              /* (一つ下)=(一つ下)*(トップ) */
  LVM_DIV,              /* (一つ下)=(一つ下)/(トップ) */
  LVM_MOD,              /* (一つ下)=(一つ下)%(トップ) */
  LVM_POW,              /* (一つ下)=(一つ下)**(トップ) (累乗) */
  /* 論理演算 */
  LVM_LOGICAL_AND,      /* (一つ下)=(一つ下)&&(トップ) */
  LVM_LOGICAL_OR,       /* (一つ下)=(一つ下)||(トップ) */
  /* 比較 */
  LVM_EQUAL,            /* (一つ下) == (トップ) : トップと一つ下の値が等しいかチェックし, 等しければ, 一つ下にTRUE, 等しくなければFALSEをセット */
  LVM_NOT_EQUAL,        /* (一つ下) != (トップ) */
  LVM_GREATER,          /* (一つ下)  > (トップ) */
  LVM_GREATER_EQUAL,    /* (一つ下) >= (トップ) */
  LVM_LESSTHAN,         /* (一つ下)  < (トップ) */
  LVM_LESSTHAN_EQUAL,   /* (一つ下) <= (トップ) */
  /* ストリーム操作系 */
  LVM_POP_TO_STREAM,       /* (一つ下(stream)) << (トップ) */
  LVM_PUSH_FROM_STREAM,      /* TODO:動作未定義 */
} LVM_OpCode;

/* 命令の構造体 */
typedef struct {
  LVM_OpCode opcode;    /* オペコード */
  union {               /* オペランド */
    RelAddr     address;    /* スタック記憶域のアドレス */
    LL1LL_Value value;      /* 即値 */
    int         jump_pc;    /* 飛び先pc */
    int         move_top;   /* スタック移動量 */
  } u;
} LVM_Instruction;

/* 命令生成 返り値は現在のプログラムカウンタ(pc) */
int genCodeValue(LVM_OpCode opcode, LL1LL_Value);     /* オペランドには値. */
int genCodeTable(LVM_OpCode opcode, int table_index); /* オペランドには記号表のインデックス */
int genCodeCalc(LVM_OpCode opcode);                   /* 演算命令の生成 */
int genCodeJump(LVM_OpCode opcode, int jump_pc);      /* jump系命令の生成 */
int genCodeMove(LVM_OpCode opcode, int move_top);    /* トップ移動命令の生成 */
int genCodeReturn(void);                              /* return命令の生成 */
void backPatch(int program_count);                    /* 引数のプログラムカウンタの命令をバックパッチ. 飛び先はこの関数を呼んだ次の命令. */
void changeJumpPc(int pc, int jump_pc);               /* pcのジャンプ命令の飛び先をjump_pcに変更する */
void changeMoveTop(int pc, int move_top);             /* pcのトップ移動量をmove_topに変更する */
int nextCode(void);                                   /* 次のプログラムカウンタを返す */

int getCodeSize(void);                                /* 現在のコードサイズを得る */
LVM_Instruction *getInstruction(void);               /* 命令列のポインタを得る */

/* デバッグ用・pc番目の命令の表示 */
void printCode(int pc);
void printCodeList(void);

#endif /* LVM_H_INCLUDED */
