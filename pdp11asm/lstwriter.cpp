// PDP11 Assembler (c) 15-01-2015 vinxru

#include "stdafx.h"
#include "lstwriter.h"
#include <string>
#include <fstream>
#include <assert.h>
#include "compiler.h"

static size_t linelen(const char* p) {
  const char *a = strchr(p, '\r'), *b = strchr(p, '\n');
  if(a == 0 && b == 0) return strlen(p);
  if(a == 0 || b < a) return (size_t)(b - p);
  return (size_t)(a - p);  
}

//-----------------------------------------------------------------------------

void LstWriter::appendBuffer(const char* data, size_t size) {
  size_t newSize = buffer.size() + size; //! ��� ����� ���� ������������
  if(newSize > buffer.capacity()) {
    size_t gran = buffer.capacity() + buffer.capacity()/2; //! ��� ����� ���� ������������
    if(newSize < gran) newSize = gran;
    buffer.reserve(newSize);
  }
  buffer.append(data, size);
}

//-----------------------------------------------------------------------------

void LstWriter::beforeCompileLine() {
  if(!out || !p) return;
  out->writePosChanged = false;
  prev_writePtr = out->writePtr;
  prev_sigCursor = p->sigCursor;
  char info[256];
  sprintf_s(info, sizeof(info), "%04i %04X ", int(p->line), int(out->writePtr));
  appendBuffer(info);
}

//-----------------------------------------------------------------------------

void LstWriter::afterCompileLine() {  
  if(!out) return;
  const int MAX_OPCODES = 3;
  char info[MAX_OPCODES*7 + 16];
  char* ptr = info;
  if(!out->writePosChanged) {
    if(!hexMode) {
      size_t l = (out->writePtr - prev_writePtr) / 2;
      if(l > MAX_OPCODES) l = MAX_OPCODES;
      for(; l > 0; l--) {
        ptr += sprintf_s(ptr, info+sizeof(info)-ptr, "%06o ", (unsigned int)(*(unsigned short*)(out->writeBuf + prev_writePtr)));
        prev_writePtr += 2;
      }
    } else {
      size_t l = (out->writePtr - prev_writePtr);
      if(l > MAX_OPCODES) l = MAX_OPCODES;
      for(; l > 0; l--) {
        ptr += sprintf_s(ptr, info+sizeof(info)-ptr, "%02X ", (unsigned int)(*(unsigned char*)(out->writeBuf + prev_writePtr)));
        prev_writePtr ++;
      }
    }
  }
  memset(ptr, ' ', info+sizeof(info)-ptr); // ������ �� ��������� - ��������� �� ���� size_t!
  info[MAX_OPCODES*7] = 9;
  info[MAX_OPCODES*7+1] = 9;
  info[MAX_OPCODES*7+2] = 0;
  appendBuffer(info);
  appendBuffer(prev_sigCursor, linelen(prev_sigCursor));
  appendBuffer("\r\n");  
}

//-----------------------------------------------------------------------------

void replaceExtension(std::string& out, const char* fileName, const char* ext) {
  const char* extSep = strrchr(fileName, '.');
  size_t fileNameLen = (extSep && strrchr(extSep, '/') == 0 && strrchr(extSep, '\\') == 0) ? (extSep - fileName) : strlen(fileName);    
  out.reserve(fileNameLen + strlen(ext));
  out.assign(fileName, fileNameLen);
  out += ext;  
}

//-----------------------------------------------------------------------------

void LstWriter::writeFile(const char* fileName) {
  std::string fileName2;
  replaceExtension(fileName2, fileName, ".lst");
  if(fileName != fileName2) {
    std::ofstream file;
    file.open(fileName2.c_str());
    if(!file.is_open()) throw std::runtime_error(("Can't create lst file (" + fileName2 + ")").c_str());
    file << buffer;
  }
}
