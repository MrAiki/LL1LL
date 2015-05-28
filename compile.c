#include "compile.h"

#define FIRST_LOCAL_ADDRESS (2) /* 各ブロックの最初のローカル変数のアドレス:1には戻り番地が入る */

static Token token; /* 次のトークン */

/* コンパイル・モジュール群 */
static void toplevel(void);               /* トップレベルのコンパイル */
static void varDecl(void);                /* 変数定義のコンパイル */
static void constDecl(void);              /* 定数定義のコンパイル */
static void funcDecl(void);               /* 関数定義のコンパイル */
static void statement(void);              /* 文のコンパイル */
static void if_statement(void);           /* if文のコンパイル */
static void switch_statement(void);       /* switch文のコンパイル */
static void for_statement(void);          /* for文のコンパイル */
static void while_statement(void);        /* while文のコンパイル */
static void do_while_statement(void);     /* do while文のコンパイル */
static void block(void);                  /* ブロックのコンパイル */
static void comma_expression(void);       /* コンマ式のコンパイル */
static void expression(void);             /* 式のコンパイル */
static void logical_or_expression(void);  /* 論理OR式のコンパイル */
static void logical_and_expression(void); /* 論理AND式のコンパイル */
static void equal_expression(void);       /* 等号, 不等号式のコンパイル */
static void compare_expression(void);     /* 不等式のコンパイル */
static void add_expression(void);         /* 加算式のコンパイル */
static void mul_expression(void);         /* 積算式のコンパイル */
static void power_expression(void);       /* 累乗式のコンパイル */
static void unary_expression(void);       /* 単項式のコンパイル */
static void term(void);                   /* 項のコンパイル */

/* コンパイル */
int compile(void)
{
  printf("Start compilation. \n");
  initSource();             /* 字句解析の準備 */
  token = nextToken();      /* 最初の先読みトークンを読む */
  toplevel();               /* トップレベルからコンパイル */

  return 0;   /* TODO: エラー個数を返すようにする */
}

/* トップレベルのコンパイル */
static void toplevel()
{
  while (1) {
    switch (token.kind) {
      case VAR:               /* 変数定義 */
        token = nextToken();
        varDecl();
        continue;
      case CONST:             /* 定数定義 */
        token = nextToken();
        constDecl();
        continue;
      case FUNC:              /* 関数定義 */
        token = nextToken();
        funcDecl();
        continue;
      default:                /* 文 */
        statement();
        continue;
    }
  }
}

/* 変数定義のコンパイル 
 * => 後に宣言にも対応するかもしれない... */
static void varDecl(void)
{
  while (1) {
    /* IDENTIFIER -> ASSIGNの並びを確認 */
    token = checkGetToken(token, IDENTIFIER);
    token = checkGetToken(token, ASSIGN);

    /* 代入する値で場合分け */
    switch (token.kind) {
      case INT_LITERAL:       /* 整数リテラル */
        /* 記号表へ登録 */
        token = nextToken();
        break;
      case DOUBLE_LITERAL:    /* 倍精度実数リテラル */
        /* 記号表へ登録 */
        token = nextToken();
        break;
      case STRING_LITERAL:    /* 文字列リテラル */
        /* 記号表へ登録 */
        token = nextToken();
        break;
      case TRUE_LITERAL:      /* FALLTHRU */ /* 論理値リテラル */
      case FALSE_LITERAL:
        /* 記号表へ登録 */
        token = nextToken();
        break;
      default:                /* それ以外はシンタックスエラー */
        compile_error(SYNTAX_ERROR, line_number);
    }

    /* 次にコンマが出ていたら変数定義が続く */
    if (token.kind == COMMA) {
      token = nextToken();
      continue;
    } else {
      break;
    }

  }

  /* 最後は;で終わらなければならない */
  /* token = checkGetToken(token, SEMICOLON); */
  /* ';'は省略可能に */
  if (token.kind == SEMICOLON) {
    token = nextToken();
  }
}

