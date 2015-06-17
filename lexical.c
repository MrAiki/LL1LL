#include "lexical.h"

static char source_line_string[MAX_LEN_SOURCE_LINE];  /* fgetsで取得した, 現在行の文字列バッファ */
static int line_index;  /* 現在行の文字インデックス */
int line_number;        /* 現在の行数 */
static char nextchar;   /* 次の文字が入っているバッファ */

static Token prev_token;    /* 一つ前のトークン */
static Token current_token; /* 今読んでいるトークン */

/* 予約語文字列と予約語種類の対応テーブルのエントリ */
typedef struct keyword_table_entry_tag {
  char *keyword_string; /* 文字列 */
  Token_kind kind;      /* 種類 */
} Keyword_table_entry;

/* 予約語文字列/種類対応テーブル */
static Keyword_table_entry keyword_table[] = {
  {"var", VAR}, {"const", CONST}, {"func", FUNC},
  {"int", INT_TYPE}, {"double", DOUBLE_TYPE},
  {"string", STRING_TYPE}, {"bool", BOOLEAN_TYPE},
  {"for", FOR}, {"while", WHILE}, {"do", DO},
  {"continue", CONTINUE}, {"break", BREAK}, {"return", RETURN},
  {"if", IF}, {"elsif", ELSIF}, {"else", ELSE},
  {"switch", SWITCH}, {"case", CASE}, {"default", DEFAULT},
  {"null", NULL_LITERAL},
  {"true", TRUE_LITERAL}, {"false", FALSE_LITERAL},
  {"mod", MOD_STRING},
  {"begin", LEFT_BRACE_STRING}, {"end", RIGHT_BRACE_STRING},
  {"or", LOGICAL_OR_STRING}, {"and", LOGICAL_AND_STRING},
  {"not", LOGICAL_NOT_STRING},
  {"$dummy_end_of_keyword", END_OF_KEYWORD},
  {"=", ASSIGN}, {"+=", ADD_ASSIGN}, {"-=", SUB_ASSIGN},
  {"*=", MUL_ASSIGN}, {"/=", DIV_ASSIGN}, {"%=", MOD_ASSIGN},
  {",", COMMA}, {";", SEMICOLON}, {":", COLON}, {".", PERIOD},
  {"(", LEFT_PARLEN}, {")", RIGHT_PARLEN},
  {"{", LEFT_BRACE}, {"}", RIGHT_BRACE},
  {"[", LEFT_BRAKET}, {"]", RIGHT_BRAKET},
  {"|", PIPE}, {"&", AMP},
  {"||", LOGICAL_OR}, {"&&", LOGICAL_AND},
  {"==", EQUAL}, {"!=", NOT_EQUAL},
  {">", GREATER}, {">=", GREATER_EQUAL},
  {"<", LESSTHAN}, {"<=", LESSTHAN_EQUAL},
  {"+", PLUS}, {"-", MINUS}, {"*", MUL}, {"/", DIV}, {"%", MOD}, {"!", LOGICAL_NOT},
  {"**", POWER},
  {"++", INCREMENT}, {"--", DECREMENT},
  {"\"", DOUBLE_QUOTE},
  {"$dummy_end_of_keysymbol", END_OF_KEYSYMBOL},
};

/* 文字と文字種類の対応 : 突っ込んだ文字に対応した種類が得られる
 * ex) char_kind['a'] == LETTER 
 * 関数でも同じことはできるが, 配列形式の方が早い */
static Token_kind char_kind[256]; /* ascii文字列は8bit256文字 */

/* 次のトークンを返す */
static Token st_nextToken(void);

/* ソースファイルのオープン 
 * オープンに成功すれば0を, 失敗すれば1を返す */
int openSource(char *filename)
{
  input_source = fopen(filename, "r");
  if (input_source == NULL) {
    fprintf(stderr, "file %s can't open \n", filename);
    return 0;
  }
  return 1;
}

