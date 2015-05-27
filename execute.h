#ifndef EXECUTE_H_INCLUDED
#define EXECUTE_H_INCLUDED

#include <stdio.h>

#include "MEM.h"
#include "share.h"
#include "generate.h"
#include "heap.h"

/* FIXME:こいつも可変にしましょう */
#define MAX_EXE_STACK_SIZE (3000)     /* 実行時スタックの最大サイズ */

int get_stack_top(void);              /* 現在のスタックトップを得る */
LL1LL_Value *get_stack_pointer(void); /* 現在のスタックを指すポインタを得る */

void execute(void);                   /* 実行 */

#endif /* EXECUTE_H_INCLUDED */
