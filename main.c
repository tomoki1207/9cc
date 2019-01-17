#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "9cc.h"

// エラー報告用の関数
__attribute__((noreturn)) void error(char *format, ...) {
  va_list ap;
  va_start(ap, format);
  vfprintf(stderr, format, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// トークナイズした結果
Vector *tokens;

// パース結果
Vector *code;
Map *variable;

// --------------- Main
int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "引数の数が正しくありません\n");
    return 1;
  }

  if (strcmp(argv[1], "-test") == 0) {
    runtest();
    return 0;
  }

  // トークナイズしてパース
  // 結果はcodeに保存される
  tokens = new_vector();
  code = new_vector();
  variable = new_map();

  tokenize(argv[1]);
  program();

  // アセンブリの前半部分
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");

  // プロローグ
  // 変数分の領域を確保する
  printf("  push rbp\n");
  printf("  mov rbp, rsp\n");
  printf("  sub rsp, %d\n", variable->keys->len * 8);

  for (int i = 0; code->data[i]; i++) {
    // 抽象構文木を下りながらコード生成
    gen((Node *)code->data[i]);

    // 式の評価結果値がスタックに1つ残っているはずなので
    // スタックが溢れないようにpop
    printf("  pop rax\n");
  }

  // エピローグ
  // RAXに残っているの最後の式の結果が返り値になる
  printf("  mov rsp, rbp\n");
  printf("  pop rbp\n");
  printf("  ret\n");
  return 0;
}