/* ソースファイルのファイルのクローズ, その他後処理 */
void closeSource(void)
{
  fclose(input_source);
}
  
/* 文字/種類対応テーブルの初期化 */
static void init_char_kind(void)
{
  int i;  /* これはintの方が良い. charだと下の256で回しきってしまう */

  /* まずはOTHERSで埋める */
  for (i = 0; i < 256; i++) {
    char_kind[i] = OTHERS;
  }

  /* 英文字[a-Z], 数字[0-9]のセット */
  for (i = 0; i < 256; i++) {
    if (isalpha(i)) {
      /* 英文字 */
      char_kind[i] = LETTER;
    } else if (isdigit(i)) {
      /* 数字 */
      char_kind[i] = DIGIT;
    }
  }

  /* 1文字で判別出来る終端文字の種類を設定 */
  char_kind['='] = ASSIGN;
  char_kind[','] = COMMA; char_kind[';'] = SEMICOLON;
  char_kind[':'] = COLON; char_kind['.'] = PERIOD;
  char_kind['('] = LEFT_PARLEN; char_kind[')'] = RIGHT_PARLEN;
  char_kind['{'] = LEFT_BRACE; char_kind['}'] = RIGHT_BRACE;
  char_kind['['] = LEFT_BRAKET; char_kind[']'] = RIGHT_BRAKET;
  char_kind['|'] = PIPE; char_kind['&'] = AMP;
  char_kind['>'] = GREATER; char_kind['<'] = LESSTHAN;
  char_kind['+'] = PLUS; char_kind['-'] = MINUS;
  char_kind['*'] = MUL; char_kind['/'] = DIV;
  char_kind['%'] = MOD; char_kind['!'] = LOGICAL_NOT;
  char_kind['"'] = DOUBLE_QUOTE;

}

/* ソースファイルの次の文字を返す */
static char nextChar(void)
{
  char ch;

  /* 改行後初めて読むときは, MAX_LEN_SOURCE_LINEだけソースファイルから持ってくる */
  if (line_index == -1) {
    if (fgets(source_line_string, MAX_LEN_SOURCE_LINE, input_source) != NULL) {
      line_index = 0; /* インデックスは0に */
    } else {
      fprintf(stderr, "End of source file\n");
      return EOF;
    }
  }

  /* 次の文字の取得 */
  ch = source_line_string[line_index++];

  /* 改行を見たらインデックスを-1に, かつ行数の増加 */
  if (ch == '\n') {
    line_index = -1;
    line_number++;
  }
  
  return ch;
}

