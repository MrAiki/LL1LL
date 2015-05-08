/*
 * メモリ管理モジュール
 * デバッグ時は, マーク（GCのマークではない）を用いてメモリブロックの検査を行う
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "memory.h"

/* 初期（デフォルト）のエラーハンドラの初期化 */
static void
default_error_handler(MEM_Controller controller,
                      char           *filename,
                      int            line,
                      char           *msg);

/* デフォルトのエラーハンドラの定義 */
static struct MEM_Controller_tag st_default_controller = {
  NULL, /* stderr */
  default_error_handler, /* 定義は下に. 簡単なエラー文を出す */
  MEM_FAIL_AND_EXIT      /* 割り当て失敗時は, 終了する */ 
};
/* ハンドラにセット */
MEM_Controller mem_default_controller = &st_default_controller;

/* 管理用領域（領域本体の前後に付く領域）の宣言 */
typedef union {
  long    l_dummy;
  double  d_dummy;
  void    *p_dummy;
} Align;

/* 管理領域の数 実際のサイズはsizeof(unsigned char) * MARK_SIZE */
#define MARK_SIZE (4)

/* メモリヘッダ（双方向リスト） */
typedef struct {
  int           size;             /* アプリが使うブロックサイズ */
  char          *filename;        /* 領域取得元のソースファイル名（若しくは, ストリーム） */
  int           line;             /* 行番号 */
  Header        *prev;            /* 前のヘッダ */
  Header        *next;            /* 次のヘッダ */
  unsigned char mark[MARK_SIZE];  /* マーク（MARK埋立地）領域 */
} HeaderStruct;

/* 1つの管理用領域のサイズ */
#define ALIGN_SIZE            (sizeof(Align))
#define revalue_up_align(val) ((val) ? (((val) - 1) / ALIGN_SIZE + 1) : 0)
/* メモリヘッダの構造体に相当するAlignの数 */
#define HEADER_ALIGN_SIZE     (revalue_up_align(sizeof(HeaderStruct)))

/* 管理領域に埋めるマーク : 1バイト値 0xCD */
#define MARK                  (0xCD)

/* メモリヘッダを構成する共用体 */
union Header_tag {
  HeaderStruct  s;                    /* メモリヘッダ, もしくは */
  Align         u[HEADER_ALIGN_SIZE]; /* その領域に相当するサイズの管理用領域の配列 */
};

/* デフォルトのエラーハンドル：メッセージ表示 */
static void
default_error_handler(MEM_Controller controller,
                      char           *filename,
                      int            line,
                      char           *msg)
{
  fprintf(controller->error_fp,
          "MEM:%s faild in %s at line %d \n", msg, filename, line);
}

/* エラーハンドル（エラーハンドラの呼び出し） */
static void
error_handler(MEM_Controller controller,
              char           *filename,
              int            line,
              char           *msg)
{
  /* error_fpがセットされていない場合は, stderrをセット */
  if (controller->error_fp == NULL) {
    controller->error_fp = stderr;
  }

  /* エラーハンドラの呼び出し */
  controller->error_handler(controller, filename, line, msg);

  /* 終了モードがEXITならば, この場で実行終了 */
  if (controller->fail_mode == MEM_FAIL_AND_EXIT) {
    exit(1);
  }
}

/* メモリコントローラの領域割り当て */
MEM_Controller
MEM_create_controller(void)
{
  MEM_Controller  p;  /* <- メモリ管理構造体の""ポインタ""p */
  /* 領域確保 */
  p = MEM_malloc_func(&st_default_controller, __FILE__, __LINE__, sizeof(struct MEM_Controller_tag));
  /* 中身はst_default_controller自体 */
  *p = st_default_controller;
  return p;
}

#ifdef DEBUG /* 以下, デバッグ時のみ有効 */

/* メモリヘッダ双線形リストの伸長 : 先頭に挿入 */
static void
chain_block(MEM_Controller controller,
            Header         *new_header)
{
  if (controller->block_header) {
    /* 既存のリストの先頭要素の前にnew_headerを登録 */
    controller->block_header->s.prev = new_header;
  }

  /* new_headerに値を詰める:
   * new_headerの前は空, new_headerの次は先頭, 
   * コントローラの先頭にnew_header */
  new_header->s.prev       = NULL;
  new_header->s.next       = controller->block_header;
  controller->block_header = new_header;
}

/* メモリブロックの再結合. reallocの際に用いる */
static void
rechain_block(MEM_Controller controller,
              Header         *header)
{
  if (header->s.prev) {
    /* headerに前の要素が存在すれば, 再結合を促す */
    header->s.prev->s.next   = header;
  } else {
    /* headerが先頭ならば, 改めて先頭にセット */
    controller->block_header = header;
  }

  /* headerに次の要素が存在すれば, 次の前は自分として結合し直す */
  if (header->s.next) {
    header->s.next->s.prev   = header;
  }
}

