// PDP11 Assembler (c) 15-01-2015 vinxru

#include <stdafx.h>
#include "parser.h"
#include <limits.h>

static std::string i2s(int i) {
  char buf[32];
  _itoa_s(i, buf, 10);
  return buf;
}

//-----------------------------------------------------------------------------

static bool tryMulAddI(int& a, int b, int c) {
  __int64 x =__int64(a)*__int64(b);
  if(x < INT_MIN || x > INT_MAX) return false;
  x += c;
  if(x < INT_MIN || x > INT_MAX) return false;
  a = int(x);
  return true;
}

//-----------------------------------------------------------------------------

static int findx(const char** a, const std::string& s, int si) {
  for(int i=0; *a; a++, i++)
    if(0==strncmp(*a, s.c_str(), si))
      return i;
  for(int i=0; *a; a++, i++)
    if(*a==s)
      return i;
  return -1;
}

//-----------------------------------------------------------------------------

Parser::Parser() {
  cfg_caseSel=false;
  cfg_cescape=false;
  cfg_remark = 0;
  cfg_operators = 0;

  sigCursor = prevCursor = cursor = 0;
  sigLine = prevLine = line = 1;
  sigCol = prevCol = col = 1;

  token = ttEof;
  tokenInt = 0;
  tokenText[0] = 0;

  loadedInt = 0;
  loadedText[0] = 0;
}

//-----------------------------------------------------------------------------

void Parser::getLabel(Label& label) {
  label.col = prevCol;
  label.line = prevLine;
  label.cursor = prevCursor;
}

//-----------------------------------------------------------------------------

void Parser::init(const char* buf, bool dontCallNextToken) {
  line = col = 1;
  prevCursor = cursor = buf;
  token = ttEof;   
  nextToken();
}

//-----------------------------------------------------------------------------

void Parser::nextToken() {
  // Сохраняем прошлый токен
  switch(token) {
    case ttWord:
    case ttString1:
    case ttString2:
      strcpy_s(loadedText, tokenText);
      break;
    case ttInteger:
      loadedInt = tokenInt;
  }

  // sig - это до пробелов
  sigCol=col-1;
  sigLine=line;
  sigCursor = cursor;

  do {
    for(;*cursor==32 || *cursor==13 || (!cfg_eol && *cursor==10) || *cursor==9; cursor++)
      if(*cursor==10) { line++; col=1; } else
      if(*cursor!=13) col++;

    // Парсим
    tokenText[0]=0;

    // Пропускаем пробелы
    prevLine=line;
    prevCol =col;
    prevCursor=cursor;

    while(true) {
      char c=*cursor;    
      if(c==0) {
        token=ttEof;
        return;
      }
      if(c!=' ' && c!=10 && c!=13 && c!=9) break;    
      cursor++;
      if(c==10) line++, col=1;
      if(cfg_eol && c==10) {
        token=ttEol;          
        return;
      }
    }

    //
    const char* s=cursor;
    nextToken2();

    // Увеличиваем курсор
    for(;s<cursor;s++)
      if(*s==10) { line++; col=1; } else
      if(*s!=13) col++;
  } while(token==ttComment);
}

//-----------------------------------------------------------------------------

bool Parser::ifToken(const char* t) {
  if(token==ttWord || token==ttOperator) { 
    if(cfg_caseSel) {
      if(0 == strcmp(tokenText, t)) { nextToken(); return true; }
    } else {
      if(0 == _strcmpi(tokenText, t)) { nextToken(); return true; }
    }    
  }
  return false;
}

//-----------------------------------------------------------------------------

