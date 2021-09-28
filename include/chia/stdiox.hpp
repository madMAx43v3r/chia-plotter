
#ifndef INCLUDE_CHIA_STDIOX_HPP_
#define INCLUDE_CHIA_STDIOX_HPP_
#include <stdio.h>

#ifdef _WIN32
#define FOPEN(...) _wfopenX(__VA_ARGS__)
#define FSEEK(...) _fseeki64(__VA_ARGS__)

#include <locale>
#include <codecvt>
#include <string>
#include <iostream>

inline FILE* _wfopenX(char const* _FileName, char const* _Mode) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring w_FileName = converter.from_bytes(_FileName);
    std::wstring w_Mode = converter.from_bytes(_Mode);
    FILE* file = _wfopen(w_FileName.c_str(), w_Mode.c_str());
    return file;
}

#else
#define FOPEN(...) fopen(__VA_ARGS__)
#define FSEEK(...) fseek(__VA_ARGS__)
#endif

#endif  // INCLUDE_CHIA_STDIOX_HPP_
