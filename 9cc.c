#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --------------- Tokenize

// トークンの型を表す値
enum {
  TK_NUM = 256, // 整数
  TK_EOF, // 入力の終わり
};

// トークンの型
typedef struct {
  int ty; // トークンの型
  int val; // tyがTK_NUMの場合の数値
  char *input; // トークン文字列(エラーメッセージ用)
} Token;

// トークナイズした結果の配列
// 100個以上は来ないものとする
Token tokens[100];

// pをトークナイズしてtokensに保存する
void tokenize(char *p) {
  int i = 0;
  while (*p) {
    if (isspace(*p)) {
      p++;
      continue;
    }

    if (*p == '+' || *p == '-' || *p == '*' || *p == '/') {
      tokens[i].ty = *p;
      tokens[i].input = p;
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

    fprintf(stderr, "トークナイズできません: %s\n", p);
    exit(1);
  }

  tokens[i].ty = TK_EOF;
  tokens[i].input = p;
}

// --------------- Syntax tree

enum {
  ND_NUM = 256 // 整数のノードの型
};

typedef struct {
  int ty; // 演算子かND_NUM
  struct Node *lhs; // 左辺
  struct Node *rhs; // 右辺
  int val; // tyがND_NUMの場合のみ使う
} Node;

// パースしているトークンの現在位置
int pos = 0;

Node *new_node(int ty, Node *lhs, Node *rhs) {
  Node *node = malloc(sizeof(Node));
  node->ty = ty;
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node *new_node_num(int val) {
  Node *node = malloc(sizeof(Node));
  node->ty = ND_NUM;
  node->val = val;
  return node;
}

Node *num() {
  if (tokens[pos].ty == TK_NUM) {
    return new_node_num(tokens[pos++].val);
  }

  error(pos);
}

Node *mul() {
  Node *node = num();

  for (;;) {
    if (consume('*')) {
      node = new_node('*', node, num());
    } else if (consume('/')) {
      node = new_node('/', node, num());
    } else {
      return node;
    }
  }
}

Node *add() {
  Node *node = mul();

  for (;;) {
    if (consume('+')) {
      node = new_node('+', node, mul());
    } else if (consume('-')) {
      node = new_node('-', node, mul());
    } else {
      return node;
    }
  }
}

// 次のトークンの型が期待した型であれば1つ読み進める
int consume(int ty) {
  if (tokens[pos].ty != ty) {
    return 0;
  }
  pos++;
  return 1;
}

// スタックマシンをエミュレートするような命令を生成する
void gen(Node *node) {
  if (node->ty == ND_NUM) {
    printf("  push %d\n", node->val);
    return;
  }

  gen(node->lhs);
  gen(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->ty) {
    case '+':
      printf("  add rax, rdi\n");
      break;
    case '-':
      printf("  sub rax, rdi\n");
      break;
    case '*':
      printf("  mul rdi\n");
      break;
    case '/':
      printf("  mov rdx, 0\n");
      printf("  div rdi\n");
  }

  printf("  push rax\n");
}

// --------------- Common

void error(int i) {
  fprintf(stderr, "予期しないトークンです: %s\n", tokens[i].input);
  exit(1);
}

// --------------- Main
int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "引数の数が正しくありません\n");
    return 1;
  }

  // トークナイズしてパース
  tokenize(argv[1]);
  Node *node = add();

  // アセンブリの前半部分
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");

  // 抽象構文木を下りながらコード生成
  gen(node);

  // スタックトップにある式全体の値をロードして返り値にする
  printf("  pop rax\n");
  printf("  ret\n");
  return 0;
}

