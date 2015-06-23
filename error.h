#ifndef ERROR_H_INCLUDED
#define ERROR_H_INCLUDED

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h> /* 可変長引数の取得 */
#include <assert.h>

#include "share.h"
#include "lexical.h"

/* エラーの種類 */
typedef enum {
  SYNTAX_ERROR,                   /* 文法エラー */
  INVAILD_CHAR_ERROR,             /* 無効な文字列エラー */
  FUNCTION_MULTIPLE_DEFINE_ERROR, /* 関数の多重定義エラー */
} CompileError;

/* エラーメッセージのフォーマット文字列 */
typedef struct {
  char *format_string;
} MessageFormat;

/* エラーフォーマットの取得 */
extern MessageFormat compile_error_message_format[];

/* コンパイルエラーの出力 */
void compile_error(CompileError error_id, int line_number,  ...);

#endif  /* ERROR_H_INCLUDED */
