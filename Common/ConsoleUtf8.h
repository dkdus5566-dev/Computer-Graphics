#pragma once
// 콘솔 한글 출력 설정 (모델러·게임 공용)
//   Shared.props의 /utf-8 옵션으로 문자열이 UTF-8로 저장되므로,
//   콘솔도 UTF-8로 맞춰야 한국어/영어 Windows 어디서나 한글이 깨지지 않는다.
//   main() 시작 부분에서 한 번 호출한다.
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

inline void InitConsoleUtf8()
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
}
