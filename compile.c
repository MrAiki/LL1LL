#include "compile.h"

#define FIRST_LOCAL_ADDRESS (2) /* 各ブロックの最初のローカル変数のアドレス:0にはディスプレイが指すアドレス, 1には戻りアドレスが入る */

static Token token;               /* 次のトークン */

/* コンパイル・モジュール群 */
static void toplevel(void);               /* トップレベルのコンパイル */
static void varDecl(void);                /* 変数定義のコンパイル */
static void constDecl(void);              /* 定数定義のコンパイル */
static void funcDecl(void);               /* 関数定義のコンパイル */
static void statement(void);              /* 文のコンパイル */
static void assign_statement(void);        /* 代入文のコンパイル */
static void if_statement(void);           /* if文のコンパイル */
static void switch_statement(void);       /* switch文のコンパイル */
static void for_statement(void);          /* for文のコンパイル */
static void while_statement(void);        /* while文のコンパイル */
static void do_while_statement(void);     /* do while文のコンパイル */
static void block(void);                  /* ブロックのコンパイル */
static void comma_expression(void);       /* コンマ式のコンパイル */
static void expression(void);             /* 式のコンパイル */
// static void logical_or_expression(void);  /* 論理OR式のコンパイル */
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
  initSource();                        /* 字句解析の準備 */
  token = nextToken();                 /* 最初の先読みトークンを読む */
  blockBegin(FIRST_LOCAL_ADDRESS, 0);  /* スタック型記憶域の初期化も兼ねてブロックをセット */
  toplevel();                          /* トップレベルからコンパイル */

  return 0;   /* TODO: エラー個数を返すようにする */
}

