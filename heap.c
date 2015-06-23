#include "heap.h"

/* FIXME: 定数に代入したオブジェクトはどうする
 *        アイデア:記号表を走査し, constならばマーク
 *        あるいは, constならスイープしない          */

/* ヒープ領域の先頭を指すポインタ */
static LL1LL_Object *heap_head;

static void addHeapEntry(LL1LL_Object *entry);    /* ヒープ領域管理リストにエントリを追加 */
static void deleteHeapEntry(LL1LL_Object *entry); /* ヒープ領域管理リストのエントリを削除 */
static void gc_mark(void);  /* 参照できるオブジェクトを辿ってマークをつける */
static void gc_sweep(void); /* マークのついていないオブジェクトの領域を開放する */
static void array_mark(LL1LL_Object *ary_ptr);   /* 配列の再帰的マーク */
static void array_sweep(LL1LL_Object *ary_ptr);  /* 配列の再帰的領域解放:要素から先にマークを確認し, 開放していく */

/* 双方向リストの先頭にエントリ(領域割り当て済み)を追加する */
static void addHeapEntry(LL1LL_Object *entry)
{
  if (heap_head == NULL) {
    /* 初期状態ならば, 前後ろ共にNULLで初期化 */
    heap_head       = entry;    /* 先頭に挿入 */
    heap_head->prev = NULL;
    heap_head->next = NULL;
  } else {
    /* 一版の場合は, 先頭に追加 */
    heap_head->prev = entry;      /* 先頭の前に追加 */
    entry->next     = heap_head;  /* エントリの次は先頭 */
    entry->prev     = NULL;       /* エントリの前はNULL */
    heap_head       = entry;      /* 先頭の更新 */
  }
}

/* 双方向リストのエントリを削除する
 * メモリ解放は行わない. */
static void deleteHeapEntry(LL1LL_Object *entry)
{
  if (heap_head == NULL) {
    /* 空リストから要素は削除できない */
    fprintf(stderr, "Error, delete entry from NULL list \n");
    exit(1);
  } else {
    if (entry->prev == NULL) {
      /* エントリが先頭ならば, エントリの次の要素を先頭に */
      heap_head         = entry->next;
      entry->next->prev = NULL;
    } else {
      /* 一般の要素の削除 */
      entry->prev->next = entry->next;  /* 前の次はエントリの次 */
      entry->next->prev = entry->prev;  /* 次の前はエントリの前 */
    }
    /* リストから削除したエントリの次と前はNULL */
    entry->next         = NULL;
    entry->prev         = NULL;
  }
}

/* 文字列srcをヒープ領域へ割り当てる. 文字列はヒープ領域へディープコピーされる. */
LL1LL_Object* alloc_string(char *src, LL1LL_Boolean is_literal)
{
  /* 領域確保 */
  LL1LL_Object *new_entry       = (LL1LL_Object *)MEM_malloc(sizeof(LL1LL_Object));
  /* オブジェクトの種類と, マークの初期化 */
  new_entry->type               = STRING_OBJECT;
  new_entry->marked             = LL1LL_FALSE;
  /* 内容を埋める */
  new_entry->u.str.string_value = MEM_strdup(src);
  new_entry->u.str.is_literal   = is_literal;
  /* ヒープ管理リストへ追加 */
  addHeapEntry(new_entry);

  return new_entry;
}

/* 引数の文字列をstr1str2で連結し, 結果をヒープに登録し, オブジェクト参照ポインタを返す */
LL1LL_Object* cat_string(char *str1, char *str2)
{
  char* str_result;    /* 連結結果の文字列 */
  int str_len;         /* 文字列長 */
  /* 領域確保 */
  LL1LL_Object *new_entry       = (LL1LL_Object *)MEM_malloc(sizeof(LL1LL_Object));
  /* オブジェクトの種類と, マークの初期化 */
  new_entry->type               = STRING_OBJECT;
  new_entry->marked             = LL1LL_FALSE;

  /* 完成後の文字列長を取得し, 結果の文字列を構成 */
  str_len    = strlen(str1) + strlen(str2) + 1;
  str_result = (char *)MEM_malloc(sizeof(char) * str_len);
  strcat(str_result, str1);
  strcat(str_result, str2);

  /* 結果をヒープに登録 */
  new_entry->u.str.string_value = str_result;
  new_entry->u.str.is_literal   = LL1LL_FALSE;
  /* ヒープ管理リストへ追加 */
  addHeapEntry(new_entry);

  return new_entry;
}

