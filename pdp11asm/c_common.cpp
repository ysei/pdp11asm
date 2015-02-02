// PDP11 Assembler (c) 15-01-2015 vinxru

#include "stdafx.h"
#include "compiler.h"
#include <fstream>
#include <string.h>
#ifndef WIN32
#include <unistd.h> // для chdir в Linux
#endif

static unsigned char cp1251_to_koi8r_tbl[256] = {
  0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
  32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,
  64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,
  96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
  128,129,130,131,132,133,134,135,136,137,060,139,140,141,142,143,144,145,146,147,148,169,150,151,152,153,154,062,176,157,183,159,
  160,246,247,074,164,231,166,167,179,169,180,060,172,173,174,183,156,177,073,105,199,181,182,158,163,191,164,062,106,189,190,167,
  225,226,247,231,228,229,246,250,233,234,235,236,237,238,239,240,242,243,244,245,230,232,227,254,251,253,154,249,248,252,224,241,
  193,194,215,199,196,197,214,218,201,202,203,204,205,206,207,208,210,211,212,213,198,200,195,222,219,221,223,217,216,220,192,209
};

//-----------------------------------------------------------------------------

static void cp1251_to_koi8r(char* str) {
  for(;*str; str++)
    *str = (char)cp1251_to_koi8r_tbl[(unsigned char)*str];
}

//-----------------------------------------------------------------------------

void loadStringFromFile(std::string& buf, syschar_t* fileName) {
  std::fstream file(fileName, std::ifstream::in|std::ifstream::binary);
  if(!file.is_open()) throw std::runtime_error("Can't open source file");
  std::streamoff size = file.rdbuf()->pubseekoff(0, std::fstream::end);
  if(size < 0 || size >= std::numeric_limits<size_t>::max()) throw std::runtime_error("Source file too large");
  buf.resize((size_t)size);
  file.rdbuf()->pubseekoff(0, std::fstream::beg);  
  file.rdbuf()->sgetn(const_cast<char*>(buf.c_str()), buf.size());
}

//-----------------------------------------------------------------------------

static void chdirToFile(syschar_t* fileName) {
#ifdef WIN32
  wchar_t *a = wcsrchr(fileName, '/'), *b = wcsrchr(fileName, '\\');
  if(a==0 || b>a) a = b;
  if(a) _wchdir(std::wstring(fileName, (size_t)(a-fileName)).c_str());
#else // LINUX
  char *a = strrchr(fileName, '/');
  if(a) chdir(std::string(fileName, a-fileName).c_str());
#endif
}

//-----------------------------------------------------------------------------

