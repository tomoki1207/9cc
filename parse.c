#include <stdio.h>
#include <stdlib.h>

#include "9cc.h"

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

// ---------- Parse

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