/* 指定したheaderのリストからの除外 */
static void
unchain_block(MEM_Controller controller,
              Header         *header)
{

  if (header->s.prev) {
    /* 先頭でない場合は, 前の要素の次を自分の次につないでやる（常套手段） */
    header->s.prev->s.next   = header->s.next;
  } else {
    /* 先頭ならば, 自分の次の要素を先頭にセット */
    controller->block_header = header->s.next;
  }

  /* 末尾でないならば, 次の要素の前を自身の前の要素にセット */
  if (header->s.next) {
    header->s.next->s.prev   = header->s.prev;
  }
}

/* ヘッダの初期化. サイズ, 対応するファイル名とその行番号,
 * そしてMARK(マクロ)によるマーク埋めを行う */
void
set_header(Header *header,
           int    size,
           char   *filename,
           int    line)
{
  /* サイズ・ファイル名・行数のセット */
  header->s.size      = size;
  header->s.filename  = filename;
  header->s.line      = line;
  /* markからヘッダの末尾までをMARKで埋める */
  memset(header->s.mark,
         MARK,
         /* (char*)&header[1]は, このヘッダの末尾アドレス値（黒魔術） : 直後にアプリケーションが要求したブツが入っている
          * (char*)header->s.markは, マークの先頭アドレス値
          */
         (char*)&header[1] - (char*)header->s.mark);
}

/* メモリブロックの末尾にMARKをセット */
void
set_tail(void *ptr, int alloc_size)
{
  char *tail;
  /* MARKをセットする先頭アドレス値
   *   = ブロック先頭  + 割当サイズ - マークサイズ */
  tail = ((char *)ptr) + alloc_size - MARK_SIZE;
  /* MARK埋め */
  memset(tail, MARK, MARK_SIZE);
}

/* markからsize * sizeof(unsigned char)分だけの領域に渡って,
 * マーク値がセットされているか確認する.
 * マークを確認できなければアボート */
void
check_mark_sub(unsigned char *mark,
               int           size)
{
  int i;

  for (i = 0;i < size;i++) {
    /* for文で走査し, MARKとの不一致を確認したらアボート */
    if (mark[i] != MARK) {
      fprintf(stderr, "detected bad mark!! \n");
      abort();
    }
  }

}

/* 引数ヘッダのマークをチェック
 * 先頭部分のマークと、末尾部分のマークの両方確認する */
void
check_mark(Header *header)
{
  unsigned char *tail;
  /* アプリケーションが要求した先頭領域（ヘッダ末尾領域）のマークチェック */
  check_mark_sub(header->s.mark,
                 (char*)&header[1] - (char*)header->s.mark);
  /* 末尾アドレス
   *   = ヘッダの先頭アドレス      + アプリ領域     + ヘッダサイズ */
  tail = ((unsigned char *)header) + header->s.size + sizeof(Header);
  /* アプリケーションが要求した末尾領域のマークチェック */
  check_mark_sub(tail, MARK_SIZE);
}

#endif /* DEBUG （マーク系のルーチンはデバッグルーチン） */

/**** 領域確保ルーチン ****/
/* ファイル名(__FILE__), 行数(__LINE__)を取得するため, 実際の呼び出しはMEM.hにあるマクロ(MEM_malloc(size))で行われる */
void*
MEM_malloc_func(MEM_Controller controller,
                char           *filename,
                int            line,
                size_t         size)
{
  void    *ptr;
  size_t  alloc_size;

  /* 確保するサイズの確定 */
#ifdef DEBUG
  /* デバッグならばヘッダと（末尾の）マークのサイズ */
  alloc_size = size + sizeof(Header) + MARK_SIZE;
#else /* DEBUG */
  /* 本番は生のサイズ（実質, mallocとほぼ変わらない） */
  alloc_size = size;
#endif /* DEBUG */

  /* mallocを実行. NULLを返したらエラーハンドル */
  ptr = malloc(alloc_size);
  if (ptr == NULL) {
    error_handler(controller, filename, line, "malloc");
  }

  /* デバッグ時は, 割り当て領域に"0xCC"が埋まる */
#ifdef DEBUG
  memset(ptr, 0xCC, alloc_size);          /* 全て0xCCで埋める */
  set_header(ptr, size, filename, line);  /* ヘッダのセット */
  set_tail(ptr, alloc_size);              /* 末尾のマーク */
  chain_block(controller, (Header*)ptr);  /* メモリブロックに追加 */
  ptr = (char*)ptr + sizeof(Header);      /* 返すポインタは, ヘッダを意識させない：ヘッダ分の領域を加算し, アプリが要求した領域の先頭を指させる */
#endif /* DEBUG */

  return ptr;
}

