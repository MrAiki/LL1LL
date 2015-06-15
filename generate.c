#include "generate.h"

static LVM_Instruction code[MAX_CODE_SIZE]; /* 命令コードの配列 */
static int current_code_size = -1;          /* 現在のコードサイズ */

static void checkCodeSize(void);            /* コードサイズの確認と, コードサイズ増加 TODO:ここでrealloc */

/* 次の命令語のアドレス(pc)を返す */
int nextCode(void)
{
  return current_code_size + 1;
}

/* オペランドに値(即値)をとる命令の生成 */
int genCodeValue(LVM_OpCode opcode, LL1LL_Value value)
{
  checkCodeSize();                          /* コードサイズ検査 */
  code[current_code_size].opcode  = opcode; /* オペコード */
  code[current_code_size].u.value = value;  /* オペランド */
  return current_code_size;                 /* 現在のコードサイズを返す */
}

/* オペランドに（変数）アドレスを取る命令の生成 */
int genCodeTable(LVM_OpCode opcode, int table_index)
{
  checkCodeSize();
  code[current_code_size].opcode    = opcode;
  code[current_code_size].u.address = getVarRelAddr(table_index); /* アドレスの取得 */
  return current_code_size;
}

/* 演算命令の生成 */
int genCodeCalc(LVM_OpCode opcode)
{
  checkCodeSize();
  code[current_code_size].opcode    = opcode;
  return current_code_size;
}

/* return命令の生成 */
int genCodeReturn(void)
{
  /* 直前の命令がreturnならば, 連続returnになるので
   * すぐに終わる */
  if (code[current_code_size].opcode == LVM_RETURN) 
    return current_code_size;

  /* コードサイズを確認してから命令生成 */
  /* オペランドのブロックレベルやパラメタ数は, table.hからの情報を使う */
  checkCodeSize();
  code[current_code_size].opcode = LVM_RETURN;
  code[current_code_size].u.address.block_level = getBlockLevel();
  code[current_code_size].u.address.address     = getCurrentNumParams();
  return current_code_size;
}

/* jump系命令の生成 */
int genCodeJump(LVM_OpCode opcode, int jump_pc)
{
  checkCodeSize();
  code[current_code_size].opcode    = opcode;
  code[current_code_size].u.jump_pc = jump_pc;
  return current_code_size;
}

/* トップ移動命令の生成 */
int genCodeMove(LVM_OpCode opcode, int move_top)
{
  checkCodeSize();
  code[current_code_size].opcode     = opcode;
  code[current_code_size].u.move_top = move_top;
  return current_code_size;
}

/* 現在のコードサイズをチェック.
 * 同時にコードサイズを増加 */
static void checkCodeSize(void)
{
  current_code_size++;
  if (current_code_size < MAX_CODE_SIZE) {
    return;
  } else {
    /* TODO:可変にして, メモリの限界まで使えるように */
    fprintf(stderr, "Too many codes... : %d TODO:Code size should be able to expand \n", current_code_size);
    exit(1);
  }
}

/* バックパッチ. 引数のジャンプ命令の飛び先アドレスに
 * 次の命令アドレスをセットする */
void backPatch(int program_count)
{
  code[program_count].u.jump_pc = current_code_size + 1;
}

/* pcのジャンプ命令の飛び先をjump_pcに変更する */
void changeJumpPc(int pc, int jump_pc)
{
  code[pc].u.jump_pc = jump_pc;
}

/* 命令列の最初の要素へのポインタを得る */
LVM_Instruction *get_instruction(void)
{
  return &(code[0]);
}