/* トップレベルのコンパイル */
static void toplevel(void)
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
  /* 値取得用 */
  LL1LL_Value value_temp;
  /* 名前取得用 */
  Token name_temp;
  /* 代入用の記号表インデックス */
  int table_index;

  while (1) {
    /* IDENTIFIER -> ASSIGNの並びを確認 */
    token = checkGetToken(token, IDENTIFIER);
    name_temp = token;
    addTableVar(name_temp.u.identifier);      /* 名前表に登録 */

    /* 代入が来たらば宣言と同時に値を代入する */
    if (token.kind == ASSIGN) {
      token = nextToken();

      /* 代入する値で場合分け */
      switch (token.kind) {
        case INT_LITERAL:       /* 整数リテラル */
          value_temp.type        = LL1LL_INT_TYPE;
          value_temp.u.int_value = token.u.int_value;
          break;
        case DOUBLE_LITERAL:    /* 倍精度実数リテラル */
          value_temp.type        = LL1LL_DOUBLE_TYPE;
          value_temp.u.int_value = token.u.double_value;
          break;
        case STRING_LITERAL:    /* 文字列リテラル */
          value_temp.type           = LL1LL_OBJECT_TYPE;
          value_temp.u.object->type = STRING_OBJECT;
          value_temp.u.object       = alloc_string(token.u.string_value, LL1LL_TRUE);
          break;
        case TRUE_LITERAL:      /* 論理値リテラル */
          value_temp.type            = LL1LL_BOOLEAN_TYPE;
          value_temp.u.boolean_value = LL1LL_TRUE;
          break;
        case FALSE_LITERAL:
          value_temp.type            = LL1LL_BOOLEAN_TYPE;
          value_temp.u.boolean_value = LL1LL_FALSE;
          break;
        default:                /* それ以外はシンタックスエラー */
          compile_error(SYNTAX_ERROR, line_number);
      }

      /* 今宣言した変数の探索 */
      table_index = searchTable(name_temp.u.identifier, VAR_IDENTIFIER);
      /* リテラルの値をスタックにプッシュし, 変数にポップ */
      genCodeValue(LVM_PUSH_IMMEDIATE, value_temp);
      genCodeTable(LVM_POP_VARIABLE, table_index);

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
  /* => ';'は省略可能に */
  if (token.kind == SEMICOLON) {
    token = nextToken();
  }
}

/* 定数定義のコンパイル */
static void constDecl(void)
{
  /* 識別子取得用のトークン */
  Token name_temp;
  /* 値取得用 */
  LL1LL_Value value_temp;

  while (1) {
    /* IDENTIFIER -> ASSIGNの並びを確認 */
    if (token.kind == IDENTIFIER) {
      name_temp = token;
    } else {
      fprintf(stderr, "Error missing identifier in constDecl \n");
      exit(1);
    }
    token = checkGetToken(token, ASSIGN);

    /* 代入する値で場合分け */
    switch (token.kind) {
      case INT_LITERAL:       /* 整数リテラル */
        value_temp.type        = LL1LL_INT_TYPE;
        value_temp.u.int_value = token.u.int_value;
        break;
      case DOUBLE_LITERAL:    /* 倍精度実数リテラル */
        value_temp.type        = LL1LL_DOUBLE_TYPE;
        value_temp.u.int_value = token.u.double_value;
        break;
      case STRING_LITERAL:    /* 文字列リテラル */
        value_temp.type           = LL1LL_OBJECT_TYPE;
        value_temp.u.object->type = STRING_OBJECT;
        value_temp.u.object       = alloc_string(token.u.string_value, LL1LL_TRUE);
        break;
      case TRUE_LITERAL:      /* 論理値リテラル */
        value_temp.type            = LL1LL_BOOLEAN_TYPE;
        value_temp.u.boolean_value = LL1LL_TRUE;
        break;
      case FALSE_LITERAL:
        value_temp.type            = LL1LL_BOOLEAN_TYPE;
        value_temp.u.boolean_value = LL1LL_FALSE;
        break;
      default:                /* それ以外はシンタックスエラー */
        compile_error(SYNTAX_ERROR, line_number);
    }

    /* 記号表に登録 */
    addTableConst(name_temp.u.identifier, value_temp);
    token = nextToken();

    /* 次にコンマが出ていたら変数定義が続く */
    if (token.kind == COMMA) {
      token = nextToken();
      continue;
    } else {
      break;
    }

  }

  /* 最後は;で終わらなければならないが, ';'は省略可能に */
  /* token = checkGetToken(token, SEMICOLON); */
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

  int tmp_label;          /* break, continue用の仮ラベル */
  int table_index;        /* 代入文用のテーブルインデックス */

  switch (token.kind) {
    case IDENTIFIER:
      /* 代入文のコンパイルへ */
      table_index = searchTable(token.u.identifier, VAR_IDENTIFIER);
      assign_statement(table_index);
      token = nextToken();
      assign_statement();
      break;
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
    case CONTINUE:
      /* continue文のコンパイル */
      tmp_label = genCodeJump(LVM_JUMP, 0);
      addContinueLabel(tmp_label);
      token = nextToken();
      /* ';'は省略可能に */
      if (token.kind == SEMICOLON) {
        token = nextToken();
      }
      /* token = checkGetToken(token, SEMICOLON); */
      break;
    case BREAK:
      /* break文のコンパイル */
      tmp_label = genCodeJump(LVM_JUMP, 0);
      addBreakLabel(tmp_label);
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

/* 代入文のコンパイル */
static void assign_statement(int table_index)
{
  /* 演算子の種類 */
  Token_kind op_kind;
  /* 識別子の種類 */
  IdentifierKind id_kind = getTableKind(table_index);

  /* 代入演算子 -> 式 */
  if (token.kind == ASSIGN
      || token.kind == ADD_ASSIGN
      || token.kind == SUB_ASSIGN
      || token.kind == MUL_ASSIGN
      || token.kind == DIV_ASSIGN
      || token.kind == MOD_ASSIGN) {
    op_kind = token.kind;

    /* 定数とか関数の識別子には代入できない */
    if (id_kind != VAR_IDENTIFIER
        && id_kind != PARAM_IDENTIFIER) {
      fprintf(stderr, "Error! Cannot assign to constant/function identifier \n");
      exit(1);
    }
    
    /* 代入演算を行う場合は, まず変数値をスタックに積む */
    if (op_kind != ASSIGN) {
      genCodeTable(LVM_PUSH_VALUE, table_index);
    }
    
    /* 右辺値（代入する値）のコンパイル */
    token = nextToken();
    comma_expression();

    /* 代入演算の種類で場合分けし, 演算を行う */
    switch (op_kind) {
      case ASSIGN:
        break;
      case ADD_ASSIGN:
        genCodeCalc(LVM_ADD);
        break;
      case SUB_ASSIGN:
        genCodeCalc(LVM_SUB);
        break;
      case MUL_ASSIGN:
        genCodeCalc(LVM_MUL);
        break;
      case ADD_ASSIGN:
        genCodeCalc(LVM_DIV);
        break;
      case MOD_ASSIGN:
        genCodeCalc(LVM_MOD);
        break;
    }
    
    /* 代入 */
    genCodeTable(LVM_POP_VARIABLE, table_index);

  } else {
    /* 代入文ではなかった => 式のコンパイルへ移行 */
    comma_expression();
  }

}

/* if文のコンパイル */
static void if_statement(void)
{
  int if_false_label, *if_end_labels;   /* バックパッチラベル. 終わりに飛び越すラベルの個数は分からないので可変長で */
  int if_count = 0, if_i;               /* ifリストの数 */ 
  /* ( -> expression -> ) -> block の並びでコンパイル */
  token = checkGetToken(token, LEFT_PARLEN);
  expression();
  token = checkGetToken(token, RIGHT_PARLEN);

  /* 最初の条件が満たされない場合の飛び先の
   * ラベルを得る. bp_labelには現在のpcが入る */
  if_false_label = genCodeJump(LVM_JUMP_IF_FALSE, 0);

  /* 処理内容のコンパイル */
  token = checkGetToken2(token, LEFT_BRACE, LEFT_BRACE_STRING);
  while (token.kind != RIGHT_BRACE
         || token.kind != RIGHT_BRACE_STRING) {
    statement();
  }

  /* 中身を実行した -> if文の終わりに飛び越す
   * 終わりに飛び越すラベルの追加 */
  if_count++;
  if_end_labels    = (int *)MEM_realloc(if_end_labels, sizeof(int) * if_count);
  if_end_labels[0] = genCodeJump(LVM_JUMP, 0);


  /* 最初の条件が満たされない場合の飛び先にバックパッチ */
  backPatch(if_false_label);

  /* elsifリストのコンパイル */
  while (token.kind == ELSIF) {
    /* elsifを見たらelsifブロックのコンパイルへ */
    token = checkGetToken(nextToken(), LEFT_PARLEN);
    expression();
    token = checkGetToken(token, RIGHT_PARLEN);

    /* elsifの条件が満たされない場合の飛び先の
     * ラベルを得る. bp_labelには現在のpc */
    if_false_label = genCodeJump(LVM_JUMP_IF_FALSE, 0);

    token = checkGetToken2(token, LEFT_BRACE, LEFT_BRACE_STRING);
    block();

    /* 中身を実行した -> if文の終わりに飛び越す */
    if_count++;
    if_end_labels    = (int *)MEM_realloc(if_end_labels, sizeof(int) * if_count);
    if_end_labels[if_count-1] = genCodeJump(LVM_JUMP, 0);

    /* elsif条件が満たされない場合の飛び先にバックパッチ */
    backPatch(if_false_label);

  }

  /* else節（無くても可）のコンパイル */
  if (token.kind == ELSE) {
    /* jumpは不要. ここまで来たら無条件に実行 */
    token = nextToken();
    token = checkGetToken2(token, LEFT_BRACE, LEFT_BRACE_STRING);
    block();
  }

  /* if文の終わりの飛び先にバックパッチ */
  for (if_i = 0; if_i < if_count; if_i++) {
    backPatch(if_end_labels[if_i]);
  }
  MEM_free(if_end_labels);

}

/* switch文のコンパイル */
static void switch_statement(void)
{
  int *true_labels, nextcase_label, *endcase_labels;
  int one_case_count = 0, onecase_i;
  int case_count = 0, case_i;

  /* ( -> expression -> ) -> { の並びを確認 */
  token = checkGetToken(token, LEFT_PARLEN);
  expression();
  token = checkGetToken(token, RIGHT_PARLEN);
  token = checkGetToken2(token, LEFT_BRACE, LEFT_BRACE_STRING);

  /* case節の並び */
  while (token.kind == CASE) {
    one_case_count = 0;

    /* caseを読んだら, expression, expression, ... -> : -> statement 
     * の順にコンパイル */
    genCodeCalc(LVM_DUPLICATE); /* 判定式を複製 */
    token = nextToken();
    expression();               /* 一つのケースを生成 */

    genCodeCalc(LVM_EQUAL);     /* ケース判定 */
    one_case_count++;
    true_labels    = (int *)MEM_realloc(sizeof(int) * one_case_count);
    true_labels[0] = genCodeJump(LVM_JUMP_IF_TRUE, 0);

    /* expression -> , の並び */
    while (token.kind == COMMA) {
      genCodeCalc(LVM_DUPLICATE); /* 判定式を複製 */
      token = nextToken();
      expression();               /* 一つのケースを生成 */

      genCodeCalc(LVM_EQUAL);     /* ケース判定 */
      one_case_count++;
      true_labels = (int *)MEM_realloc(sizeof(int) * one_case_count);
      true_labels[one_case_count-1] = genCodeJump(LVM_JUMP_IF_TRUE, 0);
    }
    token = checkGetToken(token, COLON);

    /* どのケースにも合致しなかった場合は, 次のケースへ飛び越す */
    nextcase_label = genCodeJump(LVM_JUMP, 0);

    /* ケースに合致した時の飛び先ラベルにバックパッチ */
    for (onecase_i = 0;
         onecase_i < one_case_count;
         onecase_i++) {
      backPatch(true_labels[onecase_i]);
    }
    MEM_free(true_labels);

    /* 処理内容の文リスト */
    while (token.kind != CASE
           && token.kind != DEFAULT
           && token.kind != RIGHT_BRACE
           && token.kind != RIGHT_BRACE_STRING) {
      statement();
    }

    /* 処理を行った場合の終わりに飛び越す命令 */
    case_count++;
    endcase_labels               = (int *)MEM_realloc(sizeof(int) * case_count);
    endcase_labels[case_count-1] = genCodeJump(LVM_JUMP, 0);

    /* 次のケースへ飛び越すラベルにバックパッチ */
    backPatch(nextcase_label);

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

  /* caseの終わりに飛び越す命令にバックパッチ */
  for (case_i = 0; 
       case_i < case_count;
       case_i++) {
    backPatch(endcase_labels[case_i]);
  }
  MEM_free(endcase_labels);

}

/* for文のコンパイル */
static void for_statement(void)
{
  /* ループ時の飛び先ラベルと, for文の終わりに飛び越すラベル */
  int for_loop_label, for_end_label;
  /* ( を確認 */
  token = checkGetToken(token, LEFT_PARLEN);

  /* 初期化式のコンパイル : 空でも可能 */
  if (token.kind != SEMICOLON) {
    comma_expression();
  }
  /* セミコロンのチェック */
  token = checkGetToken(token, SEMICOLON);

  /* ループ時の飛び先 */
  for_loop_label = nextCode();

  /* 条件式のコンパイル : 空でも可能 */
  if (token.kind != SEMICOLON) {
    expression();
  }
  /* セミコロンのチェック */
  token = checkGetToken(token, SEMICOLON);
  /* 偽ならば終わりに飛び越す */
  for_end_label  = genCodeJump(LVM_JUMP_IF_FALSE, 0);

  /* 更新式のコンパイル : 空でも可能 */
  if (token.kind != RIGHT_PARLEN) {
    comma_expression();
  }
  /* )のチェック */
  token = checkGetToken(token, RIGHT_PARLEN);

  /* 処理内容のコンパイル */
  token = checkGetToken2(token, LEFT_BRACE, LEFT_BRACE_STRING);
  block();

  /* ループの先頭に飛ぶ */
  genCodeJump(LVM_JUMP, for_loop_label);

  /* for文の終わりにバックパッチ */
  backPatch(for_end_label);
  /* continue文のバックパッチ */
  backPatchContinueLabels(for_loop_label);
  /* break文のバックパッチ */
  backPatchBreakLabels();

}

/* while文のコンパイル */
static void while_statement(void)
{
  int while_loop_label, while_end_label;

  /* ループ先のラベルにセット */
  while_loop_label = nextCode();
  /* ( -> expression -> ) -> block の順にコンパイル */
  token = checkGetToken(token, LEFT_PARLEN);
  expression();
  token = checkGetToken(token, RIGHT_PARLEN);
  /* while文の終わりに飛び越す命令 */
  while_end_label = genCodeJump(LVM_JUMP_IF_FALSE, 0);

  token = checkGetToken2(token, LEFT_BRACE, LEFT_BRACE_STRING);
  block();
  /* ループの先頭にジャンプ */
  genCodeJump(LVM_JUMP, while_loop_label);

  /* while文の終わりに飛び越す命令にバックパッチ */
  backPatch(while_end_label);
  /* continue文のバックパッチ */
  backPatchContinueLabels(while_loop_label);
  /* break文のバックパッチ */
  backPatchBreakLabels();
}

/* do while文のコンパイル */
static void do_while_statement(void)
{
  int do_while_loop_label;
  /* block -> while -> ( -> expression -> ) 
   * の順にコンパイル */

  /* ループの先頭のラベルをセット */
  do_while_loop_label = nextCode();

  token = checkGetToken2(token, LEFT_BRACE, LEFT_BRACE_STRING);
  block();
  token = checkGetToken(token, WHILE);
  token = checkGetToken(token, LEFT_PARLEN);
  expression();
  token = checkGetToken(token, RIGHT_PARLEN);

  /* ループの先頭に飛び越す命令へバックパッチ */
  genCodeJump(LVM_JUMP_IF_TRUE, do_while_loop_label);
  /* continue文のバックパッチ */
  backPatchContinueLabels(do_while_loop_label);
  /* break文のバックパッチ */
  backPatchBreakLabels();
}

/* コンマ式のコンパイル
 * コンマ式も式だが, 仮引数との兼ね合いが難しいので分割 */
static void comma_expression(void)
{
  /* まず式を読む */
  expression();
  genCodeCalc(LVM_POP); 
  /* , expressionの並び */
  while (token.kind == COMMA) {
    token = nextToken();
    expression();
    if (token.kind == COMMA) {
    /* 一番右の式の値以外は捨てる
     * => 評価値は一番右の式のみ */
    genCodeCalc(LVM_POP); 
    } else {
      break;
    }
  }
}

/* 論理OR式（式）のコンパイル */
static void expression(void)
{
  /* まず, 論理AND式を読む */
  logical_and_expression();
  /* 論理OR -> 論理AND式の並び */
  while (token.kind == LOGICAL_OR
         || token.kind == LOGICAL_OR_STRING) {
    token = nextToken();
    logical_and_expression();
    genCodeCalc(LVM_LOGICAL_OR);
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
    genCodeCalc(LVM_LOGICAL_AND);
  }
}

/* 等号/非等号式のコンパイル */
static void equal_expression(void)
{
  /* 演算子の種類 */
  Token_kind op_kind;
  /* まず, 比較式を読む */
  compare_expression();
    /* 等号/非等号 -> 不等式の並び */
  while (token.kind == EQUAL
         || token.kind == NOT_EQUAL) {
    op_kind = token.kind;
    token   = nextToken();
    compare_expression();

    /* 演算子の種類で場合分けしコード生成 */
    switch (op_kind) {
      case EQUAL:
        genCodeCalc(LVM_EQUAL);
        break;
      case NOT_EQUAL:
        genCodeCalc(LVM_NOT_EQUAL);
        break;
    }
  }

}

/* 不等式のコンパイル */
static void compare_expression(void)
{
  /* 演算子の種類 */
  Token_kind op_kind;
  /* まず, 加算式を読む */
  add_expression();
  /* 不等号 -> 加算式の並び */
  while (token.kind == GREATER
         || token.kind == GREATER_EQUAL
         || token.kind == LESSTHAN
         || token.kind == LESSTHAN_EQUAL) {
    op_kind = token.kind;
    token   = nextToken();
    add_expression();

    /* 演算子の種類で場合分けしコード生成 */
    switch (op_kind) {
      case GREATER:
        genCodeCalc(LVM_GREATER);
        break;
      case GREATER_EQUAL:
        genCodeCalc(LVM_GREATER_EQUAL);
        break;
      case LESSTHAN:
        genCodeCalc(LVM_LESSTHAN);
        break;
      case LESSTHAN_EQUAL:
        genCodeCalc(LVM_LESSTHAN_EQUAL);
        break;
    }
  }

}

/* 加算式のコンパイル */
static void add_expression(void)
{
  /* 演算子の種類 */
  Token_kind op_kind;
  /* まず, 積算式を読む */
  mul_expression();
  /* +,- -> 積算式の並び */
  while (token.kind == PLUS || token.kind == MINUS) {
    op_kind = token.kind;
    token   = nextToken();
    mul_expression();

    /* 演算子の種類で場合分けしコード生成*/
    switch (op_kind) {
      case PLUS:
        genCodeCalc(LVM_ADD);
        break;
      case MINUS:
        genCodeCalc(LVM_SUB);
        break;
    }
  }

}

/* 積算式のコンパイル */
static void mul_expression(void)
{
  /* 演算子の種類 */
  Token_kind op_kind;
  /* まず, 累乗式を読む */
  power_expression();
  /* *,/,%,mod -> 累乗式の並び */
  while (token.kind == MUL
      || token.kind == DIV
      || token.kind == MOD
      || token.kind == MOD_STRING) {
    op_kind = token.kind;
    token   = nextToken();
    power_expression();

    /* 演算子の種類で場合分けし, コード生成 */
    switch (op_kind) {
      case MUL:
        genCodeCalc(LVM_MUL);
        break;
      case DIV:
        genCodeCalc(LVM_DIV);
        break;
      case MOD:        /* FALLTHRU */
      case MOD_STRING:
        genCodeCalc(LVM_MOD);
        break;
    }
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
      genCodeCalc(LVM_POW);
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
