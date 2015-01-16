// PDP11 Assembler (c) 15-01-2015 vinxru

#pragma once
#include <string>
#include "output.h"
#include "parser.h"

class LstWriter {
public:
  std::string buffer;
  int prev_writePos;
  const char* prev_sigCursor;
  Output* out;
  Parser* p;

  inline LstWriter() { prev_writePos=0; prev_sigCursor=0; out=0; p=0; }
  void beforeCompileLine();
  void afterCompileLine();
  void writeFile(const char* fileName);

protected:
  void appendBuffer(const char* data, int size);
  inline void appendBuffer(const char* data) { appendBuffer(data, strlen(data)); }
};
