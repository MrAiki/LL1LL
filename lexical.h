#ifndef LEXICAL_H_INCLUDED
#define LEXICAL_H_INCLUDED

#include <stdio.h>
#include <ctype.h>  /* isdigitやら, 文字種類判定に使う */
#include <string.h>
#include <stdlib.h>

#include "share.h"
#include "error.h"

/* トークン（若しくは, 文字）の種類 */
typedef enum token_kind_tag {
  VAR = 0, CONST, FUNC,                     /* 変数/定数/関数定義 var, const, func */
  INT_TYPE, DOUBLE_TYPE,                    /* 整数型, 倍精度実数型 */
  STRING_TYPE, BOOLEAN_TYPE,                /* 文字列型, 論理型 */
  FOR, WHILE, DO,                           /* 制御構文 for, while, do */
  CONTINUE, BREAK, RETURN,                  /* 制御構文 break, return  */
  IF, ELSIF, ELSE,                          /* 制御構文 if, elsif, else */
  SWITCH, CASE, DEFAULT,                    /* 制御構文 switch, case, default */
  NULL_LITERAL,                             /* NULLリテラル "null" */
  TRUE_LITERAL, FALSE_LITERAL,              /* 論理値リテラル "true", "false" */
  MOD_STRING,                               /* "mod" '%'と等価 */
  LEFT_BRACE_STRING, RIGHT_BRACE_STRING,    /* "begin", "end" それぞれ '{', '}'と等価 */
  LOGICAL_OR_STRING, LOGICAL_AND_STRING,    /* "or", "and" それぞれ "||", "&&"と等価*/
  LOGICAL_NOT_STRING,                       /* "not" '!'と等価 */
  STDIN, STDOUT, STDERR,                    /* "stdin", "stdout", "stderr" */
  END_OF_KEYWORD,                           /* ここまで予約語. 番兵に使う */
  ASSIGN, ADD_ASSIGN, SUB_ASSIGN,           /* 代入/代入演算子 =, +=, -=, */
  MUL_ASSIGN, DIV_ASSIGN, MOD_ASSIGN,       /* *=, /=, %= */
  COMMA, SEMICOLON, COLON, PERIOD,          /* ',', ';', ':', '.' */
  LEFT_PARLEN, RIGHT_PARLEN,                /* '(', ')' */
  LEFT_BRACE, RIGHT_BRACE,                  /* '{', '}' */
  LEFT_BRAKET, RIGHT_BRAKET,                /* '[', ']' */
  PIPE, AMP,                                /* '|', '&' */
  LOGICAL_OR, LOGICAL_AND,                  /* "||", "&&" */
  EQUAL, NOT_EQUAL,                         /* "==", "!=" */
  GREATER, GREATER_EQUAL,                   /* '>', ">=" */
  LESSTHAN, LESSTHAN_EQUAL,                 /* '<', "<=" */
  PLUS, MINUS, MUL, DIV, MOD, LOGICAL_NOT,  /* '+', '-', '*', '/', '%', '!' */
  POWER,                                    /* "**" */
  INCREMENT, DECREMENT,                     /* "++", "--" */
  PUT_TO_STREAM, GET_FROM_STREAM,           /* "<<", ">>" */
  DOUBLE_QUOTE,                             /* 文字列境界 '"' */
  END_OF_KEYSYMBOL,                         /* ここまで予約シンボル. 番兵に使う */
  IDENTIFIER,                               /* 識別子文字列 */
  INT_LITERAL, DOUBLE_LITERAL,              /* 整数/倍精度実数リテラル */
  STRING_LITERAL,                           /* 文字列リテラル */
  END_OF_TOKEN,                             /* ここまでトークンの種類. 番兵に使う */
  LETTER,                                   /* [a-Z] 英文字 */
  DIGIT,                                    /* [0-9] 数字 */
  OTHERS,                                   /* それ以外の文字 */
  END_OF_CHAR_KIND,                         /* ここまで文字種類 */
  END_OF_FILE,                              /* ファイル終わり */
} Token_kind;

/* トークン */
typedef struct token_tag {
  Token_kind kind;  /* 種類 */
  /* 内容 : 整数,倍精度,文字列,論理値,識別子文字列 */
  union {
    int           int_value;
    double        double_value;
    char          string_value[MAX_LEN_STRING_LITERAL];
    LL1LL_Boolean boolean_value;
    char          identifier[MAX_LEN_IDENTIFIER];
  } u;
} Token;

int line_number;                /* 現在コンパイル中の行数 */

int openSource(char *filename); /* ソースファイルのオープン */
void closeSource(void);         /* ソースファイルのクローズ */
void initSource(void);          /* 読み込み準備 */
Token nextToken(void);          /* 次のトークンを読む */
Token prevToken(void);          /* 1つ前のトークンを返す */
Token checkGetToken(Token token, Token_kind kind);  /* 引数のトークンが第二引数のトークンの種類と一致するか確認し, 次のトークンを返す */
Token checkGetToken2(Token token, Token_kind kind1, Token_kind kind2);  /* 引数のトークンが第二引数, または第三引数のトークンの種類と一致するか確認し, 次のトークンを返す */
void printToken(Token token);   /* トークンの印字 */

#endif /* LEXICAL_H_INCLUDED */
