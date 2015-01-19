// PDP11 Assembler (c) 15-01-2015 vinxru

#include <string.h>
#include <string>

#ifndef WIN32

// � Windows � Linux ���������� �� ������� (����, ��� �������� � ������ ����)
#define sprintf_s snprintf

// strcpy_s ���� �� �������������� � Linux (G++)
inline void strcpy_s(char* a, const char* b) { strcpy(a, b); }

// � Windows � Linux ���������� �� �������
inline int _strcmpi(const char* a, const char* b) { return strcasecmp(a, b); }

// ��������� ������ � Linux
typedef char syschar_t;

#else

// ����� � Windows
#include <tchar.h>

// ��������� ������ � Windows
typedef wchar_t syschar_t;

// ������������� ? � ��������� switch � ������������� ? �� ��������������
#pragma warning(disable:4062) 

#endif