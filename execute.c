#include "execute.h"

static LL1LL_Value stack[MAX_EXE_STACK_SIZE]; /* 実行時スタック */
static int top;                               /* スタックトップ(プッシュ・ポップの対象となるスタックのインデックス). 次に更新されるスタックのアドレス. */

/* 単項演算命令のサブルーチン */
static LL1LL_Value do_single_calc(LL1LL_Value term,
                                  LVM_OpCode);
/* 演算命令のサブルーチン */
static LL1LL_Value do_calculate(LL1LL_Value left, 
                                LL1LL_Value right,
                                LVM_OpCode);
/* 比較命令のサブルーチン */
static LL1LL_Value do_compare(LL1LL_Value left, 
                              LL1LL_Value right,
                              LVM_OpCode);
/* ストリームプッシュのサブルーチン */
static LL1LL_Value do_put_stream(LL1LL_Value left,
                                 LL1LL_Value right);
/* スタックのstack_pの要素を印字 */
static void printStackElement(int stack_p);

/* スタックトップを得る */
int getStackTop(void)
{
  return top;
}

/* スタックを指すポインタを得る */
LL1LL_Value *getStackPointer(void)
{
  return &(stack[0]);
}

/* 命令列の実行 */
void execute(void)
{
  int pc;                                     /* プログラムカウンタ */
  int temp_level;                             /* ブロックレベルと */
  LL1LL_Value temp_value;                     /* 値のテンポラリ */
  LVM_Instruction *code = getInstruction();   /* 命令列 */
  int code_size         = getCodeSize();      /* 命令列のサイズ */
  LVM_Instruction inst;                       /* 現在の命令 */
  /* int display[MAX_BLOCK_LEVEL] = {0};      ディスプレイの配列:各ブロックの先頭アドレス => tableに移動 */

  /* ---実行開始--- */
  top = 0; pc = 0;               /* スタックトップ, pcの初期化 */
  /* レベル0(トップレベル)の... */
  stack[top].u.int_value   = 0;  /* ディスプレイの退避場所 */
  stack[top+1].u.int_value = 0;  /* 戻り番地(終了番地) */
  /* display[0]               = 0;   トップレベルの先頭番地:0 */
  setDisplayAt(0,0);             /* トップレベルの先頭番地:0 */
  
  /* ---命令実行--- */
  do {
    inst = code[pc++];  /* これから実行する命令語を取得 */
    /* 命令分岐 */
    switch (inst.opcode) {
      case LVM_NOP:
        /* 何もしない */
        break;
      case LVM_MOVE_STACK_P:
        /* スタックポインタの移動 */
        top += inst.u.move_top;
        /* TODO:スタックオーバーフローのチェック */
        break;
      case LVM_PUSH_IMMEDIATE:
        /* 即値のプッシュ */
        stack[top++] = inst.u.value;
        break;
      case LVM_PUSH_VALUE:
        /* 変数のプッシュ : スタック記憶域からアドレスを取得してプッシュ */
        stack[top++] 
          = stack[getDisplayAt(inst.u.address.block_level)
                    + inst.u.address.address];
        break;
      case LVM_POP_VARIABLE:
        /* 変数のポップ : アドレスを指定してポップ */
        stack[getDisplayAt(inst.u.address.block_level)
                    + inst.u.address.address] = stack[--top];
        break;
      case LVM_POP:
        /* 単純にトップを一つずらすだけ */
        top--;
        break;
      case LVM_DUPLICATE:
        /* トップの値を複製してプッシュ */
        stack[top] = stack[top-1];
        top++;
        break;
      case LVM_JUMP:
        /* 無条件ジャンプ */
        pc = inst.u.jump_pc;  /* pcを書き換える */
        break;
      case LVM_JUMP_IF_TRUE:
        /* トップの値がTRUEならばジャンプ. トップは捨てる */
        if (stack[--top].u.boolean_value == LL1LL_TRUE) {
          pc = inst.u.jump_pc;
        }
        break;
      case LVM_JUMP_IF_FALSE:
        /* トップの値がFALSEならばジャンプ. トップは捨てる */
        if (stack[--top].u.boolean_value == LL1LL_FALSE) {
          pc = inst.u.jump_pc;
        }
        break;
      case LVM_INVOKE:
        /* 関数呼び出し */
        temp_level = inst.u.address.block_level + 1;    /* 関数ブロック内のレベルは呼び出したブロック+1 */
        /* stack[top].u.int_value   = display[temp_level];  ディスプレイの退避 */
        stack[top].type          = LL1LL_INT_TYPE;
        stack[top].u.int_value   = getDisplayAt(temp_level);
        stack[top+1].type        = LL1LL_INT_TYPE;
        stack[top+1].u.int_value = pc;                  /* 戻り先のpc(現在のpc) */
        /* display[temp_level]      = top;                 関数内のディスプレイは現在のtopを指させる */
        setDisplayAt(temp_level, top);
        pc = inst.u.address.address;                    /* 関数内部へジャンプ */
        break;
      case LVM_RETURN:
        /* 関数からのリターン */
        temp_value   = stack[--top];                        /* 戻り値の確保 */
        /* top          = display[inst.u.address.block_level];  スタックトップを呼び出し側の値に戻す */
        top          = getDisplayAt(inst.u.address.block_level);
        /* display[inst.u.address.block_level] = stack[top].u.int_value;   ディスプレイ情報の復帰 */
        setDisplayAt(inst.u.address.block_level, stack[top].u.int_value);
        pc           = stack[top+1].u.int_value;                        /* 戻り先のpcにセット */
        top         -= inst.u.address.address;              /* 実引数の数だけトップを移動 */
        stack[top++] = temp_value;                          /* 戻り値をトップにセット */
        break;
        /* 単項演算命令 -> サブルーチンに投げる */
      case LVM_MINUS:       /* FALLTHRU */
      case LVM_LOGICAL_NOT: /* FALLTHRU */
      case LVM_INCREMENT:   /* FALLTHRU */
      case LVM_DECREMENT:
        stack[top-1]
          = do_single_calc(stack[top-1], inst.opcode);
          break;
        /* 演算命令 -> サブルーチンに投げる */
      case LVM_ADD:         /* FALLTHRU */
      case LVM_SUB:         /* FALLTHRU */
      case LVM_MUL:         /* FALLTHRU */
      case LVM_DIV:         /* FALLTHRU */
      case LVM_MOD:         /* FALLTHRU */
      case LVM_POW:         /* FALLTHRU */
      case LVM_LOGICAL_AND: /* FALLTHRU */
      case LVM_LOGICAL_OR:
        top--;
        stack[top-1] 
          = do_calculate(stack[top-1], stack[top], inst.opcode);
        break;
        /* 比較命令 -> サブルーチンに投げる */
      case LVM_EQUAL:         /* FALLTHRU */
      case LVM_NOT_EQUAL:     /* FALLTHRU */ 
      case LVM_GREATER:       /* FALLTHRU */
      case LVM_GREATER_EQUAL: /* FALLTHRU */
      case LVM_LESSTHAN:      /* FALLTHRU */
      case LVM_LESSTHAN_EQUAL:
        top--;
        stack[top-1] 
          = do_compare(stack[top-1], stack[top], inst.opcode);
        break;
        /* ストリームにポップ */
      case LVM_POP_TO_STREAM:
        top--;
        stack[top-1]
          = do_put_stream(stack[top-1], stack[top]);
        break;
        /* ストリームからプッシュ */
      case LVM_PUSH_FROM_STREAM:
        /* TODO:これは考察すべき. どの関数を使うか. */
        break;
      default:
        fprintf(stderr, "Error! Invailed opecode \n");
        exit(EXIT_FAILURE);
    }

    /* デバッグ用, スタックの状態を表示 */
       printStack();
       printf("pc : %4d ", pc);
       printCode(pc);
  } while (pc <= code_size);
  /* } while (pc != 0); */

  /* TODO:*code の解放 */
}

