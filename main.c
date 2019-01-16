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

// --------------- Tokenize

Token *add_token(int ty, char *input) {
  Token *token = malloc(sizeof(Token));
  token->ty = ty;
  token->input = input;
  vec_push(tokens, (void *)token);
  return token;
}

Token get_token(int i) {
  return *((Token *)(tokens->data[i]));
}

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
      add_token(*p, p);
      i++;
      p++;
      continue;
    }

    if ('a' <= *p && *p <= 'z') {
      Token *token = add_token(TK_IDENT, p);
      token->val = *p;
      i++;
      p++;
      continue;
    }

    if (isdigit(*p)) {
      Token *token = add_token(TK_NUM, p);
      token->val = strtol(p, &p, 10);
      i++;
      continue;
    }

    error("トークナイズできません: %s\n", p);
  }

  add_token(TK_EOF, p);
}

// パース結果
Vector *code;

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
  tokens = new_vector();
  code = new_vector();
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

