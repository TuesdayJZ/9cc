#include<stdio.h>
#include<stdlib.h>
#include<ctype.h>
#include<stdarg.h>
#include<stdbool.h>
#include<string.h>

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

Token *token; //現在のトークン linked list になる

//エラーをprintするための関数
//printfと同じ引数を取る
void error(char *fmt, ...){
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

//次のトークンが期待している記号のときは1つ読み進めてtrueを返す
bool consume(char op){
  if (token->kind != TK_RESERVED || token->str[0] != op) return false;
  token = token->next;
  return true;
}

//次のトークンが期待している記号のときは1つ読み進める
void expect(char op){
  if(token->kind != TK_RESERVED || token->str[0] != op)
    error("'%c'ではありません", op);
  token = token->next;
}

//次のトークンが数値の場合1つ読み進めてその数値を返す
int expect_number(){
  if (token->kind != TK_NUM) error("数ではありません");
  int val = token->val;
  token = token->next;
  return val;
}

//次のトークンがEOFの場合trueを返す
bool at_eof() { return token->kind == TK_EOF; }

//新しいトークンを作ってcurrentに繋げる
Token *new_token(TokenKind kind, Token *current, char *str){
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  current->next = tok;
  return tok;
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

    //+か=だったら新しいトークンを作ってcurrentに繋げる
    if (*p == '+' || *p == '-') {
      current = new_token(TK_RESERVED, current, p++);
      continue;
    }

    //数字だったらcurrentに繋げてvalに数値をセット
    if(isdigit(*p)){
      current = new_token(TK_NUM, current, p);
      current->val = strtol(p, &p, 10);
      continue;
    }

    error("トークナイズできません");
  }

  new_token(TK_EOF, current, p);
  return head.next;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "引数の個数が正しくありません\n");
    return 1;
  }

  //入力文字列をトークナイズする
  token = tokenize(argv[1]);

  //アセンブリの冒頭部分を出力
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");

  //式の最初が数字になっているかチェックしてmov命令を出力
  printf("  mov rax, %d\n", expect_number());

  // '+ <数>' または '- <数>' の並びを消費しつつアセンブリを出力
  while(!at_eof()){
    if(consume('+')){
      printf("  add rax, %d\n", expect_number());
      continue;
    }

    /*if(consume('-')){
      printf("  sub rax, %d\n", expect_number());
      continue;
    }*/

    expect('-');
    printf("  sub rax, %d\n", expect_number());
  }

  printf("  ret\n");
  return 0;
}
