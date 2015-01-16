// PDP11 Assembler (c) 15-01-2015 vinxru

#include <stdafx.h>
#include "lstwriter.h"
#include <string>
#include <fstream>
#include "compiler.h"

static int linelen(const char* p) {
  auto a = strchr(p, '\r'), b = strchr(p, '\n');
  if(a == 0 && b == 0) return strlen(p);
  if(a == 0 || b < a) return b - p;  
  return a - p;  
}

//-----------------------------------------------------------------------------

void LstWriter::appendBuffer(const char* data, int size) {
  auto newSize = buffer.size() + size; //! Тут может быть переполнение
  if(newSize > buffer.capacity()) {
    auto gran = buffer.capacity() + buffer.capacity()/2; //! Тут может быть переполнение
    if(newSize < gran) newSize = gran;
    buffer.reserve(newSize);
  }
  buffer.append(data, size);
}

//-----------------------------------------------------------------------------

void LstWriter::beforeCompileLine() {
  if(!out || !p) return;
  out->writePosChanged = false;
  prev_writePos = out->writePos;
  prev_sigCursor = p->sigCursor;
  char info[256];
  sprintf_s(info, "%04i %04X ", p->line, out->writePos);
  appendBuffer(info);
}

//-----------------------------------------------------------------------------

void LstWriter::afterCompileLine() {  
  if(!out) return;
  const int MAX_OPCODES = 3;
  char info[MAX_OPCODES*7 + 16];
  auto ptr = info;
  if(!out->writePosChanged) {
    int l = (out->writePos-prev_writePos)/2;
    if(l > MAX_OPCODES) l = MAX_OPCODES;
    for(; l > 0; l--) {
      sprintf_s(ptr, info+sizeof(info)-ptr, "%06o ", (unsigned int)(*(unsigned short*)(out->writeBuf + prev_writePos)));
      prev_writePos += 2;
      ptr += 7;
    }
  }
  memset(ptr, ' ', info+sizeof(info)-ptr);
  info[MAX_OPCODES*7] = 9;
  info[MAX_OPCODES*7+1] = 9;
  info[MAX_OPCODES*7+2] = 0;
  appendBuffer(info);
  appendBuffer(prev_sigCursor, linelen(prev_sigCursor));
  appendBuffer("\r\n");  
}

//-----------------------------------------------------------------------------

void LstWriter::writeFile(const char* fileName) {
  std::string fileName2;
  auto extSep = strrchr(fileName, '.');
  if(extSep && strrchr(extSep, '/') == 0 && strrchr(extSep, '\\') == 0) {
    fileName2.assign(fileName, extSep - fileName);      
  } else {
    fileName2 = fileName;
  }
  fileName2 += ".lst";
  std::ofstream file;
  file.open(fileName2);
  if(!file.is_open()) throw std::exception("Can't create lst file");
  file << buffer;
}