/* 単項演算の実行 */
static LL1LL_Value 
do_single_calc(LL1LL_Value term, LVM_OpCode code)
{
  LL1LL_Value ret_value;  /* 結果のテンポラリ */

  /* またしても演算の種類で場合分け */
  switch (code) {
    case LVM_MINUS:
      /* 単項マイナス */
      switch (term.type) {
        case LL1LL_INT_TYPE:
          ret_value.type        = LL1LL_INT_TYPE;
          ret_value.u.int_value = -term.u.int_value;
          break;
        case LL1LL_DOUBLE_TYPE:
          ret_value.type           = LL1LL_DOUBLE_TYPE;
          ret_value.u.double_value = -term.u.double_value;
          break;
        default:
          fprintf(stderr, "Invailed type for single term minus \n");
          exit(EXIT_FAILURE);
      }
      break;
    case LVM_LOGICAL_NOT:
      /* 単項論理否定 */
      if (term.type == LL1LL_BOOLEAN_TYPE) {
        ret_value.type = LL1LL_BOOLEAN_TYPE;
        if (term.u.boolean_value == LL1LL_TRUE) {
          ret_value.u.boolean_value = LL1LL_FALSE;
        } else {
          ret_value.u.boolean_value = LL1LL_TRUE;
        }
      } else {
        fprintf(stderr, "Invailed type for single term logical not \n");
        exit(EXIT_FAILURE);
      }
      break;
    case LVM_INCREMENT:
      /* 単項インクリメント */
      if (term.type == LL1LL_INT_TYPE) {
        ret_value.type = LL1LL_INT_TYPE;
        ret_value.u.int_value = term.u.int_value + 1;
      } else {
        fprintf(stderr, "Invailed type for single term increment \n");
        exit(EXIT_FAILURE);
      }
      break;
    case LVM_DECREMENT:
      /* 単項デクリメント */
      if (term.type == LL1LL_INT_TYPE) {
        ret_value.type = LL1LL_INT_TYPE;
        ret_value.u.int_value = term.u.int_value - 1;
      } else {
        fprintf(stderr, "Invailed type for single term decrement \n");
        exit(EXIT_FAILURE);
      }
      break;
    default:
      fprintf(stderr, "Error! Invailed opecode in single calc \n");
      exit(EXIT_FAILURE);
  }
  return ret_value;

}

