#include <stdio.h>
#include <stdlib.h>

#include "9cc.h"

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

// 次のトークンの型が期待した型であれば1つ読み進める
int consume(int ty) {
  if (get_token(pos).ty != ty) {
    return 0;
  }
  pos++;
  return 1;
}

Node *add();

Node *term() {
  if (consume('(')) {
    Node *node = add();
    if (!consume(')')) {
      error("'('に対応する')'がありません: %s", get_token(pos).input);
    }
    return node;
  }

  if (get_token(pos).ty == TK_NUM) {
    return new_node_num(get_token(pos++).val);
  }

  if (get_token(pos).ty == TK_IDENT) {
    return new_node_ident(get_token(pos++).val);
  }

  error("無効なトークンです: %s", get_token(pos).input);
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
    error("';'ではないトークンです: %s", get_token(pos).input);
  }
  return node;
}

void program() {
  while(get_token(pos).ty != TK_EOF) {
    vec_push(code, (void *)stmt());
  }
  vec_push(code, NULL);
}

