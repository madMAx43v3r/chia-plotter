
#ifndef INCLUDE_CHIA_STDIOX_HPP_
#define INCLUDE_CHIA_STDIOX_HPP_
#include <stdio.h>

#ifdef _WIN32
#define FOPEN(...) stdiox::fopen(__VA_ARGS__)
#define FSEEK(...) _fseeki64(__VA_ARGS__)

#include <locale>
#include <codecvt>
#include <string>

namespace stdiox {
    inline FILE* fopen(char const* _FileName, char const* _Mode) {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        std::wstring W_FileName = converter.from_bytes(_FileName);
        std::wstring W_Mode = converter.from_bytes(_Mode);
        return _wfopen(W_FileName.c_str(), W_Mode.c_str());
    }
}


#else
#define FOPEN(...) fopen(__VA_ARGS__)
#define FSEEK(...) fseek(__VA_ARGS__)
#endif


#endif  // INCLUDE_CHIA_STDIOX_HPP_