Compiler::Compiler() {
  convert1251toKOI8R = false;
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

bool Compiler::ifConst4(Parser::num_t& out, bool numIsLabel) {
  if(regInParser()) return false;
  if(numIsLabel && p.ifToken(ttInteger)) {
    makeLocalLabelName();
    goto itsLabel;
  }
  if(p.ifToken(ttWord)) {
itsLabel:
    std::map<std::string, Parser::num_t>::iterator l = labels.find(p.loadedText);
    if(l != labels.end()) {
      out = l->second;
      return true;
    }
    if(step2) p.syntaxError(p.loadedText);
    out = 16384;
    return true;
  }
  if(p.ifToken(ttString1)) {
    out = (unsigned char)p.loadedText[0]; 
    if(convert1251toKOI8R) out = cp1251_to_koi8r_tbl[out]; 
    return true; 
  }
  if(p.ifToken(ttInteger)) { 
    out = p.loadedNum; 
    return true;
  }
  Parser::Label l(p);
  if(p.ifToken("-")) { 
    if(p.ifToken(ttInteger)) {
      out = 0-p.loadedNum;
      return true; 
    }
    p.jump(l);
  }
  if(p.ifToken(".")) { 
    out = this->out.writePtr;
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------

bool Compiler::ifConst3(Parser::num_t& a, bool numIsLabel) {
  if(!ifConst4(a, numIsLabel)) return false;
  static const char* ops[] = { "+", "-" };
  int o;
  while(p.ifToken(ops, o)) {
    Parser::num_t b;
    if(!ifConst4(b, numIsLabel)) p.syntaxError();
    switch(o) {
      case 0: a += b; break;
      case 1: a -= b; break;
    }
  }
  return true;
}

//-----------------------------------------------------------------------------

Parser::num_t Compiler::readConst3(bool numIsLabel) {
  Parser::num_t i;
  if(!ifConst3(i, numIsLabel)) p.syntaxError();
  return i;
}

//-----------------------------------------------------------------------------

void Compiler::compileOrg() {
  p.needToken(ttInteger);
  if(p.loadedNum > sizeof(out.writeBuf)) p.syntaxError();
  out.writePosChanged = true;
  out.writePtr = (size_t)p.loadedNum;
  return;
}

//-----------------------------------------------------------------------------

void Compiler::compileByte() {
  for(;;) {
    Parser::num_t c;
    if(ifConst3(c)) {
      if(p.ifToken("dup")) {
        p.needToken("(");
        Parser::num_t d = readConst3();
        if(d>std::numeric_limits<unsigned char>::max()) p.syntaxError();
        p.needToken(")");
        for(;c>0; c--) out.write(&d, 1);
      } else {
        if(c>std::numeric_limits<unsigned char>::max()) p.syntaxError();
        out.write(&c, 1);
      }
    } else {
      p.needToken(ttString2);
      if(convert1251toKOI8R) cp1251_to_koi8r(p.loadedText);
      out.write(p.loadedText, strlen(p.loadedText));
    }
    if(!p.ifToken(",")) break;
  }
}

//-----------------------------------------------------------------------------

void Compiler::compileWord() {
  for(;;) {
    Parser::num_t c = readConst3();
    if(p.ifToken("dup")) {
      p.needToken("(");
      Parser::num_t d = readConst3();
      if(d>std::numeric_limits<unsigned short>::max()) p.syntaxError();
      p.needToken(")");
      for(;c>0; c--) out.write((short)d);
    } else {
      if(c>std::numeric_limits<unsigned short>::max()) p.syntaxError();
      out.write((short)c);
    }
    if(!p.ifToken(",")) break;
  }
}

//-----------------------------------------------------------------------------

void Compiler::makeLocalLabelName() {
  // p.loadedNum is unsigned
  if(p.loadedNum > std::numeric_limits<int>::max()) p.syntaxError();
  sprintf_s(p.loadedText, sizeof(p.loadedText), "%s@%i", lastLabel, (int)p.loadedNum);
}

//-----------------------------------------------------------------------------

void Compiler::compileLine() {
retry:
  // Метки и константы определяются по второму слову
  Parser::Label l(p);
  p.nextToken();
  bool label = (p.token==ttOperator && p.tokenText[0]==':' && p.tokenText[1]==0);
  bool equ   = (p.token==ttWord && 0==strcmp(p.tokenText, "EQU")) || (p.token==ttOperator && 0==strcmp(p.tokenText, "="));
  p.jump(l);
  
  // Это метка
  if(label) {
    if(p.ifToken(ttInteger)) {
      makeLocalLabelName();
      labels[p.loadedText] = out.writePtr;
    } else {
      p.needToken(ttWord);
      labels[p.loadedText] = out.writePtr;
      strcpy_s(lastLabel, p.loadedText);
    }
    p.needToken(":");    
    // После метки может идти команда
    if(p.token != ttEol) goto retry;
    return;
  }

  // Это константа
  if(equ) {    
    p.needToken(ttWord);         
    Parser::TokenText name;
    strcpy_s(name, p.loadedText);
    if(!p.ifToken("=")) p.needToken("equ");
    Parser::num_t a = readConst3();
    if(!step2) labels[name] = a;
    return;
  }

  // Установка адреса
  if(p.ifToken("org")) {
    compileOrg();
    return;
  }

  // Команды MACRO11
  if(p.ifToken(".")) {
    // Вставить байты
    if(p.ifToken("byte")) { 
      compileByte();
      return; 
    }
    if(p.ifToken("word")) { 
      compileWord(); 
      return; 
    }
    if(p.ifToken("end")) { 
      return; 
    }
    if(p.ifToken("link")) { 
      compileOrg();
      return; 
    }
    if(p.ifToken("blkb")) { 
      p.needToken(ttInteger); 
      for(;p.tokenNum>0; p.tokenNum--) out.write("", 1);
      return;
    }
    if(p.ifToken("blkw")) { 
      p.needToken(ttInteger);
      for(;p.tokenNum>0; p.tokenNum--) out.write(0); 
      return;
    }
    p.altstring = '/';
    if(p.ifToken("ascii")) {      
      p.altstring = 0;      
      p.needToken(ttString2);
      if(convert1251toKOI8R) cp1251_to_koi8r(p.loadedText);
      out.write(p.loadedText, strlen(p.loadedText));
      return;
    }
    p.altstring = 0;
    p.syntaxError();
  }

  // Создание выходного файла
  bool make_binary_file = p.ifToken("make_binary_file");
  if(make_binary_file || p.ifToken("make_bk0010_rom")) {
    p.needToken(ttString2);
    Parser::TokenText fileName;
    strcpy_s(fileName, p.loadedText);
    size_t start = 0, stop = out.writePtr;
    if(p.ifToken(",")) {
      start = ullong2size_t(readConst3());
      if(p.ifToken(",")) stop = ullong2size_t(readConst3());
    }
    // Работает только на втором проходе
    if(step2) {
      if(stop<=start || stop>sizeof(out.writeBuf)) p.syntaxError();
      size_t length = stop - start;
      std::ofstream f;
      f.open(fileName, std::ofstream::binary|std::ofstream::out);
      if(!f.is_open()) p.syntaxError("Can't create file");      
      if(!make_binary_file) {
        f.write((const char*)&start, 2);
        f.write((const char*)&length, 2);
      }
      f.write(out.writeBuf+start, length);
      f.close();
      lstWriter.writeFile(fileName);
    }
    return;
  }

  if(p.ifToken("convert1251toKOI8R")) {
    convert1251toKOI8R = !p.ifToken("OFF");
    return;
  }

  if(p.ifToken("decimalnumbers")) {
    p.cfg_decimalnumbers = !p.ifToken("OFF");
    return;
  }

  if(p.ifToken("insert_file")) {
    p.needToken(ttString2);
    Parser::TokenText fileName;
    strcpy_s(fileName, p.loadedText);
    long long start=0, size=0;
    if(p.ifToken(",")) {
      start = ullong2size_t(readConst3());
      if(p.ifToken(",")) size = ullong2size_t(readConst3());
    }
    std::ifstream f;
    if(size==0 || step2) {
      f.open(fileName, std::ifstream::binary|std::ifstream::in);
      if(!f.is_open()) p.syntaxError("Can't open file");
      if(size==0) size = f.rdbuf()->pubseekoff(0, std::ifstream::end);  //! Тут может быть переполнение
    }
    if(size<0 || out.writePtr+size>=65536) p.syntaxError();
    if(step2) {
      f.rdbuf()->pubseekoff(start, std::ifstream::beg);
      f.rdbuf()->sgetn(out.writePtr+out.writeBuf, size);
    }
    out.writePtr += (int)size; // Переполнение проверено выше
    return;
  }

  // Выровнять код
  if(p.ifToken("align")) {
    p.needToken(ttInteger);
    if(p.loadedNum < 1 || p.loadedNum >= std::numeric_limits<size_t>::max()) p.syntaxError();
    size_t a = size_t(p.loadedNum);
    out.writePtr = (out.writePtr + a - 1) / a * a;
    return;
  }
  // Вставить байты
  if(p.ifToken("db")) {
    compileByte();
    return;
  }
  // Вставить слово
  if(p.ifToken("dw")) {
    compileWord();
    return;
  }
  if(compileLine_pdp11()) return;
  if(compileLine_bitmap()) return;
  p.syntaxError();
}

//-----------------------------------------------------------------------------

void Compiler::compileFile(syschar_t* fileName) {
  // Загрузка файла
  loadStringFromFile(p.source, fileName);

  // Установка пути для INCLUDE-файлов
  chdirToFile(fileName);

  // Два прохода
  for(int s=0; s<2; s++) {
    step2 = s==1;
    p.init(p.source.c_str());
    out.init();      
    strcpy(lastLabel, "undefined");
    while(!p.ifToken(ttEof)) {
      if(p.ifToken(ttEol)) continue;
      if(step2) lstWriter.beforeCompileLine();
      compileLine();
      if(step2) lstWriter.afterCompileLine();        
      if(p.ifToken(ttEof)) break;
      //! Теперь в одной строке может быть много команд  p.needToken(ttEol);
    }
  }
}
