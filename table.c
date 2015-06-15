#include "table.h"

/* 名前表のエントリ */
typedef struct {
  IdentifierKind kind;  /* 識別子の種類 */
  char           name[MAX_LEN_IDENTIFIER]; /* 名前 FIXME:ポインタに置き換える事 */
  /* 中身 */
  union {
    RelAddr     rel_address;  /* 変数の場合:スタック型記憶域のアドレス */
    LL1LL_Value value;        /* 定数の場合:値 */
    struct {                   /* 関数の場合 */
      RelAddr rel_address;     /* 記憶域のアドレス */
      int     num_parameter;   /* 仮引数の数 */
    } f;
  } u;
} NameTableEntry;

static NameTableEntry name_table[MAX_TABLE_SIZE]; /* 名前表 FIXME:ポインタに置き換える */
static int table_index = 0;   /* 現在参照している名前表のインデックス */
static int table_func_index;  /* 現在参照している関数の名前表のインデックス */
static int current_block_level = -1;  /* 現在のブロックレベル */

/* TODO:ブロックの情報を構造体にまとめる */
static int last_index[MAX_BLOCK_LEVEL]; /* i番目の要素は, ブロックレベルiの最後の名前表のインデックス */
static int last_addr[MAX_BLOCK_LEVEL];  /* i番目の要素は, ブロックレベルiの最後の変数のアドレス */
static int block_kind[MAX_BLOCK_LEVEL]; /* i番目の要素は, ブロックレベルiの種類 */
static int local_addr;        /* 現在のブロックの最後の変数番地 */
static int break_label_count[MAX_BLOCK_LEVEL];    /* 各ブロックのbreakラベルの個数 */
static int continue_label_count[MAX_BLOCK_LEVEL]; /* 各ブロックのcontinueラベルの個数 */

/* 名前表エントリ数を増やし, 名前を登録 */
void addTableName(char *identifier)
{
  table_index++;  /* エントリの増加 */

  /* 限界値に達していなければ新規登録 */
  if (table_index < MAX_TABLE_SIZE) {
    strcpy(name_table[table_index].name, identifier);
  } else {
    fprintf(stderr, "Too many name table entry... \n TODO:Substitute tabel array to pointer one. \n");
    exit(1);
  }
}

/* 名前表に関数を登録 */
int addTableFunc(char *identifier, int address)
{
  addTableName(identifier); /* 名前を登録 */
  name_table[table_index].kind                        = FUNC_IDENTIFIER;  /* 識別子の種類 */
  name_table[table_index].u.f.rel_address.block_level = current_block_level;  /* ブロックレベル */
  name_table[table_index].u.f.rel_address.address     = address;      /* 先頭番地をセット */
  name_table[table_index].u.f.num_parameter           = 0;            /* 仮引数の数は0で初期化 */
  table_func_index = table_index; /* 仮引数の情報登録のため, 現在の関数のインデックスを保存 */

  return table_index;
}

/* 名前表に仮引数を登録 */
int addTableParam(char *identifier)
{
  addTableName(identifier); /* 名前を登録 */
  name_table[table_index].kind = PARAM_IDENTIFIER;  /* 識別子の種類 */
  name_table[table_index].u.rel_address.block_level  = current_block_level; /* 現在のブロックレベル */
  name_table[table_func_index].u.f.num_parameter++; /* 現在の関数のインデックスを用いて, 仮引数の数を増やす */
  return table_index;
}

/* 名前表に変数を登録 */
int addTableVar(char *identifier)
{
  addTableName(identifier);
  name_table[table_index].kind = VAR_IDENTIFIER;
  name_table[table_index].u.rel_address.block_level = current_block_level;  /* 現在のブロックレベル */
  name_table[table_index].u.rel_address.address     = local_addr++; /* ローカル変数のアドレスを更新しつつ登録 */
  return table_index;
}

/* 名前表に定数を登録 */
int addTableConst(char *identifier, LL1LL_Value value)
{
  addTableName(identifier);
  name_table[table_index].kind    = CONST_IDENTIFIER;
  name_table[table_index].u.value = value;  /* 値を登録 */
  return table_index;
}

/* 関数の仮引数にアドレスを割り当てる */
void parEnd(void) 
{
  int i;
  int n_param = name_table[table_func_index].u.f.num_parameter; /* 現在コンパイル中の関数の仮引数の個数を取得 */

  /* パラメタ無しの関数なら戻る */
  if (n_param == 0) return; 

  /* 仮引数にアドレスを割り当てていく:
   * 現在のディスプレイが指す領域の一つ前までを, 仮引数で埋める */
  for (i = 1; i <= n_param; i++) {
    name_table[table_func_index + i].u.rel_address.address = (i - 1 - n_param);
  }

}

/* table_indexにある関数の先頭アドレスの変更 */
void changeFuncAddr(int table_index, int new_address)
{
  name_table[table_index].u.f.rel_address.address = new_address;
}

