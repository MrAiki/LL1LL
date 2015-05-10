#ifndef HEAP_H_INCLUDED
#define HEAP_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>

#include "MEM.h"
#include "share.h"
#include "execute.h"

/* 文字列srcのヒープ領域への割り当て. 文字列はヒープへとディープコピーされる */
LL1LL_Object* alloc_string(char *src, LL1LL_Boolean is_literal);
/* 配列のヒープ領域への割り当て. */
LL1LL_Object* alloc_array(size_t size);

/* gcを行う. マーク・アンド・スイープ方式 */
void gaberage_collect(void);

#endif /* HEAP_H_INCLUDED */