/* 二項演算の実行 */
static LL1LL_Value 
do_calculate(LL1LL_Value left, LL1LL_Value right, LVM_OpCode code)
{

  char str_buf[RUNTIME_STR_BUF_SIZE]; /* 文字列バッファ */
  LL1LL_Value ret_value;              /* 結果のテンポラリ */
  ret_value.type = left.type;         /* 返す型はとりあえず左辺に合わせる */

  switch (code) {
    case LVM_ADD:
      /* 加算 */
      if (left.type == LL1LL_INT_TYPE 
          && right.type == LL1LL_INT_TYPE) {
        /* 左辺がint, 右辺がint */
        ret_value.u.int_value = left.u.int_value + right.u.int_value;
      } else if (left.type == LL1LL_DOUBLE_TYPE
                 && right.type == LL1LL_INT_TYPE) {
        /* 左辺がdouble, 右辺がint */
        ret_value.u.double_value = left.u.double_value + (double)right.u.int_value;
      } else if (left.type == LL1LL_INT_TYPE
                 && right.type == LL1LL_DOUBLE_TYPE) {
        /* 左辺がint, 右辺がdouble */
        ret_value.type = LL1LL_DOUBLE_TYPE;
        ret_value.u.double_value = (double)left.u.int_value + right.u.int_value;
      } else if (left.type == LL1LL_DOUBLE_TYPE
                 && right.type == LL1LL_DOUBLE_TYPE) {
        /* 左辺がdouble, 右辺がdouble */
        ret_value.u.double_value = left.u.double_value + right.u.double_value;
      } else if (left.type == LL1LL_BOOLEAN_TYPE
                 && right.type == LL1LL_BOOLEAN_TYPE) {
        /* 左辺がboolean, 右辺がboolean */
        if (left.u.boolean_value == LL1LL_TRUE) {
          ret_value.u.boolean_value = LL1LL_TRUE;
        } else {
          ret_value.u.boolean_value = right.u.boolean_value;
        }
      } else if (is_string(left)
                 && right.type == LL1LL_INT_TYPE) {
        /* 左辺がstring, 右辺がint */
        /* 数字を文字列に変換 */
        sprintf(str_buf, "%d", right.u.int_value);
        /* 文字列を連結し,結果のオブジェクト参照を取得 */
        ret_value.u.object
          = cat_string(get_string_value(left), str_buf);
      } else if (is_string(left)
                 && right.type == LL1LL_DOUBLE_TYPE) {
        /* 左辺がstring, 右辺がdouble */
        /* 数字を文字列に変換 */
        sprintf(str_buf, "%f", right.u.double_value);
        /* 文字列を連結 */
        left.u.object
          = cat_string(get_string_value(left), str_buf);
        ret_value = left;
      } else if (is_string(left)
                 && right.type == LL1LL_BOOLEAN_TYPE) {
        /* 左辺がstring, 右辺がboolean */
        if (right.u.boolean_value == LL1LL_TRUE) {
          strcat(str_buf, "true");
        } else {
          strcpy(str_buf, "false");
        }
        /* 文字列を連結 */
        left.u.object
          = cat_string(get_string_value(left), str_buf);
        ret_value = left;
      } else if (is_string(left) && is_string(right)) {
        /* 左辺がstring, 右辺がstring */
        /* 文字列を連結 */
        left.u.object
          = cat_string(get_string_value(left), get_string_value(right));
        /* 参照も同時に渡すので, 多分大丈夫 */
        ret_value = left;
      } else {
        /* 加算の型エラー */
        fprintf(stderr, "Error! Invailed type in add \n");
        exit(EXIT_FAILURE);
      }
      break;
    case LVM_SUB:
      /* 減算 */
      if (left.type == LL1LL_INT_TYPE 
          && right.type == LL1LL_INT_TYPE) {
        /* 左辺がint, 右辺がint */
        ret_value.u.int_value = left.u.int_value - right.u.int_value;
      } else if (left.type == LL1LL_DOUBLE_TYPE
                 && right.type == LL1LL_INT_TYPE) {
        /* 左辺がdouble, 右辺がint */
        ret_value.u.double_value = left.u.double_value - (double)right.u.int_value;
      } else if (left.type == LL1LL_INT_TYPE
                 && right.type == LL1LL_DOUBLE_TYPE) {
        /* 左辺がint, 右辺がdouble */
        ret_value.type = LL1LL_DOUBLE_TYPE;
        ret_value.u.double_value = (double)left.u.int_value - right.u.int_value;
      } else if (left.type == LL1LL_DOUBLE_TYPE
                 && right.type == LL1LL_DOUBLE_TYPE) {
        /* 左辺がdouble, 右辺がdouble */
        ret_value.u.double_value = left.u.double_value - right.u.double_value;
      } else if (left.type == LL1LL_BOOLEAN_TYPE
                 && right.type == LL1LL_BOOLEAN_TYPE) {
        /* 左辺がboolean, 右辺がboolean */
        if (right.u.boolean_value == LL1LL_TRUE) {
          ret_value.u.boolean_value = LL1LL_FALSE;
        } else {
          ret_value.u.boolean_value = left.u.boolean_value;
        }
      } else {
        /* 減算の型エラー */
        fprintf(stderr, "Error! Invailed type in add \n");
        exit(EXIT_FAILURE);
      }
      break;
    case LVM_MUL:
      /* 積算 */
      if (left.type == LL1LL_INT_TYPE 
          && right.type == LL1LL_INT_TYPE) {
        /* 左辺がint, 右辺がint */
        ret_value.u.int_value = left.u.int_value * right.u.int_value;
      } else if (left.type == LL1LL_DOUBLE_TYPE
                 && right.type == LL1LL_INT_TYPE) {
        /* 左辺がdouble, 右辺がint */
        ret_value.u.double_value = left.u.double_value * (double)right.u.int_value;
      } else if (left.type == LL1LL_INT_TYPE
                 && right.type == LL1LL_DOUBLE_TYPE) {
        /* 左辺がint, 右辺がdouble */
        ret_value.type = LL1LL_DOUBLE_TYPE;
        ret_value.u.double_value = (double)left.u.int_value * right.u.int_value;
      } else if (left.type == LL1LL_DOUBLE_TYPE
                 && right.type == LL1LL_DOUBLE_TYPE) {
        /* 左辺がdouble, 右辺がdouble */
        ret_value.u.double_value = left.u.double_value * right.u.double_value;
      } else if (left.type == LL1LL_BOOLEAN_TYPE
                 && right.type == LL1LL_BOOLEAN_TYPE) {
        /* 左辺がboolean, 右辺がboolean */
        if (left.u.boolean_value == LL1LL_TRUE) {
          ret_value.u.boolean_value = right.u.boolean_value;
        } else {
          ret_value.u.boolean_value = LL1LL_FALSE;
        }
      } else if (is_string(left)
                 && right.type == LL1LL_INT_TYPE) {
        /* 左辺がstring, 右辺がint */
        /* -> 文字列を整数回繰り返す */
      } else {
        /* 積算の型エラー */
        fprintf(stderr, "Error! Invailed type in mul \n");
        exit(EXIT_FAILURE);
      }
      break;
    case LVM_DIV:
      /* 除算 */
      if (left.type == LL1LL_INT_TYPE 
          && right.type == LL1LL_INT_TYPE) {
        /* 左辺がint, 右辺がint */
        if (right.u.int_value == 0) {
          fprintf(stderr, "Zero division detected! \n");
          exit(EXIT_FAILURE);
        }
        ret_value.u.int_value = left.u.int_value / right.u.int_value;
      } else if (left.type == LL1LL_DOUBLE_TYPE
                 && right.type == LL1LL_INT_TYPE) {
        /* 左辺がdouble, 右辺がint */
        ret_value.u.double_value = left.u.double_value / (double)right.u.int_value;
      } else if (left.type == LL1LL_INT_TYPE
                 && right.type == LL1LL_DOUBLE_TYPE) {
        /* 左辺がint, 右辺がdouble */
        ret_value.type = LL1LL_DOUBLE_TYPE;
        ret_value.u.double_value = (double)left.u.int_value / right.u.int_value;
      } else if (left.type == LL1LL_DOUBLE_TYPE
                 && right.type == LL1LL_DOUBLE_TYPE) {
        /* 左辺がdouble, 右辺がdouble */
        ret_value.u.double_value = left.u.double_value / right.u.double_value;
      } else {
        /* 除算の型エラー */
        fprintf(stderr, "Error! Invailed type in div \n");
        exit(EXIT_FAILURE);
      }
      break;
    case LVM_MOD:
      /* 余算 */
      if (left.type == LL1LL_INT_TYPE 
          && right.type == LL1LL_INT_TYPE) {
        /* 左辺がint, 右辺がint */
        if (right.u.int_value == 0) {
          fprintf(stderr, "Zero modulo detected! \n");
          exit(EXIT_FAILURE);
        }
        ret_value.u.int_value = left.u.int_value % right.u.int_value;
      } else if (left.type == LL1LL_DOUBLE_TYPE
                 && right.type == LL1LL_INT_TYPE) {
        /* 左辺がdouble, 右辺がint */
        ret_value.u.double_value = fmod(left.u.double_value, (double)right.u.int_value);
      } else if (left.type == LL1LL_INT_TYPE
                 && right.type == LL1LL_DOUBLE_TYPE) {
        /* 左辺がint, 右辺がdouble */
        ret_value.type = LL1LL_DOUBLE_TYPE;
        ret_value.u.double_value = fmod((double)left.u.int_value, right.u.double_value);
      } else if (left.type == LL1LL_DOUBLE_TYPE
                 && right.type == LL1LL_DOUBLE_TYPE) {
        /* 左辺がdouble, 右辺がdouble */
        ret_value.u.double_value = fmod(left.u.double_value, right.u.double_value);
      } else {
        /* 除算の型エラー */
        fprintf(stderr, "Error! Invailed type in mod \n");
        exit(EXIT_FAILURE);
      }
      break;
    case LVM_POW:
      /* 累乗 */
      if (left.type == LL1LL_INT_TYPE 
          && right.type == LL1LL_INT_TYPE) {
        /* 左辺がint, 右辺がint */
        if (left.u.int_value == 0) {
          fprintf(stderr, "Zero power detected! \n");
          exit(EXIT_FAILURE);
        }
        ret_value.u.int_value = (int)pow((double)left.u.int_value,
                                         (double)right.u.int_value);
      } else if (left.type == LL1LL_DOUBLE_TYPE
                 && right.type == LL1LL_INT_TYPE) {
        /* 左辺がdouble, 右辺がint */
        ret_value.u.double_value = pow(left.u.double_value,
                                       (double)right.u.int_value);
      } else if (left.type == LL1LL_INT_TYPE
                 && right.type == LL1LL_DOUBLE_TYPE) {
        /* 左辺がint, 右辺がdouble */
        ret_value.type = LL1LL_DOUBLE_TYPE;
        ret_value.u.double_value = pow((double)left.u.int_value,
                                       right.u.double_value);
      } else if (left.type == LL1LL_DOUBLE_TYPE
                 && right.type == LL1LL_DOUBLE_TYPE) {
        /* 左辺がdouble, 右辺がdouble */
        ret_value.u.double_value = pow(left.u.double_value,
                                       right.u.double_value);
      } else {
        /* 累乗の型エラー */
        fprintf(stderr, "Error! Invailed type in pow \n");
        exit(EXIT_FAILURE);
      }
      break;
    case LVM_LOGICAL_AND:
      /* 論理積 */
      if (left.type == LL1LL_BOOLEAN_TYPE 
          && right.type == LL1LL_BOOLEAN_TYPE) {
        /* 左辺がboolean, 右辺がboolean */
        if (left.u.boolean_value == LL1LL_FALSE) {
          ret_value.u.boolean_value = LL1LL_FALSE;
        } else {
          ret_value.u.boolean_value = right.u.boolean_value;
        }
      } else {
        /* 論理積の型エラー */
        fprintf(stderr, "Error! Invailed type in logical and \n");
        exit(EXIT_FAILURE);
      }
      break;
    case LVM_LOGICAL_OR:
      /* 論理和 */
      if (left.type == LL1LL_BOOLEAN_TYPE 
          && right.type == LL1LL_BOOLEAN_TYPE) {
        /* 左辺がboolean, 右辺がboolean */
        if (left.u.boolean_value == LL1LL_TRUE) {
          ret_value.u.boolean_value = LL1LL_TRUE;
        } else {
          ret_value.u.boolean_value = right.u.boolean_value;
        }
      } else {
        /* 論理積の型エラー */
        fprintf(stderr, "Error! Invailed type in logical and \n");
        exit(EXIT_FAILURE);
      }
      break;
    default:
      fprintf(stderr, "Error! Invailed opecode in binary calc \n");
      exit(EXIT_FAILURE);
  }
  return ret_value;

}