/* 定数定義のコンパイル */
static void constDecl(void)
{
  while (1) {
    /* IDENTIFIER -> ASSIGNの並びを確認 */
    token = checkGetToken(token, IDENTIFIER);
    token = checkGetToken(token, ASSIGN);

    /* 代入する値で場合分け */
    switch (token.kind) {
      case INT_LITERAL:       /* 整数リテラル */
        /* 記号表へ登録 */
        token = nextToken();
        break;
      case DOUBLE_LITERAL:    /* 倍精度実数リテラル */
        /* 記号表へ登録 */
        token = nextToken();
        break;
      case STRING_LITERAL:    /* 文字列リテラル */
        /* 記号表へ登録 */
        token = nextToken();
        break;
      case TRUE_LITERAL:      /* FALLTHRU */ /* 論理値リテラル */
      case FALSE_LITERAL:
        /* 記号表へ登録 */
        token = nextToken();
        break;
      default:                /* それ以外はシンタックスエラー */
        compile_error(SYNTAX_ERROR, line_number);
    }

    /* 次にコンマが出ていたら変数定義が続く */
    if (token.kind == COMMA) {
      token = nextToken();
      continue;
    } else {
      break;
    }

  }

  /* 最後は;で終わらなければならない */
  /* token = checkGetToken(token, SEMICOLON); */
  /* ';'は省略可能に */
  if (token.kind == SEMICOLON) {
    token = nextToken();
  }
}

/* 関数定義のコンパイル */
static void funcDecl(void)
{
  /* TODO:関数の情報をバッファにとっておいて, コンパイル後に登録 */

  /* IDENTIFIERを確認 */
  token = checkGetToken(token, IDENTIFIER);

  if (token.kind == LEFT_PARLEN) {
    /* 仮引数リストが始まっていた場合 */
    token = nextToken();

    /* 仮引数の並びparam1, param2, ...のコンパイル */
    while (1) {
      /* IDENTIFIERを取得 */
      token = checkGetToken(token, IDENTIFIER);
      if (token.kind == COMMA) {
        /* コンマが来たら仮引数の並びが続く */
        token = nextToken();
        continue;
      } else {
        break;
      }
    }
    /* 仮引数リストの閉じ ) を確認する */
    token = checkGetToken(token, RIGHT_PARLEN);
  } else {
    /* 仮引数リストを省略した場合(仮引数無しの関数と等価) */
  }

  /* 関数の処理内容のコンパイル */
  token = checkGetToken2(token, LEFT_BRACE, LEFT_BRACE_STRING);
  block();
}

/* ブロックのコンパイル */
static void block(void)
{

  while (1) {
    if (token.kind == RIGHT_BRACE
        || token.kind == RIGHT_BRACE_STRING) {
      /* ブロック終了記号を見たら終了 */
      token = nextToken();
      break;
    }
    /* 変数定義, 定義定義, 文の並び */
    switch (token.kind) {
      case VAR:
        token = nextToken();
        varDecl();
        continue;
      case CONST:
        token = nextToken();
        constDecl();
        continue;
      default:
        statement();
        continue;
    } 
  }

}

/* 文のコンパイル */
static void statement(void)
{

  switch (token.kind) {
    case IF:
      /* if文のコンパイルへ */
      token = nextToken();
      if_statement();
      break;
    case SWITCH:
      /* switch文のコンパイルへ */
      token = nextToken();
      switch_statement();
      break;
    case FOR:
      /* for文のコンパイルへ */
      token = nextToken();
      for_statement();
      break;
    case WHILE:
      /* while文のコンパイルへ */
      token = nextToken();
      while_statement();
      break;
    case DO:
      /* do while文のコンパイルへ */
      token = nextToken();
      do_while_statement();
      break;
    case BREAK:
      /* break文のコンパイル */
      /* expression -> (;)の並び */
      token = nextToken();
      /* ';'は省略可能に */
      if (token.kind == SEMICOLON) {
        token = nextToken();
      }
      /* token = checkGetToken(token, SEMICOLON); */
      break;
    case RETURN:
      /* return文のコンパイル */
      token = nextToken();
      /* expression -> (;)の並び */
      expression();
      /* ';'は省略可能に */
      if (token.kind == SEMICOLON) {
        token = nextToken();
      }
      /* token = checkGetToken(token, SEMICOLON); */
      break;
    case LEFT_BRACE:        /* FALLTHRU */
    case LEFT_BRACE_STRING:
      /* ブロックのコンパイルへ */
      token = nextToken();
      block();
      break;
    default:
      /* それ以外は式からなる文と判定 */
      comma_expression();
      break;
  }

}

