// PDP11 Assembler (c) 15-01-2015 vinxru

#pragma once

#include <exception>

class Output {
public:
  char writeBuf[65536];
  int writePos;
  bool writePosChanged;

  inline Output() {
    init();
  }

  inline void init() {
    writePos = 0;
    writePosChanged = 0;
    memset(writeBuf, 0, sizeof(writeBuf));
  }

  inline void write(void* data, int n) {
    if(writePos + n > 65536) throw std::exception("64K overflow");
    memcpy(writeBuf + writePos, data, n);
    writePos += n;
  }

  inline void write(short n) { 
    write(&n, 2); 
  }
};