/*** 領域再確保ルーチン. ptr == NULLの時はMEM_mallocと同じ動作. ***/
/* ファイル名(__FILE__), 行数(__LINE__)を取得するため, 実際の呼び出しはMEM.hにあるマクロ(MEM_realloc(size))で行われる */
void*
MEM_realloc_func(MEM_Controller controller,
                 char           *filename,
                 int            line,
                 void           *ptr,
                 size_t         size)
{
  void      *new_ptr;
  size_t    alloc_size;
  void      *real_ptr;

#ifdef DEBUG
  /* 退避用の古いヘッダと, そのサイズ */
  Header    old_header;
  int       old_size;

  /* 要求される割り当てサイズ */
  alloc_size = size + sizeof(Header) + MARK_SIZE;

  if (ptr != NULL) {
    /* 実際のメモリブロック先頭アドレスを取得 */
    real_ptr   = (char*)ptr - sizeof(Header);
    /* マークを確認 */
    check_mark((Header *)real_ptr);
    /* ヘッダのポインタにキャストして, ヘッダを取得 */
    old_header = *((Header *)real_ptr);
    /* サイズも取得 */
    old_size   = old_header.s.size;
    /* TODO:第二引数はヘッダのポインタにキャストしないと
     * ブロックのリストからの除外 */
    unchain_block(controller, real_ptr);
  } else {
    /* malloc時 : ポインタはNULL, サイズは0 */
    real_ptr = NULL;
    old_size = 0;
  }

#else /* DEBUG */

  /* デバッグでなければ, ナマのサイズとポインタをセット */
  alloc_size = size;
  real_ptr   = ptr;

#endif /* DEBUG */

  /* 実際にreallocを行い, エラーハンドルを行う */
  new_ptr = realloc(real_ptr, alloc_size);

  /* エラーハンドル */
  if (new_ptr == NULL) {
    if (ptr == NULL) {
      /* ポインタがNULLならば, mallocと同様の動きをする */
      error_handler(controller, filename, line, "realloc(malloc : argument pointer is NULL)");
    } else {
      error_handler(controller, filename, line, "realloc");
      free(real_ptr); /* 処理を続行する場合, real_ptrは参照できなくなる（でも, real_ptrを返すわけにはいかない）ので, real_ptrを開放する */
    }
  }

#ifdef DEBUG
  if (ptr) {
    /* ptrがNULLでなければ, old_headerを復帰してサイズ等更新 */
    *((Header *)new_ptr)        = old_header;
    /* サイズの更新 */
    ((Header *)new_ptr)->s.size = size;
    /* メモリブロック・リストへ再登録 */
    rechain_block(controller, (Header *)new_ptr);
    /* 末尾のマークセット */
    set_tail(new_ptr, alloc_size);
  } else {
    /* ptrがNULL:mallocと同じなので, ヘッダを初期化する */
    set_header(new_ptr, size, filename, line);
    set_tail(new_ptr, alloc_size);
    /* リストへ再登録 */
    chain_block(controller, (Header *)new_ptr);
  }

  /* 実際に返すポインタ：使う側からはヘッダを意識させない */
  new_ptr = (char *)new_ptr + sizeof(Header);

  /* 増えたアプリの領域を0xCCで埋める */
  if (size > old_size) {
    memset((char *)new_ptr + old_size, 0xCC, size - old_size);
  }

#endif /* DEBUG */

  return new_ptr;
}

/* 文字列のディープコピー（参照のコピーではなく, 完全に複製する） 
 * デバッグ時, 複製した文字列はメモリブロックリストに追加される */
char*
MEM_strdup_func(MEM_Controller controller,
                char           *filename,
                int            line,
                char           *str)
{
  char    *ptr;
  int     size;
  size_t  alloc_size;

  /* 文字列長(null文字含む)の取得 */
  size = strlen(str) + 1;

#ifdef DEBUG
  /* デバッグ時はヘッダ, マークも追加したサイズ */
  alloc_size = size + sizeof(Header) + MARK_SIZE;

#else /* DEBUG */
  /* 本番時はそのまま */
  alloc_size = size;

#endif /* DEBUG */

  /* コピー先の領域確保 */
  ptr = malloc(alloc_size);
  if (ptr == NULL) {
    /* 領域確保失敗時はエラーハンドル */
    error_handler(controller, filename, line, "strdup");
  }

#ifdef DEBUG
  /* デバッグ時は, まず全て0xCCで埋める */
  memset(ptr, 0xCC, alloc_size);
  /* ヘッダ, 末尾マークのセット */
  set_header((Header *)ptr, size, filename, line);
  set_tail(ptr, alloc_size);
  /* メモリブロックリストに追加 */
  chain_block(controller, (Header *)ptr);
  /* 実際に返すポインタは, ヘッダを意識させない */
  ptr = (char *)ptr + sizeof(Header);

#endif /* DEBUG */

  /* 文字列コピー動作 */
  strcpy(ptr, str);

  return ptr;
}

