// PDP11 Assembler (c) 15-01-2015 vinxru

#pragma once
#include <string>
#include "output.h"
#include "parser.h"

class LstWriter {
public:
  std::string buffer;
  size_t prev_writePtr;
  const char* prev_sigCursor;
  Output* out;
  Parser* p;

  inline LstWriter() { prev_writePtr=0; prev_sigCursor=0; out=0; p=0; }
  void beforeCompileLine();
  void afterCompileLine();
  void writeFile(const char* fileName);

protected:
  void appendBuffer(const char* data, size_t size);
  inline void appendBuffer(const char* data) { appendBuffer(data, strlen(data)); }
};