/* if文のコンパイル */
static void if_statement(void)
{
  /* ( -> expression -> ) -> block の並びでコンパイル */
  token = checkGetToken(token, LEFT_PARLEN);
  expression();
  token = checkGetToken(token, RIGHT_PARLEN);
  token = checkGetToken2(token, LEFT_BRACE, LEFT_BRACE_STRING);
  block();

  /* elsifリストのコンパイル */
  while (token.kind == ELSIF) {
    /* elsifを見たらelsifブロックのコンパイルへ */
    token = checkGetToken(nextToken(), LEFT_PARLEN);
    expression();
    token = checkGetToken(token, RIGHT_PARLEN);
    token = checkGetToken2(token, LEFT_BRACE, LEFT_BRACE_STRING);
    block();
  }

  /* else節（無くても可）のコンパイル */
  if (token.kind == ELSE) {
    token = nextToken();
    token = checkGetToken2(token, LEFT_BRACE, LEFT_BRACE_STRING);
    block();
  }

}

/* switch文のコンパイル */
static void switch_statement(void)
{
  /* ( -> expression -> ) -> { の並びを確認 */
  token = checkGetToken(token, LEFT_PARLEN);
  expression();
  token = checkGetToken(token, RIGHT_PARLEN);
  token = checkGetToken2(token, LEFT_BRACE, LEFT_BRACE_STRING);

  /* case節の並び */
  while (token.kind == CASE) {
    /* caseを読んだら, expression, expression, ... -> : -> statement 
     * の順にコンパイル */
    token = nextToken();
    expression();
    /* expression -> , の並び */
    while (token.kind == COMMA) {
      token = nextToken();
      expression();
    }
    token = checkGetToken(token, COLON);
    /* 処理内容の文リスト */
    while (token.kind != CASE
           && token.kind != DEFAULT
           && token.kind != RIGHT_BRACE
           && token.kind != RIGHT_BRACE_STRING) {
      statement();
    }
  }

  /* default節のコンパイル */
  if (token.kind == DEFAULT) {
    /* : -> statement(処理内容) の順にコンパイル */
    token = checkGetToken(nextToken(), COLON);
    while (token.kind != RIGHT_BRACE
           && token.kind != RIGHT_BRACE_STRING) {
      statement();
    }
  }

  /* 終わりの}を確認 */
  token = checkGetToken2(token, RIGHT_BRACE, RIGHT_BRACE_STRING);

}

/* for文のコンパイル */
static void for_statement(void)
{
  /* ( を確認 */
  token = checkGetToken(token, LEFT_PARLEN);

  /* 初期化式のコンパイル : 空でも可能 */
  if (token.kind != SEMICOLON) {
    comma_expression();
  }
  /* セミコロンのチェック */
  token = checkGetToken(token, SEMICOLON);

  /* 条件式のコンパイル : 空でも可能 */
  if (token.kind != SEMICOLON) {
    comma_expression();
  }
  /* セミコロンのチェック */
  token = checkGetToken(token, SEMICOLON);

  /* 更新式のコンパイル : 空でも可能 */
  if (token.kind != RIGHT_PARLEN) {
    comma_expression();
  }
  /* )のチェック */
  token = checkGetToken(token, RIGHT_PARLEN);

  /* 処理内容のコンパイル */
  token = checkGetToken2(token, LEFT_BRACE, LEFT_BRACE_STRING);
  block();

}

/* while文のコンパイル */
static void while_statement(void)
{
  /* ( -> expression -> ) -> block の順にコンパイル */
  token = checkGetToken(token, LEFT_PARLEN);
  expression();
  token = checkGetToken(token, RIGHT_PARLEN);
  token = checkGetToken2(token, LEFT_BRACE, LEFT_BRACE_STRING);
  block();
}

/* do while文のコンパイル */
static void do_while_statement(void)
{
  /* block -> while -> ( -> expression -> ) 
   * の順にコンパイル */
  token = checkGetToken2(token, LEFT_BRACE, LEFT_BRACE_STRING);
  block();
  token = checkGetToken(token, WHILE);
  token = checkGetToken(token, LEFT_PARLEN);
  expression();
  token = checkGetToken(token, RIGHT_PARLEN);
}

/* コンマ式のコンパイル
 * コンマ式も式だが, 仮引数との兼ね合いが難しいので分割 */
static void comma_expression(void)
{
  /* まず式を読む */
  expression();
  /* , expressionの並び */
  while (token.kind == COMMA) {
    token = nextToken();
    expression();
  }
}