/* デバッグ用・pc番目の命令の表示 */
void printCode(int pc)
{
  /* ローカルにオペランドの取る種類 */
  typedef enum {
    OPRAND_IMMEDIATE,
    OPRAND_RELADDR,
    OPRAND_JUMP_PC,
    OPRAND_MOVE_TOP,
    OPRAND_VOID,
  } OprandKind;

  OprandKind oprand_kind;

  /* 命令で場合分けし, オペコード名を印字
   * オペランドの種類をセット */
  switch (code[pc].opcode) {
    case LVM_NOP:
      printf("nop");
      oprand_kind = OPRAND_VOID;
      break;
    case LVM_MOVE_STACK_P:
      printf("move_stack_pointer");
      oprand_kind = OPRAND_MOVE_TOP;
      break;
    case LVM_PUSH_IMMEDIATE:
      printf("push_immediate");
      oprand_kind = OPRAND_IMMEDIATE;
      break;
    case LVM_PUSH_VALUE:
      printf("push_value");
      oprand_kind = OPRAND_RELADDR;
      break;
    case LVM_POP_VARIABLE:
      printf("pop_variable");
      oprand_kind = OPRAND_RELADDR;
      break;
    case LVM_POP:
      printf("pop");
      oprand_kind = OPRAND_VOID;
      break;
    case LVM_DUPLICATE:
      printf("duplicate");
      oprand_kind = OPRAND_VOID;
      break;
    case LVM_JUMP:
      printf("jump");
      oprand_kind = OPRAND_JUMP_PC;
      break;
    case LVM_JUMP_IF_TRUE:
      printf("jump_if_true");
      oprand_kind = OPRAND_JUMP_PC;
      break;
    case LVM_JUMP_IF_FALSE:
      printf("jump_if_false");
      oprand_kind = OPRAND_JUMP_PC;
      break;
    case LVM_INVOKE:
      printf("invoke");
      oprand_kind = OPRAND_RELADDR;
      break;
    case LVM_RETURN:
      printf("return");
      oprand_kind = OPRAND_RELADDR;
      break;
    case LVM_MINUS:
      printf("minus");
      oprand_kind = OPRAND_VOID;
      break;
    case LVM_LOGICAL_NOT:
      printf("logical_not");
      oprand_kind = OPRAND_VOID;
      break;
    case LVM_INCREMENT:
      printf("increment");
      oprand_kind = OPRAND_VOID;
      break;
    case LVM_DECREMENT:
      printf("decrement");
      oprand_kind = OPRAND_VOID;
      break;
    case LVM_ADD:
      printf("add");
      oprand_kind = OPRAND_VOID;
      break;
    case LVM_SUB:
      printf("sub");
      oprand_kind = OPRAND_VOID;
      break;
    case LVM_MUL:
      printf("mul");
      oprand_kind = OPRAND_VOID;
      break;
    case LVM_DIV:
      printf("div");
      oprand_kind = OPRAND_VOID;
      break;
    case LVM_MOD:
      printf("mod");
      oprand_kind = OPRAND_VOID;
      break;
    case LVM_POW:
      printf("pow");
      oprand_kind = OPRAND_VOID;
      break;
    case LVM_LOGICAL_AND:
      printf("logical_and");
      oprand_kind = OPRAND_VOID;
      break;
    case LVM_LOGICAL_OR:
      printf("logical_or");
      oprand_kind = OPRAND_VOID;
      break;
    case LVM_EQUAL:
      printf("equal");
      oprand_kind = OPRAND_VOID;
      break;
    case LVM_NOT_EQUAL:
      printf("not_equal");
      oprand_kind = OPRAND_VOID;
      break;
    case LVM_GREATER:
      printf("greater");
      oprand_kind = OPRAND_VOID;
      break;
    case LVM_GREATER_EQUAL:
      printf("greater_equal");
      oprand_kind = OPRAND_VOID;
      break;
    case LVM_LESSTHAN:
      printf("lessthan");
      oprand_kind = OPRAND_VOID;
      break;
    case LVM_LESSTHAN_EQUAL:
      printf("lessthan_equal");
      oprand_kind = OPRAND_VOID;
      break;
    default:
      fprintf(stderr, "Error! mismatch opcode name in print_code\n");
      exit(1);
      break;
  }

  /* オペランドの種類に合わせて印字 */
  switch (oprand_kind) {
    case OPRAND_IMMEDIATE:
      /* 即値の場合は, 値を表示する */
      switch (code[pc].u.value.type) {
        case LL1LL_INT_TYPE:
          printf(", int:%d\n", code[pc].u.value.u.int_value);
          return;
        case LL1LL_DOUBLE_TYPE:
          printf(", double:%f\n", code[pc].u.value.u.double_value);
          return;
        case LL1LL_BOOLEAN_TYPE:
          if (code[pc].u.value.u.boolean_value == LL1LL_TRUE) {
            printf(", boolean:true\n");
          } else {
            printf(", boolean:false\n");
          }
          return;
        case LL1LL_OBJECT_TYPE:
          switch (code[pc].u.value.u.object->type) {
            case STRING_OBJECT:
              printf(", string:\"%s\"\n", 
                  code[pc].u.value.u.object->u.str.string_value);
              return;
            case ARRAY_OBJECT:
              /* 配列:これから... */
              return;
          }
        case LL1LL_NULL_TYPE:
          printf(", null\n");
          return;
      }
    case OPRAND_RELADDR:
      printf(", level:%d", code[pc].u.address.block_level);
      printf(", address:%d\n", code[pc].u.address.address);
      return;
    case OPRAND_JUMP_PC:
      printf(", jump_pc:%d\n", code[pc].u.jump_pc);
      return;
    case OPRAND_MOVE_TOP:
      printf(", move_top:%d\n", code[pc].u.move_top);
      return;
    case OPRAND_VOID: /* FALLTHRU */
    default:
      printf("\n");
      return;
  }

}

/* 全命令列の表示 */
void printCodeList(void)
{
  int i;
  printf("Code List: \n");
  for (i = 0; i <= current_code_size; i++) {
    printf("%4d: ", i);
    printCode(i);
  }
}