/* 次のトークンを返す. 字句解析の全て */
static Token st_nextToken(void)
{
  int token_string_index = 0; /* トークン文字のインデックス */
  int table_index;  /* 予約語テーブルのインデックス */
  int is_double = 0;  /* 数字リテラル読み込み時に, '.'を読んだか否か */
  /* 文字列リテラルのバッファ */
  char string_buf[MAX_LEN_STRING_LITERAL];
  /* 識別子文字列のバッファ */
  char identifier_buf[MAX_LEN_IDENTIFIER];
  /* 数字文字列のバッファ */
  char number_buf[MAX_LEN_NUMBER_LITERAL];
  /* 一時的な判定の為の文字種, トークン */
  Token_kind tmp_char_kind;
  Token      temp_token;

  /* 空白, タブ文字, 改行の読み飛ばし */
  while(1) {
    if (nextchar == ' ' || nextchar == '\t' || nextchar == '\n') {
      nextchar = nextChar();
    } else {
      break;
    }
  }

  /* ファイルの末端に達していたら終わり */
  if (feof(input_source)) {
    temp_token.kind = END_OF_FILE;
    return temp_token;
  }

  /* 読み込んだ文字の種類で場合分け, トークンに値を詰める */
  tmp_char_kind = char_kind[(int)nextchar];
  switch (tmp_char_kind) {
    case LETTER:  /* 文字 */
      /* 文字列の読み取り : 数字, 文字以外の文字が出るまで読む */
      do {
        if (token_string_index < MAX_LEN_IDENTIFIER) {
          identifier_buf[token_string_index++] = nextchar;
          nextchar = nextChar();
        }
      } while(char_kind[(int)nextchar] == LETTER 
          || char_kind[(int)nextchar] == DIGIT);

      /* 識別子の最大長を超えている
       * -> 警告を出し, 長さをMAX_LEN_IDENTIFIERに切り詰める */
      if (token_string_index >= MAX_LEN_IDENTIFIER) {
        fprintf(stderr, "Too long identifier. \n");
        token_string_index = MAX_LEN_IDENTIFIER - 1;
      }

      /* 得られた文字列の末尾にナル文字をセット */
      identifier_buf[token_string_index] = '\0';

      /* 予約語かどうかのチェック */
      for (table_index = 0;
           table_index < END_OF_KEYWORD;
           table_index++) {
        /* 一致したら種類をセットし, すぐに返す */
        if (strcmp(identifier_buf, 
                   keyword_table[table_index].keyword_string)
            == 0) {
          temp_token.kind = keyword_table[table_index].kind;
          /* 論理値リテラルはこの場で値を詰める */
          if (temp_token.kind == TRUE_LITERAL) {
            temp_token.u.boolean_value = LL1LL_TRUE;
          } else if (temp_token.kind == FALSE_LITERAL) {
            temp_token.u.boolean_value = LL1LL_FALSE;
          }
          /* printToken(temp_token);  */
          return temp_token;
        }
      }

      /* 予約語では無かった
       * -> ユーザが定義した識別子 */
      temp_token.kind = IDENTIFIER;
      strcpy(temp_token.u.identifier, identifier_buf);
      break;

    case DIGIT: /* 数字 */
      /* 数字リテラルの読み込み */
      do {
        if (token_string_index < MAX_LEN_NUMBER_LITERAL) {
          /* リテラルを読み込んでいく */
          number_buf[token_string_index++] = nextchar;
          nextchar = nextChar();

          /* 次の文字列が'.'ならば, 実数と判定する */
          if (nextchar == '.') {
            /* 2回以上読み込んだらエラー */
            if (is_double) {
              fprintf(stderr, "Syntax error in DIGIT \n");
              exit(1);
            }
            /* 実数フラグを立て, '.'を読み, 次の文字へ */
            is_double = 1;
            number_buf[token_string_index++] = nextchar;
            nextchar = nextChar();
          }
        }
      } while(char_kind[(int)nextchar] == DIGIT);

      /* 得られた文字列の末尾にナル文字をセット */
      number_buf[token_string_index] = '\0';
      /* 桁が大きすぎる場合は, 最早正しく読み込めないので終了 */
      if (token_string_index >= MAX_LEN_NUMBER_LITERAL) {
        fprintf(stderr, "Too large number \n");
        exit(1);
      }

      /* strtod, strtolを使って文字列を数字に変換 */
      if (is_double) {
        temp_token.kind = DOUBLE_LITERAL;
        temp_token.u.double_value = strtod(number_buf, NULL);
      } else {
        temp_token.kind = INT_LITERAL;
        temp_token.u.int_value = strtol(number_buf, NULL, 10);
      }

      break;
      /* 以下, 2文字以上の記号で構成される予約シンボル */
    case PLUS:  /* '+' */
      nextchar = nextChar();
      switch (nextchar) {
        case '=': /* "+=" */
          nextchar = nextChar();
          temp_token.kind = ADD_ASSIGN;
          break;
        case '+': /* "++" */
          nextchar = nextChar();
          temp_token.kind = INCREMENT;
          break;
        default:  /* '+' */
          temp_token.kind = PLUS;
          break;
      }
      break;

    case MINUS: /* '-' */
      nextchar = nextChar();
      switch (nextchar) {
        case '=': /* "-=" */
          nextchar = nextChar();
          temp_token.kind = SUB_ASSIGN;
          break;
        case '-': /* "--" */
          nextchar = nextChar();
          temp_token.kind = DECREMENT;
          break;
        default:  /* '-' */
          temp_token.kind = MINUS;
          break;
      }
      break;

    case MUL: /* '*' */
      nextchar = nextChar();
      switch (nextchar) {
        case '=' : /* "*=" */
          nextchar = nextChar();
          temp_token.kind = MUL_ASSIGN;
          break;
        case '*' : /* "**" */
          nextchar = nextChar();
          temp_token.kind = POWER;
          break;
        default:  /* '*' */
          temp_token.kind = MUL;
          break;
      }
      break;

    case DIV: /* '/' : コメントの読み飛ばしを含む */
      nextchar = nextChar();
      switch (nextchar) {
        case '=' : /* "/=" */
          nextchar = nextChar();
          temp_token.kind = DIV_ASSIGN;
          break;
        case '*' : /* Cスタイルコメント */
          nextchar = nextChar();
          do {
            while (nextchar != '*') {
              nextchar = nextChar();
            }
            nextchar = nextChar();
          } while (nextchar != '/');
          /* nextchar == '/' */
          nextchar = nextChar();
          return nextToken(); /* リトライして次のトークンを得る */
        case '/' : /* "//" : C++スタイルコメント */
          do {
            nextchar = nextChar();
          } while (nextchar != '\n');
          return nextToken();
        default:  /* '/' */
          temp_token.kind = DIV;
          break;
      }
      break;

    case MOD: /* '%' */
      nextchar = nextChar();
      switch (nextchar) {
        case '=' : /* "%=" */
          nextchar = nextChar();
          temp_token.kind = MOD_ASSIGN;
          break;
        default:
          temp_token.kind = MOD;
          break;
      }
      break;

    case PIPE: /* '|' */
      nextchar = nextChar();
      switch (nextchar) {
        case '|' : /* "||" */
          nextchar = nextChar();
          temp_token.kind = LOGICAL_OR;
          break;
        default:
          fprintf(stderr, "Syntax error in PIPE. \n");
          exit(1);
          break;
      }
      break;

    case AMP: /* '&' */
      nextchar = nextChar();
      switch (nextchar) {
        case '&' : /* "&&" */
          nextchar = nextChar();
          temp_token.kind = LOGICAL_AND;
          break;
        default:
          fprintf(stderr, "Syntax error in AMP. \n");
          exit(1);
          break;
      }
      break;

    case ASSIGN: /* '=' */
      nextchar = nextChar();
      switch (nextchar) {
        case '=' : /* "==" */
          nextchar = nextChar();
          temp_token.kind = EQUAL;
          break;
        default:  /* '=' */
          temp_token.kind = ASSIGN;
          break;
      }
      break;

    case LOGICAL_NOT: /* '!' */
      nextchar = nextChar();
      switch (nextchar) {
        case '=' : /* "!=" */
          nextchar = nextChar();
          temp_token.kind = NOT_EQUAL;
          break;
        default:  /* '!' */
          temp_token.kind = LOGICAL_NOT;
          break;
      }
      break;

    case GREATER: /* '>' */
      nextchar = nextChar();
      switch (nextchar) {
        case '=' : /* ">=" */
          nextchar = nextChar();
          temp_token.kind = GREATER_EQUAL;
          break;
        default:  /* '>' */
          temp_token.kind = GREATER;
          break;
      }
      break;

    case LESSTHAN: /* '<' */
      nextchar = nextChar();
      switch (nextchar) {
        case '=' :  /* "<=" */
          nextchar = nextChar();
          temp_token.kind = LESSTHAN_EQUAL;
          break;
        default:
          temp_token.kind = LESSTHAN;
          break;
      }
      break;

    case DOUBLE_QUOTE: /* '"' : 文字列リテラルスタート */
      nextchar = nextChar();
      /* 文字列の読み込み */
      while (nextchar != '"') {
        if (token_string_index < MAX_LEN_STRING_LITERAL) {
          string_buf[token_string_index++] = nextchar;
          /* '\'を読んだときは, 次の文字を強制的に読み込む */
          if (nextchar == '\\') {
            nextchar = nextChar();
            continue;
          }
          nextchar = nextChar();
        }
      }

      /* nextchar == '"' まで読んでいるので, 次を先読み */
      nextchar = nextChar();

      /* 長すぎる文字列を見た時は, 最早正常処理ができないので終了 */
      if (token_string_index >= MAX_LEN_STRING_LITERAL) {
        fprintf(stderr, "Too long string literal. \n");
        exit(1);
      }

      /* 得られた文字列の末尾にナル文字を追加 */
      string_buf[token_string_index] = '\0';

      /* トークンに値を詰める */
      temp_token.kind = STRING_LITERAL;
      strcpy(temp_token.u.string_value, string_buf);
      break;

      /* それ以外は1文字の予約シンボル, 又は無効な文字 */
    default:
      temp_token.kind = tmp_char_kind;
      nextchar = nextChar();
      break;
  }

  /* printToken(temp_token); */
  return temp_token;
}

