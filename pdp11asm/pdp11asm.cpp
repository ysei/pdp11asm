// PDP11 Assembler (c) 15-01-2015 vinxru

#include "stdafx.h"
#include "compiler.h"
#include <iostream>
#include <tchar.h>

int _tmain(int argc, _TCHAR* argv[]) {
  try {
    // ���������
    setlocale(LC_ALL, "RUSSIAN");

    // ��������� ���� ��������
    if(argc!=2) {
      std::cout << "Specify one file name on the command line" << std::endl;
      return 0;
    }
    
    // ��������� �������
    Compiler c;
    c.compileFile(argv[1]);

    // ����� ��� ������
    std::cout << "Done" << std::endl;
  	return 0;

    // ����� � ��������
  } catch(std::exception e) {
    std::cout << e.what() << std::endl;
    return 1;
  }
}