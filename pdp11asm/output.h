// PDP11 Assembler (c) 15-01-2015 vinxru

#pragma once

#include <string.h>
#include <stdexcept>

class Output {
public:
  char writeBuf[65536];
  size_t writePtr;
  bool writePosChanged;

  inline Output() {
    init();
  }

  inline void init() {
    writePtr = 0;
    writePosChanged = 0;
    memset(writeBuf, 0, sizeof(writeBuf));
  }

  inline void write(void* data, size_t n) {
    if(writePtr + n > 65536) throw std::runtime_error("64K overflow");
    memcpy(writeBuf + writePtr, data, n);
    writePtr += n;
  }

  inline void write(int n) { 
    write(&n, 2); 
  }
};
