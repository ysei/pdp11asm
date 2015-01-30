// PDP11 Assembler (c) 15-01-2015 vinxru

#pragma once
#include <string>
#include <map>

enum Token { ttEof, ttEol, ttWord, ttInteger, ttOperator, ttString1, ttString2, ttComment };

class Parser {
public:
  typedef unsigned long long num_t;
  static const int maxTokenText = 256;
  typedef char TokenText[maxTokenText];

  // ���������
  const char** cfg_operators;
  const char** cfg_remark;
  bool cfg_caseSel;
  char altstring;
  bool cfg_eol;
  bool cfg_cescape;
  bool cfg_decimalnumbers; // ����� �� ��������� 10-����, ����� 8-������

  // ��� ������ ������
  std::string fileName; 

  // �����
  std::string source;

  // ������
  const char *cursor, *prevCursor, *sigCursor;
  size_t line, prevLine, sigLine;
  size_t col, prevCol, sigCol;

  // ����������� �����
  Token token;
  TokenText tokenText;
  num_t tokenNum;  

  // ������� �����
  TokenText loadedText;
  num_t loadedNum;

  //  ������
  Parser();
  void init(const char* text);
  void nextToken();
  void nextToken2();

  // ��������
  struct Label {
    const char* cursor;
    size_t line, col;
    
    inline Label() { cursor=0; line=0; col=0; }
    inline Label(Parser& p) { p.getLabel(*this); }
  };
  void getLabel(Label&);
  void jump(Label&);

  // ������
  void syntaxError(const char* text = 0);

  // ���������
  inline bool ifToken(Token t) { if(token != t) return false; nextToken(); return true; }
  inline void needToken(Token t) { if(token != t) syntaxError(); nextToken(); }
  bool ifToken(const char* text);  
  inline void needToken(const char* text) { if(!ifToken(text)) syntaxError(); }
  bool ifToken(const char** a, int& n);
  inline int needToken(const char** a) { int n; if(!ifToken(a, n)) syntaxError(); return n; }

  template<class T> inline bool ifToken(T* a, int& n) {    
    for(T* i = a; i->name; i++) {
      if(ifToken(i->name)) {
        n = i - a;
        return true;
      }
    }
    return false;
  }
};
