// PDP11 Assembler (c) 15-01-2015 vinxru

#include <stdafx.h>
#include "compiler.h"

struct SimpleCommand {
  const char* name;
  int code;
};

struct ImmCommand {
  const char* name;
  int code, max;
};

//-----------------------------------------------------------------------------

inline void Compiler::write(int n, Arg& a) {
  out.write(n);
  if(a.needExt1) out.write(a.ext - (a.subip ? out.writePos+2 : 0) );  
}

//-----------------------------------------------------------------------------

inline void Compiler::write(int n, Arg& a, Arg& b) {
  out.write(n);
  if(a.needExt1) out.write(a.ext - (a.subip ? out.writePos+2 : 0) );  
  if(b.needExt1) out.write(b.ext - (b.subip ? out.writePos+2 : 0) );  
}

//-----------------------------------------------------------------------------

int Compiler::readReg() {
  const char* regs[] = { "R0","R1","R2","R3","R4","R5","SP","PC",0 };
  return p.needToken(regs);
}

//-----------------------------------------------------------------------------

bool Compiler::regInParser() {
  if(p.token==ttWord && p.tokenText[2]==0) {
    if(p.tokenText[0]=='R' && (p.tokenText[1]>='0' || p.tokenText[1]<='5')) return true;
    if(p.tokenText[0]=='P' && p.tokenText[1]=='C') return true;
    if(p.tokenText[0]=='S' && p.tokenText[1]=='P') return true;
  }
  return false;
}

//-----------------------------------------------------------------------------

void Compiler::readArg(Arg& a) {
  a.subip = false;

  int mode, reg;
  bool x = p.ifToken("@");
  bool n = p.ifToken("#");

  int ii;
  if(ifConst(ii)) { // Вычесть из адреса смещение, если не поствалены @ #
    a.subip = (!x && !n); 
    a.ext = ii;
    a.needExt1 = true;
    Parser::Label pl(p);
    if(!n && p.ifToken("(")) {
      if(p.ifToken(")")) { p.jump(pl); goto xxx; }
      mode = 6;
      reg = readReg();
      if(!x) a.subip=0;
      p.needToken(")");
    } else {
xxx:
      reg = 7;
      mode = n ? 2 : 6;
    }
    if(x) mode++;
    a.code = (mode<<3) | reg;
    return;
  }
  if(n) p.syntaxError();
  bool d = p.ifToken("-");
  a.needExt1 = false;
  if(!d) {
    int ii;
    if(ifConst(ii)) {
      a.subip = !x && !n;
      a.needExt1 = true;
      a.ext = ii;
    }
  }
  bool o = p.ifToken("("); if((x || d || a.needExt1) && !o) p.needToken("(");  
  reg = readReg();
  if(o) p.needToken(")");
  bool i = false;
  if(o && (!d && !a.needExt1)) i = p.ifToken("+");
  if(x && !d && !i && !a.needExt1) { a.needExt1=true; a.ext=0; }
  mode = !o ? 0 : i ? 2 : d ? 4 : a.needExt1 ? 6 : 1;
  if(x) mode++;
  a.code = (mode<<3) | reg;
}

//-----------------------------------------------------------------------------

bool Compiler::compileLine_pdp11() {
  // Комманды без аргументов
  static SimpleCommand simpleCommands[] = {
    "halt", 0, "wait", 1, "rti", 2, "bpt", 3, "iot", 4, "reset", 5, "rtt", 6, "nop", 0240,
    "clc", 0241, "clv", 0242, "clz", 0244, "cln", 0250, "sec", 0261, "sev", 0262, 
    "sez", 0264, "sen", 0270, "scc", 0277, "ccc", 0257, 0
  };

  if(p.ifToken(simpleCommands)) {
    out.write(simpleCommands[p.loadedInt].code);
    return true;
  }

  // Комманды с одним регистром

  static SimpleCommand oneCommands[] = {
    "jmp", 00001, "swab", 00003,
    "clr", 00050, "clrb", 01050, "com",  00051, "comb", 01051, 
    "inc", 00052, "incb", 01052, "dec",  00053, "decb", 01053, 
    "neg", 00054, "negb", 01054, "adc",  00055, "adcb", 01055, 
    "sbc", 00056, "sbcb", 01056, "tst",  00057, "tstb", 01057, 
    "ror", 00060, "rorb", 01060, "rol",  00061, "rolb", 01061, 
    "asr", 00062, "asrb", 01062, "asl",  00063, "aslb", 01063, 
    "sxt", 00067, "mtps", 01064, "mfps", 01067, 0
  };

  if(p.ifToken(oneCommands)) {
    int n = p.loadedInt;
    Arg a;
    readArg(a);
    write((oneCommands[n].code<<6)|a.code, a);
    return true;
  }

  // Комманды перехода
  
  static SimpleCommand jmpCommands[] = { 
    "br",  00004, "bne",  00010, "beq", 00014, "bge", 00020, "blt", 00024, 
    "bgt", 00030, "ble", 00034, "bpl",  01000, "bmi", 01004, "bhi", 01010, 
    "bvc", 01024, "bhis", 01030, "bcc", 01030, "blo", 01034, "bcs", 01034,
    "blos", 01014, 
    0 
  };

  if(p.ifToken(jmpCommands)) {
    int n = p.loadedInt;
    int i = readConst();
    i-=out.writePos+2;
    if(i&1) p.syntaxError("unaligned");
    i/=2;
    if(step2 && (i<-128 || i>127)) p.syntaxError("Too far jump");
    out.write((jmpCommands[n].code<<6) | (i&0xFF));
    return true;
  }

  // Комманды с константой
  
  static ImmCommand immCommands[] = {
    "emt", 0104000, 0377, "trap", 104400, 0377, "mark", 0006400, 077, 0
  };

  if(p.ifToken(immCommands)) {
    int n = p.loadedInt;
    p.needToken(ttInteger);
    if(p.loadedInt<0 || p.loadedInt>immCommands[n].max) p.syntaxError();
    out.write(immCommands[n].code | p.loadedInt);
    return true;
  }

  // Комманды с двумя регистрами

  static const char* twoCommands[] = { 
    "", "mov", "cmp", "bit", "bic", "bis", "add", "", "",
    "movb", "cmpb", "bitb", "bicb", "bisb", "sub", 0
  };

  if(p.ifToken(twoCommands)) {
    int n = p.loadedInt;
    Arg src, dest;
    readArg(src);
    p.needToken(",");
    readArg(dest);
    write((n<<12)|(src.code<<6)|dest.code, src, dest);
    return true;
  }

  // Остальные команды
  
  static SimpleCommand aCommands[] = {
    "jsr", 004000, "xor", 0074000, 0
  };

  if(p.ifToken(aCommands)) {
    int n = p.loadedInt;
    int r = readReg();
    p.needToken(",");
    Arg a;
    readArg(a);
    write(aCommands[n].code | (r<<6) | a.code, a);
    return true;
  }

  if(p.ifToken("sob")) {
    int r = readReg();
    p.needToken(",");
    int n = (out.writePos+2) - readConst();
    if(n&1) p.syntaxError();
    n/=2;
    if(n<0 || n>63) p.syntaxError();
    out.write(0077000 | (r<<6) | (n&077));
    return true;
  }

  if(p.ifToken("rts")) {
    int r = readReg();
    out.write(0000200 | r);
    return true;
  }

  return false;
}