/* サイズsizeの配列をヒープ領域へ割り当てる. 要素の初期化は特に行わず, 呼んだ側で頑張ってもらう */
LL1LL_Object* alloc_array(size_t size)
{
  /* 領域確保 */
  LL1LL_Object *new_entry      = (LL1LL_Object *)MEM_malloc(sizeof(LL1LL_Object));
  /* オブジェクトの種類と, マークの初期化 */
  new_entry->type              = ARRAY_OBJECT;
  new_entry->marked            = LL1LL_FALSE;
  /* 配列として, LL1LL_Valueの配列を動的に確保する */
  new_entry->u.ary.array_value = (LL1LL_Value *)MEM_malloc(sizeof(LL1LL_Value) * size);
  /* サイズ設定 */
  new_entry->u.ary.size        = size;
  new_entry->u.ary.alloc_size  = size * sizeof(LL1LL_Value); /* TODO:これは正しくない（配列の配列の場合は, 要素の配列のサイズも加算しなくてはいけない）が, 今のところ問題にはなっていない */
  /* ヒープ管理リストに追加 */
  addHeapEntry(new_entry);

  return new_entry;

}

/* 配列オブジェクトの要素をマーク */
static void array_mark(LL1LL_Object *ary_ptr)
{
  int i;
  LL1LL_Value ary_i;
  ary_ptr->marked = LL1LL_TRUE; /* まず, 根本をマーク */

  /* 配列要素のマーク */
  for (i = 0; i < ary_ptr->u.ary.size; i++) {
    ary_i = ary_ptr->u.ary.array_value[i];  /* 要素を取得 */
    if (ary_i.type != LL1LL_OBJECT_TYPE) {
      /* オブジェクト型でないなら次へ */
      continue;
    } else {
      /* オブジェクト型の場合 */
      if (ary_i.u.object->type == STRING_OBJECT) {
        ary_i.u.object->marked = LL1LL_TRUE; /* 文字列ならそのままマーク */
      } else {
        /* should be ARRAY_OBJECT here */
        array_mark(ary_i.u.object);            /* 配列のマーク */
      }
    }
  }

}

/* 配列の再帰的領域解放 : 要素から先にマークを確認し, 開放していく */
static void array_sweep(LL1LL_Object *ary_ptr)
{
  int i;
  LL1LL_Value ary_i;

  /* 配列要素のスイープ */
  for (i = 0; i < ary_ptr->u.ary.size; i++) {
    ary_i = ary_ptr->u.ary.array_value[i];  /* 要素を取得 */
    if (ary_i.type != LL1LL_OBJECT_TYPE) {
      /* オブジェクト型でないなら次へ */
      continue;
    } else {
      /* オブジェクト型の場合 */
      if (ary_i.u.object->marked == LL1LL_FALSE) {
        /* マークがFALSE => スイープ（解放） */
        if (ary_i.u.object->type == STRING_OBJECT) {
          MEM_free(ary_i.u.object); /* 文字列ならばそのまま解放 */
        } else {
          /* should be ARRAY_OBJECT here */
          /* 配列ならば要素のスイープ */
          array_sweep(ary_i.u.object);
        }
      }

    }
  }

  /* 大本の配列のスイープ */
  if (ary_ptr->marked == LL1LL_FALSE) {
    MEM_free(ary_ptr);
  }

}

/* GCのマークフェーズ */
static void gc_mark(void)
{
  int i;
  /* スタックトップと, スタックを指すポインタを取得 */
  int stack_top        = getStackTop();  
  LL1LL_Value *stack_p = getStackPointer(); 
  LL1LL_Object *pos;

  /* 全マークをリセット */
  for (pos = heap_head; pos != NULL; pos = pos->next)
    pos->marked = LL1LL_FALSE;

  /* スタック（参照できるオブジェクト）を走査 */
  for (i = 0; i < stack_top; i++) {
    if (stack_p[i].type != LL1LL_OBJECT_TYPE) { 
      /* オブジェクトでないなら次へ */
      continue;
    } else {
      /* マークを付ける */
      if (is_string(stack_p[i])) {
        stack_p[i].u.object->marked = LL1LL_TRUE;
      } else {
        /* should be ARRAY_OBJECT here */
        array_mark(stack_p[i].u.object);  /* 配列マークのルーチンへ */
      }
    }
  }

}

/* GCのスイープフェーズ */
static void gc_sweep(void)
{
  LL1LL_Object *pos, *sweep_entry;

  /* ヒープ領域管理リストを走査し, マークが付いてない
   * オブジェクトをリストから削除し, 開放していく */
  for (pos = heap_head; pos != NULL; pos = pos->next) {
    /* マークが付いてないオブジェクトを発見 */
    if (pos->marked == LL1LL_FALSE) {
      /* 削除・解放するエントリを設定 */
      sweep_entry = pos; 

      /* リストから削除 */
      if (pos == heap_head) {
        /* 先頭の場合は, 先頭を削除してから先頭を再設定 */
        deleteHeapEntry(sweep_entry);
        pos = heap_head;
      } else {
        /* 次のループに備えて一個前に戻る */ 
        pos = pos->prev;
        deleteHeapEntry(sweep_entry);
      }

      /* 領域解放 */
      if (sweep_entry->type == STRING_OBJECT) {
        MEM_free(sweep_entry);
      } else {
        /* should be ARRAY_OBJECT here */
        array_sweep(sweep_entry);
      }
    }  
  }

}

/* GC(ガベージコレクション)を行う */
void startGC(void)
{
  gc_mark();  /* マーク */
  gc_sweep(); /* スイープ */
}