/* 式のコンパイル */
static void expression(void)
{
  /* まず, 論理OR式を読む */
  logical_or_expression();
  /* 代入演算子 -> 論理OR式の並び */
  while (token.kind == ASSIGN
         || token.kind == ADD_ASSIGN
         || token.kind == SUB_ASSIGN
         || token.kind == MUL_ASSIGN
         || token.kind == DIV_ASSIGN
         || token.kind == MOD_ASSIGN) {
    token = nextToken();
    logical_or_expression();
  }  
}

/* 論理OR式のコンパイル */
static void logical_or_expression(void)
{
  /* まず, 論理AND式を読む */
  logical_and_expression();
  /* 論理OR -> 論理AND式の並び */
  while (token.kind == LOGICAL_OR
         || token.kind == LOGICAL_OR_STRING) {
    token = nextToken();
    logical_and_expression();
  }
}

/* 論理AND式のコンパイル */
static void logical_and_expression(void)
{
  /* まず, 等号/不等号式を読む */
  equal_expression();
  /* 論理AND -> 等号/不等号式の並び */
  while (token.kind == LOGICAL_AND
         || token.kind == LOGICAL_AND_STRING) {
    token = nextToken();
    equal_expression();
  }
}

/* 等号/非等号式のコンパイル */
static void equal_expression(void)
{
  /* まず, 比較式を読む */
  compare_expression();
    /* 等号/非等号 -> 不等式の並び */
  while (token.kind == EQUAL
         || token.kind == NOT_EQUAL) {
    token = nextToken();
    compare_expression();
  }
}

/* 不等式のコンパイル */
static void compare_expression(void)
{
  /* まず, 加算式を読む */
  add_expression();
  /* 不等号 -> 加算式の並び */
  while (token.kind == GREATER
         || token.kind == GREATER_EQUAL
         || token.kind == LESSTHAN
         || token.kind == LESSTHAN_EQUAL) {
    token = nextToken();
    add_expression();
  }
}

/* 加算式のコンパイル */
static void add_expression(void)
{
  /* まず, 積算式を読む */
  mul_expression();
  /* +,- -> 積算式の並び */
  while (token.kind == PLUS || token.kind == MINUS) {
    token = nextToken();
    mul_expression();
  }
}

/* 積算式のコンパイル */
static void mul_expression(void)
{
  /* まず, 累乗式を読む */
  power_expression();
  /* *,/,%,mod -> 累乗式の並び */
  while (token.kind == MUL
         || token.kind == DIV
         || token.kind == MOD
         || token.kind == MOD_STRING) {
      token = nextToken();
      power_expression();
  } 
}

/* 累乗式のコンパイル */
static void power_expression(void)
{
  /* まず, 単項式を読む */
  unary_expression();
  /* ** -> 単項式の並び */
  while (token.kind == POWER) {
      token = nextToken();
      unary_expression();
  } 
}

/* 単項式のコンパイル */
static void unary_expression(void)
{
  /* 項の前に付く演算子（無くても可） !,not,+,- */
  if (token.kind == LOGICAL_NOT
      || token.kind == LOGICAL_NOT_STRING
      || token.kind == PLUS
      || token.kind == MINUS) {
    token = nextToken();
  }

  /* 項のコンパイル */
  term();

  /* 項の後に付く演算子（無くても可） ++,-- */
  if (token.kind == INCREMENT
      || token.kind == DECREMENT) {
    token = nextToken();
  }

}

/* 項のコンパイル */
static void term(void)
{
  /* トークンの種類で場合分け */
  switch (token.kind) {
    /* 変数/定数参照, 関数呼び出し, 配列参照
     * -> 記号表から判断して構文解析 */
    case IDENTIFIER:
      token = nextToken();
      break;
      /* リテラル : その場でコード生成？ */
    case INT_LITERAL:     /* FALLTHRU */
    case DOUBLE_LITERAL:  /* FALLTHRU */
    case STRING_LITERAL:  /* FALLTHRU */
    case TRUE_LITERAL:    /* FALLTHRU */
    case FALSE_LITERAL:
      token = nextToken();
      break;
      /* (comma_expression) のコンパイル */
    case LEFT_PARLEN:
      token = nextToken();
      comma_expression();
      token = checkGetToken(token, RIGHT_PARLEN);
      break;
      /* 不正なトークン */
    case OTHERS:
      compile_error(INVAILD_CHAR_ERROR, line_number);
      break;
      /* それ以外の有効なトークンは構文エラー */
    default:
      printf("%s \n", token.u.identifier);
      compile_error(SYNTAX_ERROR, line_number);
  }
}