/* 次のトークンを読む */
Token nextToken(void)
{
  /* 前のトークンを保存し, 今読んでいるトークンを更新する */
  prev_token = current_token;
  current_token = st_nextToken();
  return current_token;
}

/* 1つ前のトークンを返す. ホントは反則技. */
Token prevToken(void)
{
  return prev_token;
}

/* 第一引数のトークンが, 第二引数の種類に合致するか,
 * つまりtoken.kind == kindをチェックして
 * 次のトークンを返す.
 * 異なる場合はコンパイルエラー */
Token checkGetToken(Token token, Token_kind kind)
{
  if (token.kind == kind) {
    /* 一致していれば次のトークンを返す */
    return nextToken();
  } else {
    /* 一致していなければエラー */
    if (kind == OTHERS) {
      /* 無効なトークン */
      compile_error(INVAILD_CHAR_ERROR, line_number);
    }

    /* シンタックスエラー */
    compile_error(SYNTAX_ERROR, line_number);
  }

  /* unreachable here */
  exit(1);
}

/* checkGetTokenの, 2種類対応版.
 * kind1, またはkind2との一致を確認して, 次のトークンを返す */
Token checkGetToken2(Token token, Token_kind kind1, Token_kind kind2)
{
  if (token.kind == kind1 || token.kind == kind2) {
    /* 一致していれば次のトークンを返す */
    return nextToken();
  } else {
    /* 一致していなければエラー */
    if (token.kind == OTHERS) {
      /* 無効なトークン */
      compile_error(INVAILD_CHAR_ERROR, line_number);
    }

    /* シンタックスエラー */
    compile_error(SYNTAX_ERROR, line_number);
  }

  /* unreachable here */
  exit(1);
}

    

/* 読み込みの準備. 初期設定 */
void initSource(void) 
{
  /* 初期の文字や, 文字/予約シンボル対応テーブルを設定 */
  line_index = -1;
  line_number = 0;
  nextchar = '\n';
  init_char_kind();
}


/* デバッグ用トークンの印字 */
void printToken(Token token)
{
  if (token.kind < END_OF_KEYSYMBOL) {
    printf("<key %s>", keyword_table[token.kind].keyword_string);
  } else {
    switch (token.kind) {
      case IDENTIFIER:
        printf("<ident %s>", token.u.identifier);
        break;
      case INT_LITERAL:
        printf("<int %d>", token.u.int_value);
        break;
      case DOUBLE_LITERAL:
        printf("<double %f>", token.u.double_value);
        break;
      case STRING_LITERAL:
        printf("<string %s>", token.u.string_value);
        break;
      default:
        printf("<other id %d>", token.kind);
        break;
    }
  }
}

