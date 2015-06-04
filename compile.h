#ifndef COMPILE_H_INCLUDED
#define COMPILE_H_INCLUDED

#include <stdio.h>
#include <string.h>

#include "share.h"
#include "lexical.h"
#include "heap.h"
#include "error.h"
#include "table.h"
#include "generate.h"

extern int line_number; /* 現在コンパイル中の行数 */

int compile(void);

#endif /* COMPILE_H_INCLUDED */
