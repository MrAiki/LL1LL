#include <string.h>
#include "error.h"

#define ANOSA "あのさぁ...（棒読み） \n"

MessageFormat compile_error_message_format[] = {
  {ANOSA "お前らこんな情けない文法エラー,恥ずかしくないの？（棒読み）(こ↑こ↓ $(token)付近)"}, /* SYNTAX_ERROR */
  {ANOSA "だから不正な文字($(bad_char))使ってんじゃねえよ（棒読み）"},  /* INVAILD_CHAR_ERROR */
  {ANOSA "関数名($(name))が重複しているんだよなぁ...そんなんじゃ甘いよ（棒読み）"},  /* FUNCTION_MULTIPLE_DEFINE_ERROR */
};