void Parser::nextToken2() {
  int tokenText_ptr = 0;

  char c=*cursor++;

  if(c=='_' || (c>='a' && c<='z') || (c>='A' && c<='Z')) {
    while(true) {
      if(tokenText_ptr == maxTokenText) syntaxError("Too large word");
      tokenText[tokenText_ptr++]=c;
      c = *cursor;
      if(!(c=='_' || (c>='0' && c<='9') || (c>='a' && c<='z') || (c>='A' && c<='Z'))) break;
      cursor++;
    }
    tokenText[tokenText_ptr]=0;
    token=ttWord;

    if(!cfg_caseSel)
      for(char* p = tokenText, *pe = tokenText + tokenText_ptr; p < pe; p++)
        *p = ::toupper(*p);
    return;
  }

  if(c=='\'' || c=='"') {
    char quoter=c;
    while(true) {
      c=*cursor;
      if(c==0 || c==10 || c==13) syntaxError("Unterminated string");
      cursor++; 
      if(!cfg_cescape) {
        if(c==quoter) {
          if(*cursor!=quoter) break;
          cursor++; 
        } else 
        if(c==quoter) break;
      } else {
        if(c=='\\') { 
          c=*cursor++;
          if(c==0 || c==10) syntaxError("Unterminated string"); else
          if(c=='n') c='\n'; else
          if(c=='r') c='\r'; else
          if(c=='\\') c='\\'; else
          if(c=='\'') c='\''; else
          if(c=='"') c='"'; else
          if(c=='x') {
            char c1=*cursor++;
            if(c1==0 || c1==10) syntaxError("Unterminated string");
            if(c1>='0' && c1<='9') c1-='0'; else
            if(c1>='a' && c1<='f') c1-='a'-10; else
            if(c1>='A' && c1<='F') c1-='A'-10; else
              syntaxError("Unknown esc");
            char c2=*cursor++;
            if(c2==0 || c2==10) syntaxError("Unterminated string");
            if(c2>='0' && c2<='9') c2-='0'; else
            if(c2>='a' && c2<='f') c2-='a'-10; else
            if(c2>='A' && c2<='F') c2-='A'-10; else
              syntaxError("Unknown esc");
            c=(c1<<4)+c2;
          } else {
            syntaxError("Unknown esc");
          }
        } else 
        if(c==quoter) break;
      }
      if(tokenText_ptr==maxTokenText) syntaxError("Too large string");
      tokenText[tokenText_ptr++]=c;
    }        
    tokenText[tokenText_ptr]=0;
    token=quoter=='\'' ? ttString1 : ttString2;
    return;
  }

  if(c>='0' && c<='9') {
    token = ttInteger;
    int n = 0;
    // Если число начинается с 0x - то читаем 16-ричное
    if(c=='0' && (cursor[0]=='x' || cursor[0]=='X')) {
      cursor++; // Пропускаем X
      while(true) {        
        c=*cursor;      
        int e;
        if(c>='0' && c<='9') e=c-'0';    else
        if(c>='a' && c<='f') e=c-'a'+10; else
        if(c>='A' && c<='F') e=c-'A'+10; else break;
        if(!tryMulAddI(n,16,e)) syntaxError("Too large number");
        cursor++;
      }
      tokenInt = n;
      return;
    }
    // Если число начинается с 0 - то читаем 8-ричное
    if(c=='0') {
      while(true) {        
        c=*cursor;      
        int e;
        if(c>='0' && c<='7') e=c-'0'; else break;
        if(!tryMulAddI(n,8,e)) syntaxError("Too large number");
        cursor++;
      }
      tokenInt = n;
      return;
    }
    // Читаем целое, плавающе и двоичное число 
    bool canBits = (c=='0' || c=='1'); // Удалось посчитать Bits (нет переполнения, алфавит)
    bool canInt = true; // Удалось посчитать Integer (нет переполнения)
    n = (c - '0');
    int b = n;
    while(true) {
      c = *cursor;
      if(c<'0' || c>'9') break;
      c -= '0';
      if(canInt && !tryMulAddI(n,10,c)) canInt=false;      
      canBits &= (c=='0' || c=='1');
      if(canBits) { if(b&0x80000000) canBits=false; else { b<<=2; if(c=='1') b|=1; } }
      cursor++;
    }
    if(c=='b' || c=='B') { 
      if(!canBits) syntaxError("Too large number");
      cursor++; 
      tokenInt = b;
      return; 
    }
    if(!canInt) syntaxError("Too large number");
    tokenInt = n; 
    return;
  }
 
  // Одиночный символ
  token = ttOperator;
  tokenText[0] = c;
  tokenText[1] = 0;

  // Составной оператор
  if(cfg_operators) {
    // Добавляем остальные символы
    tokenText_ptr = 1;
    while(true) {
      c = *cursor;
      if(c==0) break;
      tokenText[tokenText_ptr] = c;
      tokenText[tokenText_ptr+1] = 0;
      if(findx(cfg_operators, tokenText, tokenText_ptr+1)==-1) break;
      cursor++;
      tokenText_ptr++;
      if(tokenText_ptr==maxTokenText) syntaxError("Too large operator");
    }
    tokenText[tokenText_ptr]=0;
  }
  // Комментарии
  if(cfg_remark) {
    for(int j=0; cfg_remark[j]; j++) {
      if(0==_strcmpi(tokenText,cfg_remark[j])) {
        while(true) {
          c=*cursor;
          if(c==0 || c==10) break;
          cursor++;
        }
        token=ttComment;
        return;
      }
    }
  }
}

//-----------------------------------------------------------------------------

void Parser::syntaxError(const char* text) {  
  throw std::exception((fileName + "(" + i2s(prevLine) + "," + i2s(prevCol) + "): " + (text[0]==0 ? "Syntax error" : text)).c_str());
}

//-----------------------------------------------------------------------------

void Parser::jump(Label& label) {
  sigCursor = cursor = prevCursor = label.cursor;
  sigLine = prevLine = line = label.line;
  sigCol = prevCol = col = label.col;
  nextToken();
}

//-----------------------------------------------------------------------------

bool Parser::ifToken(const char** a) {
  for(const char** i = a; *i; i++) {
    if(ifToken(*i)) {
      loadedInt = i - a;
      return true;
    }
  }
  return false;
}
