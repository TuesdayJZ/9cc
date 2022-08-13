#include<stdio.h>
#include<stdlib.h>
#include<ctype.h>
#include<stdarg.h>
#include<stdbool.h>
#include<string.h>

/* tokenizer */

//トークンの種類
typedef enum {
    TK_RESERVED, //記号
    TK_IDENT,    //識別子
    TK_NUM,      //整数トークン
    TK_EOF,      //入力の終わり
} TokenKind;

typedef struct Token Token;

struct Token {
    TokenKind kind; //トークンの型
    Token *next;    //次の入力トークン
    int val;        //kindがTK_NUMの場合、その数値
    char *str;      //トークン文字列
    int len;       //トークンの長さ
};

char *user_input; //入力プログラム
Token *token;  //現在のトークン linked list になる

//エラーを報告するための関数
void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

//改良版・エラーを報告するための関数
void error_at(char *location, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  int position = location - user_input;
  fprintf(stderr, "%s\n", user_input);
  fprintf(stderr, "%*s", position, " ");
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

//次のトークンが期待している記号のときは1つ読み進めてtrueを返す
bool consume(char *op){
  //長い記号から先にtokenizeする必要がある
  if (token->kind != TK_RESERVED || strlen(op) != token->len || memcmp(token->str, op, token->len)) return false;
  token = token->next;
  return true;
}

//次のトークンが期待している記号のときは1つ読み進める
void expect(char *op){
  if(token->kind != TK_RESERVED || strlen(op) != token->len || memcmp(token->str, op, token->len))
    error_at(token->str, "'%s'ではありません", op);
  token = token->next;
}

//次のトークンが数値の場合1つ読み進めてその数値を返す
int expect_number(){
  if (token->kind != TK_NUM) error_at(token->str, "数ではありません");
  int val = token->val;
  token = token->next;
  return val;
}

//次のトークンがEOFの場合trueを返す
bool at_eof() { return token->kind == TK_EOF; }

//新しいトークンを作ってcurrentに繋げる
Token *new_token(TokenKind kind, Token *current, char *str, int len){
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  tok->len = len;
  current->next = tok;
  return tok;
}

bool startswith(char *p, char *q){
  return memcmp(p, q, strlen(q)) == 0;
}

//入力文字列pをトークナイズしてそれを返す
Token *tokenize(char *p) {
  Token head;
  head.next = NULL;
  Token *current = &head;

  while(*p){
    //空白文字をスキップ
    if (isspace(*p)) {
      p++;
      continue;
    }

    //数字以外だったら新しいトークンを作ってcurrentに繋げる

    if(startswith(p, "==") || startswith(p, "!=") || startswith(p, "<=") || startswith(p, ">=")){
      current = new_token(TK_RESERVED, current, p, 2);
      p += 2;
      continue;
    }

    if (strchr("+-*()/<>", *p)) {
      current = new_token(TK_RESERVED, current, p++, 1);
      continue;
    }

    //数字だったらcurrentに繋げてvalに数値をセット
    if(isdigit(*p)){
      current = new_token(TK_NUM, current, p, 0);
      char *q = p;
      current->val = strtol(p, &p, 10);
      current->len = p - q;
      continue;
    }

    error_at(p, "無効なトークンです");
  }

  new_token(TK_EOF, current, p, 0);
  return head.next;
}

/* parser */

//抽象構文木のノードの種類
typedef enum {
  ND_ADD,
  ND_SUB,
  ND_MUL,
  ND_DIV,
  ND_NUM,
  ND_EQ,
  ND_NE,
  ND_LT,
  ND_LE,
} NodeKind;

typedef struct Node Node;

//抽象構文木のノードの型
struct Node {
  NodeKind kind;    //ノードの型
  Node *leftside;   //左辺
  Node *rightside;  //右辺
  int val;          // ND_NUMの場合その値
};

//新しいノードを作成 数字以外の場合
Node *new_node(NodeKind kind, Node *leftside, Node *rightside){
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->leftside = leftside;
  node->rightside = rightside;
  return node;
}

//新しいノードを作成 数字の場合
Node *new_node_num(int val) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_NUM;
  node->val = val;
  return node;
}

Node *expr();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();

//expr = equality
Node *expr() {
  return equality();
}

//equality = relational ("==" relational | "!=" relational)*
Node *equality() {
  Node *node = relational();
  for (;;) {
    if (consume("=="))
      node = new_node(ND_EQ, node, relational());
    else if (consume("!="))
      node = new_node(ND_NE, node, relational());
    else
      return node;
  }
}

//relational = add ("<" add | "<=" add | ">" add | ">=" add)*
Node *relational() {
  Node *node = add();

  for(;;) {
    if (consume("<"))
      node = new_node(ND_LT, node, add());
    else if (consume ("<="))
      node = new_node(ND_LE, node, add());
    else if (consume (">"))
      node = new_node(ND_LT, add(), node);
    else if (consume (">="))
      node = new_node(ND_LE, add(), node);
    else
      return node;
  }
}

//add = mul ("+" mul | "-" mul)*
Node *add() {
  Node *node = mul();
  for(;;) {
    if(consume("+"))
      node = new_node(ND_ADD, node, mul());
    else if(consume("-"))
      node = new_node(ND_SUB, node, mul());
    else
      return node;
  }
}

Node *mul() {
  Node *node = unary();
  for (;;) {
    if (consume("*"))
      node = new_node(ND_MUL, node, unary());
    else if (consume("/"))
      node = new_node(ND_DIV, node, unary());
    else
      return node;
  }
}

Node *unary(){
  if(consume("+"))
    return unary();
  if(consume("-"))
    return new_node(ND_SUB, new_node_num(0), unary());
  return primary();
}

Node *primary(){
  //()の中に入っているのはexprのはず
  if(consume("(")){
    Node *node = expr();
    expect(")");
    return node;
  }
  //そうでなかったら数字
  return new_node_num(expect_number());
}

//アセンブリを出力
void gen(Node *node){
  if (node->kind == ND_NUM) {
    printf("  push %d\n", node->val);
    return;
  }

  gen(node->leftside);
  gen(node->rightside);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->kind) {
    case ND_ADD:
      printf("  add rax, rdi\n");
      break;
    case ND_SUB:
      printf("  sub rax, rdi\n");
      break;
    case ND_MUL:
      printf("  imul rax, rdi\n");
      break;
    case ND_DIV:
      printf("  cqo\n");
      printf("  idiv rdi\n");
      break;
    case ND_EQ:
      printf("  cmp rax, rdi\n");
      printf("  sete al\n");
      printf("  movzb rax, al\n");
      break;
    case ND_NE:
      printf("  cmp rax, rdi\n");
      printf("  setne al\n");
      printf("  movzb rax, al\n");
      break;
    case ND_LT:
      printf("  cmp rax, rdi\n");
      printf("  setl al\n");
      printf("  movzb rax, al\n");
      break;
    case ND_LE:
      printf("  cmp rax, rdi\n");
      printf("  setle al\n");
      printf("  movzb rax, al\n");
      break;
  }

  printf("  push rax\n");
}

int main(int argc, char **argv) {
  if (argc != 2) error("%s: 引数の個数が正しくありません\n", argv[0]);

  //入力文字列をトークナイズする
  user_input = argv[1];
  token = tokenize(user_input);
  Node *node = expr();

  //アセンブリの冒頭部分を出力
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");

  //抽象構文木をアセンブリに変換して出力
  gen(node);

  //アセンブリの後半部分を出力
  printf("  pop rax\n");
  printf("  ret\n");
  return 0;
}
