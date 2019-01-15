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

// トークナイズした結果の配列
// 100個以上は来ないものとする
Token tokens[100];

// --------------- Tokenize

// pをトークナイズしてtokensに保存する
void tokenize(char *p) {
  int i = 0;
  while (*p) {
    if (isspace(*p)) {
      p++;
      continue;
    }

    if (*p == '+' || *p == '-' || *p == '*' || *p == '/'
          || *p == '(' || *p == ')' || *p == '=' || *p == ';') {
      tokens[i].ty = *p;
      tokens[i].input = p;
      i++;
      p++;
      continue;
    }

    if ('a' <= *p && *p <= 'z') {
      tokens[i].ty = TK_IDENT;
      tokens[i].input = p;
      tokens[i].val = *p;
      i++;
      p++;
      continue;
    }

    if (isdigit(*p)) {
      tokens[i].ty = TK_NUM;
      tokens[i].input = p;
      tokens[i].val = strtol(p, &p, 10);
      i++;
      continue;
    }

    error("トークナイズできません: %s\n", p);
  }

  tokens[i].ty = TK_EOF;
  tokens[i].input = p;
}

// パース結果
Node *code[100];

// パースしているトークンの現在位置
int pos = 0;

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
  tokenize(argv[1]);
  program();

  // アセンブリの前半部分
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");

  // プロローグ
  // 変数26個分の領域を確保する
  printf("  push rbp\n");
  printf("  mov rbp, rsp\n");
  printf("  sub rsp, 208\n");

  int i = 0;
  for (i = 0; code[i]; i++) {
    // 抽象構文木を下りながらコード生成
    gen(code[i]);

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

