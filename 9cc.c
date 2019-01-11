#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// トークンの型を表す値
enum {
  TK_NUM = 256, // 整数
  TK_IDENT, // 識別子
  TK_EOF, // 入力の終わり
};

// トークンの型
typedef struct {
  int ty; // トークンの型
  int val; // tyがTK_NUMの場合の数値
  char *input; // トークン文字列(エラーメッセージ用)
} Token;

// ノードの型を表す値
enum {
  ND_NUM = 256, // 整数のノードの型
  ND_IDENT // 識別子のノードの型
};

// ノードの型
typedef struct {
  int ty; // 演算子かND_NUM
  struct Node *lhs; // 左辺
  struct Node *rhs; // 右辺
  int val; // tyがND_NUMの場合のみ使う
  char name; // tyがND_IDENTの場合のみ使う
} Node;

void tokenize(char *p);
Node *new_node(int ty, Node *lhs, Node *rhs);
Node *new_node_num(int val);
Node *new_node_ident(char name);
Node *program();
Node *stmt();
Node *assign();
Node *add();
Node *mul();
Node *term();
int consume(int ty);
void gen(Node *node);
void gen_lval(Node *node);
void error(char *format, int i);

// --------------- Tokenize

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

    error("トークナイズできません: %s\n", i);
  }

  tokens[i].ty = TK_EOF;
  tokens[i].input = p;
}

// --------------- Syntax tree

// パース結果
Node *code[100];

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

Node *new_node_ident(char name) {
  Node *node = malloc(sizeof(Node));
  node->ty = ND_IDENT;
  node->name = name;
  return node;
}

Node *term() {
  if (consume('(')) {
    Node *node = add();
    if (!consume(')')) {
      error("'('に対応する')'がありません: %s", pos);
    }
    return node;
  }

  if (tokens[pos].ty == TK_NUM) {
    return new_node_num(tokens[pos++].val);
  }

  if (tokens[pos].ty == TK_IDENT) {
    return new_node_ident(tokens[pos++].val);
  }

  error("無効なトークンです: %s", pos);
}

Node *mul() {
  Node *node = term();

  for (;;) {
    if (consume('*')) {
      node = new_node('*', node, term());
    } else if (consume('/')) {
      node = new_node('/', node, term());
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

Node *assign() {
  Node *node = add();

  for (;;) {
    if (consume('=')) {
      node = new_node('=', node, assign());
    } else {
      return node;
    }
  }
}

Node *stmt() {
  Node *node = assign();
  if (!consume(';')) {
    error("';'ではないトークンです: %s", pos);
  }
  return node;
}

Node *program() {
  int i = 0;
  while(tokens[pos].ty != TK_EOF) {
    code[i++] = stmt();
  }
  code[i] = NULL;
}

// 次のトークンの型が期待した型であれば1つ読み進める
int consume(int ty) {
  if (tokens[pos].ty != ty) {
    return 0;
  }
  pos++;
  return 1;
}

// Nodeから命令を生成する
void gen(Node *node) {
  if (node->ty == ND_NUM) {
    printf("  push %d\n", node->val);
    return;
  }

  if (node->ty == ND_IDENT) {
    gen_lval(node);
    printf("  pop rax\n");
    printf("  mov rax, [rax]\n");
    printf("  push rax\n");
    return;
  }

  if (node->ty == '=') {
    gen_lval(node->lhs);
    gen(node->rhs);

    printf("  pop rdi\n");
    printf("  pop rax\n");
    printf("  mov [rax], rdi\n");
    printf("  push rdi\n");
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

// Nodeをlvalueとする命令を生成する
void gen_lval(Node *node) {
  if (node->ty != ND_IDENT) {
    error2("代入の左辺値が変数ではありません");
  }

  int offset = ('z' - node->name + 1) * 8;
  printf("  mov rax, rbp\n");
  printf("  sub rax, %d\n", offset);
  printf("  push rax\n");
}

// --------------- Common

void error(char *format, int i) {
  fprintf(stderr, format, tokens[i].input);
  exit(1);
}
void error2(char *format) {
  fprintf(stderr, format);
  exit(1);
}

// --------------- Main
int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "引数の数が正しくありません\n");
    return 1;
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