/* 名前表から, 名前identifier, 種類kindのエントリを探し, あればそのインデックスを返す. なければ0を返すが, 変数を探索していた場合は, 変数を新規登録する */
int searchTable(char *identifier, IdentifierKind kind)
{

  /* スタック型領域のため, 探索は末尾から始める */
  int i = table_index;

  /* 0番目に番兵を立てる */
  strcpy(name_table[0].name, identifier);

  /* 探索 */
  while( strcmp(identifier, name_table[i].name) != 0 )
    i--;

  if ( i != 0 ) {
    /* 発見 */
    return i;
  } else {
    fprintf(stderr, "warning: undefined identifier:%s \n", identifier);
    /* 探しているエントリが変数の場合は,
     * 仮登録を行う */
    if (kind == VAR_IDENTIFIER) {
      return addTableVar(identifier);
    }
    return 0;
  }

}

/* 引数のインデックスに該当するエントリの, 識別子の種類を得る */
IdentifierKind getTableKind(int index)
{
  return name_table[index].kind;
}

/* 引数のインデックスに該当する変数の, アドレスを得る */
RelAddr getVarRelAddr(int index)
{
  return name_table[index].u.rel_address;
}

/* 引数のインデックスに該当する定数の値を得る */
LL1LL_Value getConstValue(int index)
{
  return name_table[index].u.value;
}

/* 引数のインデックスに該当する関数の, 仮引数の数を得る */
int getNumParams(int index)
{
  return name_table[index].u.f.num_parameter;
}

/* ブロック開始で呼ばれる.
 * スタック型記憶領域の更新, ブロックレベル更新 */
void blockBegin(int first_address, BlockKind kind)
{
  int i;
  /* トップレベルの際は, 初期設定を行う */
  if (current_block_level == -1) {
    local_addr    = first_address; /* (FIRST_LOCAL_ADDRESSが入る) */
    table_index   = 0;             /* 名前表インデックスの初期化 */
    block_kind[0] = TOPLEVEL;      /* ブロックの種類をトップレベルに */
    /* ラベルの個数を全てリセット */
    for (i = 0; i < MAX_BLOCK_LEVEL; i++) {
      break_label_count[i] = 0;
      continue_label_count[i] = 0;
    }
    current_block_level++;                 /* ブロックレベルを0に */
    return;
  }

  /* ブロックを深くネストしすぎた場合は, エラー終了
   * FIXME:ブロックネストも可変であるべき.
   * <=> last_index, last_addrを可変に */
  if (current_block_level == MAX_BLOCK_LEVEL-1) {
    fprintf(stderr, "Too many nested blocks. \n");
    exit(1);
  }

  /* 現在のブロックの情報を保存 */
  last_index[current_block_level] = table_index;  /* 名前表インデックス */
  last_addr[current_block_level]  = local_addr;   /* ローカル変数のインデックス */
  block_kind[current_block_level] = kind;         /* 現在のブロックの種類 */
  /* ラベルの個数を初期化 */
  break_label_count[current_block_level]    = 0;
  continue_label_count[current_block_level] = 0;

  /* 新しいブロックの最初の変数のアドレスに書き換え */
  local_addr              = first_address;
  /* ブロックレベルの更新 */
  current_block_level++;

}

/* ブロックの終わりで呼ばれる
 * 前のスタック型記憶領域を復帰する */
void blockEnd(void)
{
  current_block_level--;  /* ブロックレベルを元に戻す */

  /* 名前表インデックスと, ローカル変数アドレス,
   * ブロックの種類の復帰 */
  table_index = last_index[current_block_level];
  local_addr  = last_addr[current_block_level];
}

/* 現ブロックのbreakラベルの数を得る */
int getBreakCount(void)
{
  return break_label_count[current_block_level];
}

/* 現ブロックのbreakラベルの数を増やして返す */
int incBreakCount(void)
{
  return ++break_label_count[current_block_level];
}

/* 現ブロックのcontinueラベルの数を得る */
int getContinueCount(void)
{
  return continue_label_count[current_block_level];
}

/* 現ブロックのcontinueラベルの数を増やして返す */
int incContinueCount(void)
{
  return ++continue_label_count[current_block_level];
}

/* 現在のブロックレベルを得る
 * current_block_levelのスコープをグローバルにしない */
int getBlockLevel(void)
{
  return current_block_level;
}

/* 現在のブロックの種類を得る */
BlockKind getBlockKind(void)
{
  return block_kind[current_block_level-1];
}

/* 最後に登録した関数の仮引数の個数を返す */
int getCurrentNumParams(void)
{
  return name_table[table_func_index].u.f.num_parameter;
}

/* 現在のブロックが必要とするメモリ容量(ローカル変数+定数+仮引数の数) */
int getBlockNeedMemory(void)
{
  return local_addr;
}
