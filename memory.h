#ifndef PRIVATE_MEM_H_INCLUDED
#define PRIVATE_MEM_H_INCLUDED
#include "MEM.h"

/* 管理用領域のヘッダ構造体の宣言(実体はunion!!) */
typedef union Header_tag Header;

/* メモリ管理を行う構造体 */
struct MEM_Controller_tag {
  FILE             *error_fp;     /* エラー時に用いるストリーム */
  MEM_ErrorHandler error_handler; /* エラーハンドルを行う関数へのポインタ */
  MEM_FailMode     fail_mode;     /* 終了モード（終わらせるか, returnするか） */
  Header           *block_header; /* ブロックヘッダ */
};

#endif  /* PRIVATE_MEM_H_INCLUDED */