/* 二項比較演算の実行 */
static LL1LL_Value 
do_compare(LL1LL_Value left, LL1LL_Value right, LVM_OpCode code)
{

  LL1LL_Value ret_value;  /* 結果のテンポラリ */
  ret_value.type = LL1LL_BOOLEAN_TYPE;  /* 返す型は論理型で確定 */
  switch (code) {
    case LVM_EQUAL:
      /* 等号 */
      if (left.type == LL1LL_BOOLEAN_TYPE
          && right.type == LL1LL_BOOLEAN_TYPE) {
        /* 左辺がboolean, 右辺がboolean */
        if (left.u.boolean_value == right.u.boolean_value) {
          ret_value.u.boolean_value = LL1LL_TRUE;
        } else {
          ret_value.u.boolean_value = LL1LL_FALSE;
        }
      } else if (left.type == LL1LL_INT_TYPE
                 && right.type == LL1LL_INT_TYPE) {
        /* 左辺がint, 右辺がint */
        if (left.u.int_value == right.u.int_value) {
          ret_value.u.boolean_value = LL1LL_TRUE;
        } else {
          ret_value.u.boolean_value = LL1LL_FALSE;
        }
      } else if (left.type == LL1LL_DOUBLE_TYPE
                 && right.type == LL1LL_DOUBLE_TYPE) {
        /* 左辺がdouble, 右辺がdouble */
        if (fabs(left.u.double_value - right.u.double_value)
            < DBL_EPSILON) {
          ret_value.u.boolean_value = LL1LL_TRUE;
        } else {
          ret_value.u.boolean_value = LL1LL_FALSE;
        }
      } else if (is_string(left) && is_string(right)) {
        /* 左辺がstring, 右辺がstring */
        if (strcmp(get_string_value(left), 
                   get_string_value(right)) == 0) {
          ret_value.u.boolean_value = LL1LL_TRUE;
        } else {
          ret_value.u.boolean_value = LL1LL_FALSE;
        }
      } else {
        /* 比較の型エラー */
        fprintf(stderr, "Error! Invailed type in equal\n");
        exit(EXIT_FAILURE);
      }
      break;
    case LVM_NOT_EQUAL:
      /* 不等号 */
      if (left.type == LL1LL_BOOLEAN_TYPE
          && right.type == LL1LL_BOOLEAN_TYPE) {
        /* 左辺がboolean, 右辺がboolean */
        if (left.u.boolean_value != right.u.boolean_value) {
          ret_value.u.boolean_value = LL1LL_TRUE;
        } else {
          ret_value.u.boolean_value = LL1LL_FALSE;
        }
      } else if (left.type == LL1LL_INT_TYPE
                 && right.type == LL1LL_INT_TYPE) {
        /* 左辺がint, 右辺がint */
        if (left.u.int_value != right.u.int_value) {
          ret_value.u.boolean_value = LL1LL_TRUE;
        } else {
          ret_value.u.boolean_value = LL1LL_FALSE;
        }
      } else if (left.type == LL1LL_DOUBLE_TYPE
                 && right.type == LL1LL_DOUBLE_TYPE) {
        /* 左辺がdouble, 右辺がdouble */
        if (fabs(left.u.double_value - right.u.double_value)
            >= DBL_EPSILON) {
          ret_value.u.boolean_value = LL1LL_TRUE;
        } else {
          ret_value.u.boolean_value = LL1LL_FALSE;
        }
      } else if (is_string(left) && is_string(right)) {
        /* 左辺がstring, 右辺がstring */
        if (strcmp(get_string_value(left), 
                   get_string_value(right)) != 0) {
          ret_value.u.boolean_value = LL1LL_TRUE;
        } else {
          ret_value.u.boolean_value = LL1LL_FALSE;
        }
      } else {
        /* 比較の型エラー */
        fprintf(stderr, "Error! Invailed type in not equal\n");
        exit(EXIT_FAILURE);
      }
      break;
    case LVM_GREATER:
      /* 大なり */
      if (left.type == LL1LL_BOOLEAN_TYPE
          && right.type == LL1LL_BOOLEAN_TYPE) {
        /* 左辺がboolean, 右辺がboolean */
        if (left.u.boolean_value == LL1LL_TRUE
            && right.u.boolean_value == LL1LL_FALSE) {
          ret_value.u.boolean_value = LL1LL_TRUE;
        } else {
          ret_value.u.boolean_value = LL1LL_FALSE;
        }
      } else if (left.type == LL1LL_INT_TYPE
                 && right.type == LL1LL_DOUBLE_TYPE) {
        /* 左辺がint, 右辺がdouble */
        if ((double)left.u.int_value > right.u.double_value) {
          ret_value.u.boolean_value = LL1LL_TRUE;
        } else {
          ret_value.u.boolean_value = LL1LL_FALSE;
        }
      } else if (left.type == LL1LL_DOUBLE_TYPE
                 && right.type == LL1LL_INT_TYPE) {
        /* 左辺がdouble, 右辺がint */
        if (left.u.double_value > (double)right.u.int_value) {
          ret_value.u.boolean_value = LL1LL_TRUE;
        } else {
          ret_value.u.boolean_value = LL1LL_FALSE;
        }
      } else if (left.type == LL1LL_INT_TYPE
                 && right.type == LL1LL_INT_TYPE) {
        /* 左辺がint, 右辺がint */
        if (left.u.int_value > right.u.int_value) {
          ret_value.u.boolean_value = LL1LL_TRUE;
        } else {
          ret_value.u.boolean_value = LL1LL_FALSE;
        }
      } else if (left.type == LL1LL_DOUBLE_TYPE
                 && right.type == LL1LL_DOUBLE_TYPE) {
        /* 左辺がdouble, 右辺がdouble */
        if (left.u.double_value > right.u.double_value) {
          ret_value.u.boolean_value = LL1LL_TRUE;
        } else {
          ret_value.u.boolean_value = LL1LL_FALSE;
        }
      } else if (is_string(left) && is_string(right)) {
        /* 左辺がstring, 右辺がstring */
        if (strcmp(get_string_value(left), 
                   get_string_value(right)) > 0) {
          ret_value.u.boolean_value = LL1LL_TRUE;
        } else {
          ret_value.u.boolean_value = LL1LL_FALSE;
        }
      } else {
        /* 比較の型エラー */
        fprintf(stderr, "Error! Invailed type in greater\n");
        exit(EXIT_FAILURE);
      }
      break;
    case LVM_GREATER_EQUAL:
      /* 以上 */
      if (left.type == LL1LL_INT_TYPE
                 && right.type == LL1LL_DOUBLE_TYPE) {
        /* 左辺がint, 右辺がdouble */
        if ((double)left.u.int_value >= right.u.double_value) {
          ret_value.u.boolean_value = LL1LL_TRUE;
        } else {
          ret_value.u.boolean_value = LL1LL_FALSE;
        }
      } else if (left.type == LL1LL_DOUBLE_TYPE
                 && right.type == LL1LL_INT_TYPE) {
        /* 左辺がdouble, 右辺がint */
        if (left.u.double_value >= (double)right.u.int_value) {
          ret_value.u.boolean_value = LL1LL_TRUE;
        } else {
          ret_value.u.boolean_value = LL1LL_FALSE;
        }
      } else if (left.type == LL1LL_INT_TYPE
                 && right.type == LL1LL_INT_TYPE) {
        /* 左辺がint, 右辺がint */
        if (left.u.int_value >= right.u.int_value) {
          ret_value.u.boolean_value = LL1LL_TRUE;
        } else {
          ret_value.u.boolean_value = LL1LL_FALSE;
        }
      } else if (left.type == LL1LL_DOUBLE_TYPE
                 && right.type == LL1LL_DOUBLE_TYPE) {
        /* 左辺がdouble, 右辺がdouble */
        if (left.u.double_value >= right.u.double_value) {
          ret_value.u.boolean_value = LL1LL_TRUE;
        } else {
          ret_value.u.boolean_value = LL1LL_FALSE;
        }
      } else if (is_string(left) && is_string(right)) {
        /* 左辺がstring, 右辺がstring */
        if (strcmp(get_string_value(left), 
                   get_string_value(right)) >= 0) {
          ret_value.u.boolean_value = LL1LL_TRUE;
        } else {
          ret_value.u.boolean_value = LL1LL_FALSE;
        }
      } else {
        /* 比較の型エラー */
        fprintf(stderr, "Error! Invailed type in greater equal\n");
        exit(EXIT_FAILURE);
      }
      break;
    case LVM_LESSTHAN:
      /* 小なり */
      if (left.type == LL1LL_BOOLEAN_TYPE
          && right.type == LL1LL_BOOLEAN_TYPE) {
        /* 左辺がboolean, 右辺がboolean */
        if (left.u.boolean_value == LL1LL_FALSE
            && right.u.boolean_value == LL1LL_TRUE) {
          ret_value.u.boolean_value = LL1LL_TRUE;
        } else {
          ret_value.u.boolean_value = LL1LL_FALSE;
        }
      } else if (left.type == LL1LL_INT_TYPE
                 && right.type == LL1LL_DOUBLE_TYPE) {
        /* 左辺がint, 右辺がdouble */
        if ((double)left.u.int_value < right.u.double_value) {
          ret_value.u.boolean_value = LL1LL_TRUE;
        } else {
          ret_value.u.boolean_value = LL1LL_FALSE;
        }
      } else if (left.type == LL1LL_DOUBLE_TYPE
                 && right.type == LL1LL_INT_TYPE) {
        /* 左辺がdouble, 右辺がint */
        if (left.u.double_value < (double)right.u.int_value) {
          ret_value.u.boolean_value = LL1LL_TRUE;
        } else {
          ret_value.u.boolean_value = LL1LL_FALSE;
        }
      } else if (left.type == LL1LL_INT_TYPE
                 && right.type == LL1LL_INT_TYPE) {
        /* 左辺がint, 右辺がint */
        if (left.u.int_value < right.u.int_value) {
          ret_value.u.boolean_value = LL1LL_TRUE;
        } else {
          ret_value.u.boolean_value = LL1LL_FALSE;
        }
      } else if (left.type == LL1LL_DOUBLE_TYPE
                 && right.type == LL1LL_DOUBLE_TYPE) {
        /* 左辺がdouble, 右辺がdouble */
        if (left.u.double_value < right.u.double_value) {
          ret_value.u.boolean_value = LL1LL_TRUE;
        } else {
          ret_value.u.boolean_value = LL1LL_FALSE;
        }
      } else if (is_string(left) && is_string(right)) {
        /* 左辺がstring, 右辺がstring */
        if (strcmp(get_string_value(left), 
                   get_string_value(right)) < 0) {
          ret_value.u.boolean_value = LL1LL_TRUE;
        } else {
          ret_value.u.boolean_value = LL1LL_FALSE;
        }
      } else {
        /* 比較の型エラー */
        fprintf(stderr, "Error! Invailed type in less than\n");
        exit(EXIT_FAILURE);
      }
      break;
    case LVM_LESSTHAN_EQUAL:
      /* 以下 */
      if (left.type == LL1LL_INT_TYPE
                 && right.type == LL1LL_DOUBLE_TYPE) {
        /* 左辺がint, 右辺がdouble */
        if ((double)left.u.int_value <= right.u.double_value) {
          ret_value.u.boolean_value = LL1LL_TRUE;
        } else {
          ret_value.u.boolean_value = LL1LL_FALSE;
        }
      } else if (left.type == LL1LL_DOUBLE_TYPE
                 && right.type == LL1LL_INT_TYPE) {
        /* 左辺がdouble, 右辺がint */
        if (left.u.double_value <= (double)right.u.int_value) {
          ret_value.u.boolean_value = LL1LL_TRUE;
        } else {
          ret_value.u.boolean_value = LL1LL_FALSE;
        }
      } else if (left.type == LL1LL_INT_TYPE
                 && right.type == LL1LL_INT_TYPE) {
        /* 左辺がint, 右辺がint */
        if (left.u.int_value <= right.u.int_value) {
          ret_value.u.boolean_value = LL1LL_TRUE;
        } else {
          ret_value.u.boolean_value = LL1LL_FALSE;
        }
      } else if (left.type == LL1LL_DOUBLE_TYPE
                 && right.type == LL1LL_DOUBLE_TYPE) {
        /* 左辺がdouble, 右辺がdouble */
        if (left.u.double_value <= right.u.double_value) {
          ret_value.u.boolean_value = LL1LL_TRUE;
        } else {
          ret_value.u.boolean_value = LL1LL_FALSE;
        }
      } else if (is_string(left) && is_string(right)) {
        /* 左辺がstring, 右辺がstring */
        if (strcmp(get_string_value(left), 
                   get_string_value(right)) <= 0) {
          ret_value.u.boolean_value = LL1LL_TRUE;
        } else {
          ret_value.u.boolean_value = LL1LL_FALSE;
        }
      } else {
        /* 比較の型エラー */
        fprintf(stderr, "Error! Invailed type in less than equal\n");
        exit(EXIT_FAILURE);
      }
      break;
    default:
      fprintf(stderr, "Error! Invailed opecode in binary compare \n");
      exit(EXIT_FAILURE);
  }
  return ret_value;

}

