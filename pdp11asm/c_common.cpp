// PDP11 Assembler (c) 15-01-2015 vinxru

#include "stdafx.h"
#include "compiler.h"
#include <fstream>

static void cp1251_to_koi8(char* str) {
  static unsigned char tbl[] = {
    128,129,130,131,132,133,134,135,136,137,060,139,140,141,142,143,
    144,145,146,147,148,169,150,151,152,153,154,062,176,157,183,159,
    160,246,247,074,164,231,166,167,179,169,180,060,172,173,174,183,
    156,177,073,105,199,181,182,158,163,191,164,062,106,189,190,167,
    225,226,247,231,228,229,246,250,233,234,235,236,237,238,239,240,
    242,243,244,245,230,232,227,254,251,253,154,249,248,252,224,241,
    193,194,215,199,196,197,214,218,201,202,203,204,205,206,207,208,
    210,211,212,213,198,200,195,222,219,221,223,217,216,220,192,209
  };

  for(;*str; str++)
    if(*str & 0x80)
      *str = tbl[*str & 0x7F];
}

void loadStringFromFile(std::string& buf, wchar_t* fileName) {
  std::fstream file;
  file.open(fileName, std::ifstream::in|std::ifstream::binary);
  if(!file.is_open()) throw std::exception("Can't open file");
  auto size = file.rdbuf()->pubseekoff(0, std::fstream::end);
  if(size < 0 || size >= UINT_MAX) throw std::exception("File too large");
  buf.resize((unsigned int)size);
  file.rdbuf()->pubseekoff(0, std::fstream::beg);  
  file.rdbuf()->sgetn(const_cast<char*>(buf.c_str()), buf.size());
}

//-----------------------------------------------------------------------------

static void chdirToFile(wchar_t* fileName) {
  auto a = wcsrchr(fileName, '/'), b = wcsrchr(fileName, '\\');
  if(a==0 && b==0) return;
  if(a==0 || b>a) a = b;
  std::wstring path;
  path.assign(fileName, a-fileName);
  _wchdir(path.c_str());
}

//-----------------------------------------------------------------------------

Compiler::Compiler() {
  lstWriter.out = &out;
  lstWriter.p = &p;
  p.cfg_eol = true;
  p.cfg_caseSel = false;
  static const char* asmRem[] = { ";", "//", 0 };
  p.cfg_remark = asmRem;
  static const char* asmOp[] = { "//", 0 };
  p.cfg_operators = asmOp;
}

//-----------------------------------------------------------------------------

bool Compiler::ifConst(int& out) {
  if(regInParser()) return false;
  if(p.ifToken(ttWord)) {
    auto l = labels.find(p.loadedText);
    if(l != labels.end()) {
      out = l->second;
      return true;
    }
    if(step2) p.syntaxError(p.loadedText);
    out = 16384;
    return true;
  }
  if(p.ifToken(ttString1)) { out = (unsigned char)(p.loadedText[0]); return true; }
  if(p.ifToken(ttInteger)) { out = p.loadedInt; return true; }
  Parser::Label l(p);
  if(p.ifToken("-")) { 
    if(p.ifToken(ttInteger)) {
      out = -p.loadedInt;
      if(out>=0) p.syntaxError("Too large integer");
      return true; 
    }
    p.jump(l);
  }
  return false;
}

//-----------------------------------------------------------------------------

int Compiler::readConst() {
  int i;
  if(!ifConst(i)) p.syntaxError();
  return i;
}

//-----------------------------------------------------------------------------

