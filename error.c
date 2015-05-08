#include "error.h"

/* エラーフォーマットの取得 */
extern MessageFormat compile_error_message_format[];

/* フォーマットに従って, メッセージを生成する */
static void
format_message(MessageFormat *format,
               char          *str,
               va_list       ap)
{
  int i;
  char buf[LINE_BUF_SIZE];

}

/* コンパイルエラー:エラー出力 */
void compile_error(CompileError error_id, int line_number,  ...)
{
  va_list ap;
  char    *message;

  /* 可変長引数スタート */
  va_start(ap, line_number);

  /* メッセージの生成 */
  /* format_message(&compile_error_message_format[error_id],
                    &message, ap); */
  /* stderrに出力 */
  fprintf(stderr, "%3d:%s", line_number, &compile_error_message_format[error_id]);

  /* 可変長引数終了 */
  va_end(ap);

  exit(1);
}
                    