/* メモリ領域の解放
 * デバッグ時はメモリブロックリストからも除外する */
void 
MEM_free_func(MEM_Controller controller, 
              void           *ptr)
{
  void *real_ptr;

  /* デバッグ時ならばメモリブロックのサイズを取得する */
#ifdef DEBUG
  /* (マクロで切り替えないとコンパイラに怒られる unused) */
  int size;

#endif /* DEBUG */

  /* ポインタがNULLならば, 終了 */
  if (ptr == NULL)
    return;

#ifdef DEBUG
  /* メモリブロックの先頭ポインタを得る */
  real_ptr = (char *)ptr - size(Header);
  /* アプリ領域先頭と末尾のマークのチェック */
  check_mark((Header *)real_ptr);
  size = ((Header *)real_ptr)->s.size;
  /* メモリブロックのリストから除外 */
  unchain_block(controller, real_ptr);
  /* ブロックがあったところを0xCCで埋める 
   * FIXME:TODO:待て, これだと末尾のマークが残る */
  /* memset(real_ptr, 0xCC, size + sizeof(Header) + MARK_SIZE); ではあるまいか？ -> いや, ドントケアかな */
  memset(real_ptr, 0xCC, size + sizeof(Header));

#else /* DEBUG */
  /* 本番時は, そのままのポインタを渡す */
  real_ptr = ptr;

#endif /* DEBUG */

  /* 実際のメモリ解放処理 */
  free(real_ptr);
}

/* メモリコントローラにエラーハンドラの登録
 * MEM_Controller, MEM_ErrorHandlerは共にポインタのマクロであることに留意せよ */
void
MEM_set_error_handler(MEM_Controller   controller,
                      MEM_ErrorHandler handler)
{
  controller->error_handler = handler;
}

/* メモリコントローラに失敗モードをセット */
void
MEM_set_fail_mode(MEM_Controller controller,
                  MEM_FailMode   mode)
{
  controller->fail_mode = mode;
}

/* 全メモリブロックのダンプ（fpに書き出す） : FOR DEBUG */
void
MEM_dump_blocks_func(MEM_Controller controller,
                     FILE           *fp)
{
#ifdef DEBUG
  Header *pos;
  int    counter = 0;
  int    i;

  /* メモリコントローラの保持するメモリブロックリストの
   * 先頭から走査する => 新しい順に表示されるはず */
  for (pos = controller->block_header;
       pos;
       pos = pos->s.next)
  {
    /* マークのチェック */
    check_mark(pos);
    /* カウンタ, アプリ領域の先頭アドレスを表示 */
    fprintf(fp, "[%04d]%p**************************\n",
                counter, (char *)pos + sizeof(Header));
    /* 確保したファイル名, その行数, アプリ領域サイズを表示 */
    fprintf(fp. "%s line %d size..%d\n",
                pos->s.filename, pos->s.line, pos->s.size);
    counter++;
  }

#endif /* DEBUG */
}
       
/* アプリ領域のポインタpに該当するブロックのマークチェック : FOR DEBUG
 * マクロで呼び出され, filenameには__FILE__, lineには__LINE__が入る */
void
MEM_check_block_func(MEM_Controller controller,
                     char           *filename,
                     int            line,
                     void           *p)
{
#ifdef DEBUG
  /* メモリブロックの先頭アドレスの取得 */
  void *real_ptr = ((char *)p) - sizeof(Header);

  /* マークチェック */
  check_mark(real_ptr);
#endif /* DEBUG */
}

/* 全てのブロックのマークをチェックする : FOR DEBUG
 * マクロで呼び出され, filenameには__FILE__, lineには__LINE__が入る */
void
MEM_check_all_blocks_func(MEM_Controller controller,
                          char           *filename,
                          int            line)
{
#ifdef DEBUG
  Header *pos;

  /* メモリコントローラのメモリブロックリストを走査し,
   * 逐次マークをチェック */
  for (pos = controller->block_header;
       pos;
       pos = pos->next) {
    check_mark(pos);
  }
#endif /* DEBUG */
}

