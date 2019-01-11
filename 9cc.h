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


extern Token tokens[100];
extern Node *code[100];
extern int pos;

Node *program();
void gen(Node *node);
__attribute__((noreturn)) void error(char *format, ...);

