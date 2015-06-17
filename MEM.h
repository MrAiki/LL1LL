#ifndef PUBLIC_MEM_H
#define PUBLIC_MEM_H

#include <stdio.h>
#include <stdlib.h>

/* 領域確保失敗の種類を表すタグ */
typedef enum {
  MEM_FAIL_AND_EXIT,    /* 失敗したら終了 */
  MEM_FAIL_AND_RETURN,  /* 失敗したら関数から戻る */
} MEM_FailMode;

/* メモリコントローラーの不完全型宣言 */
typedef struct MEM_Controller_tag *MEM_Controller;
/* エラーハンドラの関数型マクロ 
 * (MEM_Controller, char *, int, char *)を引数に取り, void*を返す */
typedef void (*MEM_ErrorHandler)(MEM_Controller, char *, int, char *);
/* メモリストレージの不完全型宣言 */
typedef struct MEM_Storage_tag *MEM_Storage;

/* デフォルトのコントローラの宣言(中身はプライベート) */
extern MEM_Controller mem_default_controller;

/* MEM_CURRENT_CONTROLLER - 現在のメモリコントローラのセット
 * MEM_CONTROLLERにマクロが定義されていなければ, mem_default_controller(memory.c)を使う */
#ifdef MEM_CONTROLLER
#define MEM_CURRENT_CONTROLLER MEM_CONTROLLER
#else /* MEM_CONTROLLER */
#define MEM_CURRENT_CONTROLLER mem_default_controller
#endif /* MEM_CONTROLLER */

/* 
 * mem_*_func を使ってはいけない（戒め）
 * これらはプライベート関数である
 * 一方大文字のMEM_*_func は公開してある
 */
MEM_Controller MEM_create_controller(void);
/* メモリ関連のルーチン群(殆どマクロでラップされる) */
/* 領域確保 */
void *MEM_malloc_func(MEM_Controller controller,
                      char           *filename,
                      int            line,
                      size_t         size);
/* 領域再確保 */
void *MEM_realloc_func(MEM_Controller controller,
                       char           *filename,
                       int            line,
                       void           *ptr,
                       size_t         size);
/* 文字列複製 */
char *MEM_strdup_func(MEM_Controller controller,
                      char           *filename,
                      int            line,
                      char           *str);
MEM_Storage MEM_open_storage_func(MEM_Controller controller,
                                  char           *filename,
                                  int            line,
                                  MEM_Storage    storage,
                                  size_t         size);
void *MEM_storage_malloc_func(MEM_Controller controller,
                              char           *filename,
                              int            line,
                              MEM_Storage    storage,
                              size_t         size);
/* 領域解放 */
void MEM_free_func(MEM_Controller controller,
                   void           *ptr);
void MEM_dispose_storage_func(MEM_Controller controller,
                              MEM_Storage    storage);
/* エラーハンドラのセット */
void MEM_set_error_handler(MEM_Controller   controller,
                           MEM_ErrorHandler handler);
/* 失敗モードのセット */
void MEM_set_fail_mode(MEM_Controller controller,
                       MEM_FailMode   mode);
/* ブロックのダンプ */
void MEM_dump_blocks_func(MEM_Controller controller,
                          FILE           *fp);
void MEM_check_block_func(MEM_Controller controller, 
                          char           *filename,
                          int            line,
                          void           *p);
void MEM_check_all_blocks_func(MEM_Controller controller,
                               char           *filename,
                               int            line);

/* alloc macros メモリ関連のマクロ.
 * 実際の呼び出しではこいつを使う
 * __LINE__でソースファイルの現在行を,
 * __FILE__でソースファイルの名前を取得できる
 */
/* 領域確保 */
#define MEM_malloc(size)\
  (MEM_malloc_func(MEM_CURRENT_CONTROLLER,\
                   __FILE__, __LINE__, size))
/* 領域再確保 */
#define MEM_realloc(ptr, size)\
  (MEM_realloc_func(MEM_CURRENT_CONTROLLER,\
                    __FILE__, __LINE__, ptr, size))
/* 文字列複製（ディープコピー） */
#define MEM_strdup(str)\
  (MEM_strdup_func(MEM_CURRENT_CONTROLLER,\
                   __FILE__, __LINE__, str))
#define MEM_open_storage(page_size)\
  (MEM_open_storage_func(MEM_CURRENT_CONTROLLER,\
                         __FILE__, __LINE__, page_size))
#define MEM_storage_malloc(storage, size)\
  (MEM_storage_malloc_func(MEM_CURRENT_CONTROLLER,\
                           __FILE__, __LINE__, storage, size))
/* 領域解放 */
#define MEM_free(ptr)\
  (MEM_free_func(MEM_CURRENT_CONTROLLER, ptr))

#ifdef DEBUG
#define MEM_dump_blocks(fp)\
  (MEM_dump_blocks_func(MEM_CURRENT_CONTROLLER, fp))
#define MEM_check_block(p)\
  (MEM_check_block_func(MEM_CURRENT_CONTROLLER,\
                        __FILE__, __LINE__, p))
#define MEM_check_all_block()\
  (MEM_check_all_blocks_func(MEM_CURRENT_CONTROLLER,\
                             __FILE__, __LINE__))

#else /* DEBUG */

/* DEBUGでないときは, ブロックチェックルーチンは
 * voidにキャストした0(NULL=(void*)0ではない)を返す */
#define MEM_dump_blocks(fp)     ((void)0)
#define MEM_check_block(p)      ((void)0)
#define MEM_check_all_blocks()  ((void)0)

#endif /* DEBUG */

#endif /* PUBLIC_MEM_H */