void Compiler::compileLine() {
retry:
  // Метки и константы определяются по второму слову
  Parser::Label l(p);
  p.nextToken();
  bool label = (0==_strcmpi(p.tokenText, ":"));
  bool equ   = (0==_strcmpi(p.tokenText, "equ"));
  p.jump(l);
  
  // Это метка
  if(label) {
    p.needToken(ttWord);
    if(!step2) labels[p.loadedText] = out.writePos;
    p.needToken(":");    
    // После метки может идти команда
    if(p.token != ttEol) goto retry;
    return;
  }

  // Это константа
  if(equ) {    
    p.needToken(ttWord);     
    char name[Parser::maxTokenText];
    strcpy_s(name, p.loadedText);
    p.needToken("equ");
    int a = readConst();
    if(!step2) labels[name] = a;
    return;
  }

  // Установка адреса
  if(p.ifToken("org")) {
    p.needToken(ttInteger);
    if(p.loadedInt<0 || p.loadedInt>=65536) p.syntaxError();
    out.writePosChanged = true;
    out.writePos = p.loadedInt;
    return;
  }

  // Создание выходного файла
  if(p.ifToken("make_bk0010_rom")) {
    p.needToken(ttString2);
    char fileName[Parser::maxTokenText];
    strcpy_s(fileName, p.loadedText);
    int start = 0, stop = out.writePos;
    if(p.ifToken(",")) {
      start = readConst();
      if(p.ifToken(",")) stop = readConst();
    }
    // Работает только на втором проходе
    if(step2) {
      if(stop<=start || start<0 || stop>=65536) p.syntaxError();
      int length = stop - start;
      std::ofstream f;
      f.open(fileName, std::ofstream::binary|std::ofstream::out);
      if(!f.is_open()) p.syntaxError("Can't create file");      
      f.write((const char*)&start, 2);
      f.write((const char*)&length, 2);
      f.write(out.writeBuf+start, length);
      f.close();
      lstWriter.writeFile(fileName);
    }
    return;
  }

  if(p.ifToken("insert_file")) {
    p.needToken(ttString2);
    char fileName[Parser::maxTokenText];
    strcpy_s(fileName, p.loadedText);
    __int64 start=0, size=0;
    if(p.ifToken(",")) {
      start = readConst();
      if(p.ifToken(",")) size = readConst();
    }
    std::ifstream f;
    if(size==0 || step2) {
      f.open(fileName, std::ifstream::binary|std::ifstream::in);
      if(!f.is_open()) p.syntaxError("Can't open file");
      if(size==0) size = f.rdbuf()->pubseekoff(0, std::ifstream::end);  //! Тут может быть переполнение
    }
    if(size<0 || out.writePos+size>=65536) p.syntaxError();
    if(step2) {
      f.rdbuf()->pubseekoff(start, std::ifstream::beg);
      f.rdbuf()->sgetn(out.writePos+out.writeBuf, size);
    }
    out.writePos += (int)size; // Переполнение проверено выше
    return;
  }

  // Выровнять код
  if(p.ifToken("align")) {
    p.needToken(ttInteger);
    if(p.loadedInt < 1) p.syntaxError();
    out.writePos = (out.writePos + p.loadedInt - 1) / p.loadedInt * p.loadedInt;    
    return;
  }

  // Вставить байты
  if(p.ifToken("db")) {
    while(true) {
      int arg;
      if(ifConst(arg)) {
        if(p.ifToken("dup")) {
          p.needToken("(");
          int d = readConst();
          if(d<0 || d>255) p.syntaxError();
          p.needToken(")");
          for(;arg>0; arg--) out.write(&d, 1);
        } else {
          if(arg<0 || arg>255) p.syntaxError();
          out.write(&arg, 1);
        }
      } else {
        p.needToken(ttString2);
        cp1251_to_koi8(p.loadedText);
        out.write(p.loadedText, strlen(p.loadedText));
      }
      if(!p.ifToken(",")) break;
    }
    return;
  }

  // Вставить слово
  if(p.ifToken("dw")) {
    while(true) {
      int c = readConst();
      if(p.ifToken("dup")) {
        p.needToken("(");
        int d = readConst();
        if(d<0 || d>65535) p.syntaxError();
        p.needToken(")");
        for(;c>0; c--) out.write(&d, 2);
      } else {
        if(c<INT_MIN || c>65535) p.syntaxError();
        out.write(c);
      }
      if(!p.ifToken(",")) break;
    }
    return;
  }
  if(compileLine_pdp11()) return;
  if(compileLine_bitmap()) return;
  p.syntaxError();
}

//-----------------------------------------------------------------------------

void Compiler::compileFile(wchar_t* fileName) {
  // Загрузка файла
  loadStringFromFile(p.source, fileName);

  // Установка пути для INCLUDE-файлов
  chdirToFile(fileName);

  // Два прохода
  for(int s=0; s<2; s++) {
    step2 = s==1;
    p.init(p.source.c_str());
    out.init();      
    while(!p.ifToken(ttEof)) {
      if(p.ifToken(ttEol)) continue;
      if(step2) lstWriter.beforeCompileLine();
      compileLine();
      if(step2) lstWriter.afterCompileLine();        
      if(p.ifToken(ttEof)) break;
      p.needToken(ttEol);
    }
  }
}