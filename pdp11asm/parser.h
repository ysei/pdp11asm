// PDP11 Assembler (c) 15-01-2015 vinxru

#pragma once
#include <string>
#include <map>

enum Token { ttEof, ttEol, ttWord, ttInteger, ttOperator, ttString1, ttString2, ttComment };

class Parser {
public:
  // Настройки
  const char** cfg_operators;
  const char** cfg_remark;
  bool cfg_caseSel;
  bool cfg_eol;
  bool cfg_cescape;

  // Для вывода ошибки
  std::string fileName; 

  // Буфер
  std::string source;

  // Курсор
  const char *cursor, *prevCursor, *sigCursor;
  int line, prevLine, sigLine;
  int col, prevCol, sigCol;

  // Загруженный токен
  Token token;
  static const int maxTokenText = 256;
  char tokenText[maxTokenText+1];
  int tokenInt;

  // Прошлый токен
  char loadedText[maxTokenText+1];
  int loadedInt;

  //  Методы
  Parser();
  void init(const char* text, bool dontCallNextToken=false);
  void nextToken();
  void nextToken2();

  // Переходы
  struct Label {
    const char* cursor;
    int line, col;
    
    inline Label() { cursor=0; line=0; col=0; }
    inline Label(Parser& p) { p.getLabel(*this); }
  };
  void getLabel(Label&);
  void jump(Label&);

  // Ошибка
  void syntaxError(const char* text = "");

  // Сравнение
  inline bool Parser::ifToken(Token t) { if(token != t) return false; nextToken(); return true; }
  inline void needToken(Token t) { if(token != t) syntaxError(); nextToken(); }
  bool ifToken(const char* text);  
  inline void needToken(const char* text) { if(!ifToken(text)) syntaxError(); }
  bool ifToken(const char** a);
  inline int needToken(const char** a) { if(!ifToken(a)) syntaxError(); return loadedInt; }

  template<class T> inline bool ifToken(T* a) {    
    for(T* i = a; i->name; i++) {
      if(ifToken(i->name)) {
        loadedInt = i - a;
        return true;
      }
    }
    return false;
  }
};
