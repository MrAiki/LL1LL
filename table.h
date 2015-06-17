#ifndef TABLE_H_INCLUDED
#define TABLE_H_INCLUDED

#include <stdio.h>
#include <string.h>

#include "error.h"
#include "MEM.h"

#define MAX_TABLE_SIZE  (200)  /* 名前表の最大長 FIXME:メモリの限界まで伸ばせるように */

/* スタック型の記憶域を管理するモジュール群 */

/* 識別子の種類 */
typedef enum {
  VAR_IDENTIFIER,   /* 変数の識別子 */
  FUNC_IDENTIFIER,  /* 関数の識別子 */
  CONST_IDENTIFIER, /* 定数の識別子 */
  PARAM_IDENTIFIER, /* 関数の仮引数の識別子 */
} IdentifierKind;

/* ブロックの種類 */
typedef enum {
  FUNCTION_BLOCK, /* 関数ブロック */
  LOOP_BLOCK,     /* ループ(for, while, do while)ブロック */
  TOPLEVEL,       /* トップレベル */
  NORMAL_BLOCK,   /* 文に平書きされたブロック */
} BlockKind;

/* 変数, 仮引数, 関数の, スタック型記憶域のアドレス */
typedef struct {
  int block_level;  /* ブロックのレベル */
  int address;      /* ブロック内でのアドレス(変数,定数),もしくは処理内容の先頭pc(関数) */
} RelAddr;

/* 公開モジュール群 */

/* 名前表に名前を登録. 名前表追加系関数では必ず呼ばれ, 名前表エントリ数の増加も暗黙で行う */
void addTableName(char *identifier);  
/* 名前表登録ルーチン. 返り値は現在のエントリ数 */
int addTableFunc(char *identifier, int address); /* 名前表に関数を登録. 先頭pcを与える */
int addTableParam(char *identifier); /* 名前表に仮引数を登録. */
int addTableVar(char *identifier);                  /* 名前表に変数を登録 */
int addTableConst(char *identifier, LL1LL_Value v); /* 名前表に定数と, その値を登録 */
 
/* 関数のアドレス仕上げ関係 */
void parEnd(void);  /* 関数の仮引数の登録完了時に呼ぶ. 仮引数にアドレスを割り当てる */
void changeFuncAddr(int table_index, int new_address); /* 名前表table_indexの, 関数の先頭アドレスの変更 */

/* 名前表から, 名前identifier, 種類kindのエントリを探し, あればそのインデックスを返す. なければ0を返すが, 変数の探索の場合は名前表にエントリが追加される. */
int searchTable(char *identifier, IdentifierKind kind);

/* 名前表から情報を得るルーチン群 */
IdentifierKind getTableKind(int table_index); /* 引数のインデックスに該当するエントリの識別子の種類を得る */
RelAddr getVarRelAddr(int table_index);          /* アドレスを得る */
LL1LL_Value getConstValue(int table_index);        /* 定数の値を得る */
int getNumParams(int table_index);            /* 関数の仮引数の数を得る */

/* ブロック関連のモジュール */
void blockBegin(int first_address, BlockKind kind); /* ブロックの開始で呼ばれ, スタック型記憶領域を更新する */
void blockEnd(void);                /* ブロックの終了で呼ばれ, 前のスタック型記憶域を復帰する */
int getBlockLevel(void);            /* 現ブロックのレベルを得る */
BlockKind getBlockKind(void);       /* 現ブロックの種類を得る */
int getCurrentNumParams(void);      /* 最後に登録した関数のパラメタ数を得る */
int getBlockNeedMemory(void);       /* 現在のブロックが必要とするメモリ容量 */
int getBreakCount(void);            /* 現ブロックのbreakラベルの数を返す */
int incBreakCount(void);            /* 現ブロックのcontinueラベルの数を増やして返す */
int getContinueCount(void);         /* 現ブロックのcontinueラベルの数を返す */
int incContinueCount(void);         /* 現ブロックのbreakラベルの数を増やして返す */
int getDisplayAt(int block_level); /* block_levelにおけるディスプレイの値を得る */  
void setDisplayAt(int block_level, int address); /* block_levelにおけるディスプレイの値をaddressにセット */

#endif /* TABLE_H_INCLUDED */
