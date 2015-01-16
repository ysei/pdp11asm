// PDP11 Assembler (c) 15-01-2015 vinxru

#pragma once
#include "lstwriter.h"
#include "parser.h"
#include "output.h"

class Compiler {
public:
  LstWriter lstWriter;
  Parser p;
  Output out;
  bool step2;
  std::map<std::string, int> labels;

  // c_common.cpp
  Compiler();
  void compileFile(wchar_t* fileName);
  void compileLine();
  bool ifConst(int& out);
  int  readConst();

  // c_bitmap.cpp
  bool compileLine_bitmap();

  // c_pdp11.cpp
  struct Arg {
    int ext, code;  
    bool needExt1, subip;
  };

  void write(int n, Arg& a);
  void write(int n, Arg& a, Arg& b);
  bool regInParser();
  int  readReg();
  void readArg(Arg& a);
  bool compileLine_pdp11();
};