/* ストリームへの出力 */
static LL1LL_Value do_put_stream(LL1LL_Value left, LL1LL_Value right)
{
  /* 返り値は, 書き込みステータスを取る */
  LL1LL_Value ret_value;

  /* 左辺がストリーム型で無いならばエラー */
  if (left.type != LL1LL_STREAM_TYPE) {
    fprintf(stderr, "Error! left hand of << isn't streamor string type \n");
    exit(EXIT_FAILURE);
  }

  /* 右辺が文字列でないならばエラー */
  if (!is_string(right)) {
    fprintf(stderr, "Error! right hand of << isn't string type \n");
    exit(EXIT_FAILURE);
  }

  /* fputsを使って書き込む.
   * TODO:fputsでいいのか? -> 今のところは良さそう */
  ret_value.type = LL1LL_INT_TYPE;
  /* 結果はfputs(書き込み)の戻り値 */
  ret_value.u.int_value
    = fputs(get_string_value(right), left.u.stream_value);

  return ret_value;
}

/* スタックの要素の印字 */
static void printStackElement(int stack_p)
{
  /* スタックの値と, 型の取得 */
  LL1LL_Value val = stack[stack_p];
  LL1LL_TypeKind type = val.type;

  /* 型で場合分けして印字 */
  switch (type) {
    case LL1LL_INT_TYPE:
      printf("%4d : int_value : %d", stack_p, val.u.int_value);
      break;
    case LL1LL_DOUBLE_TYPE:
      printf("%4d : double_value : %f", stack_p, val.u.double_value);
      break;
    case LL1LL_BOOLEAN_TYPE:
      if (val.u.boolean_value == LL1LL_TRUE) {
        printf("%4d : boolean_value : true", stack_p);
      } else {
        printf("%4d : boolean_value : false", stack_p);
      }
      break;
    case LL1LL_STREAM_TYPE:
      if (val.u.stream_value == stdin) {
        printf("%4d : stream_value : stdin", stack_p);
      } else if (val.u.stream_value == stdout) {
        printf("%4d : stream_value : stdout", stack_p);
      } else if (val.u.stream_value == stderr) {
        printf("%4d : stream_value : stderr", stack_p);
      } else {
        printf("%4d : stream_value(file pointer) : %p", stack_p, val.u.stream_value);
      }
      break;
    case LL1LL_OBJECT_TYPE:
      switch (val.u.object->type) {
        case ARRAY_OBJECT:
          /* TODO */
          break;
        case STRING_OBJECT:
          printf("%4d : string_object : \"%s\"", stack_p, val.u.object->u.str.string_value);
          break;
        default:
          break;
      }
      break;
    default:
      fprintf(stderr, "Error invaild type for printStackElement \n");
      break;
  }

  /* printf("\n"); */

}

/* スタックの印字 */
void printStack(void)
{
  int i;
  /* トップから下まで順に印字 */
  puts("---------------------------");
  /* top-1 */
  for (i = top-1; i >= 0; i--) {
    printStackElement(i);
    printf("\n");
    puts("---------------------------");
  }

  puts("");
}

