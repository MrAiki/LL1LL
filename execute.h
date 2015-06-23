#ifndef EXECUTE_H_INCLUDED
#define EXECUTE_H_INCLUDED

#include <stdio.h>
#include <math.h>
#include <float.h>  /* For DBL_EPSILON */

#include "MEM.h"
#include "share.h"
#include "generate.h"
#include "heap.h"

/* FIXME:こいつも可変にしましょう */
#define MAX_EXE_STACK_SIZE (3000)     /* 実行時スタックの最大サイズ */
#define RUNTIME_STR_BUF_SIZE (200)    /* 実行時に確保しておく文字列バッファの長さ */

int getStackTop(void);              /* 現在のスタックトップを得る */
LL1LL_Value *getStackPointer(void); /* 現在のスタックを指すポインタを得る */

void execute(void);                   /* 実行 */

void printStack(void);                /* スタックの状態を表示 */

#endif /* EXECUTE_H_INCLUDED */
