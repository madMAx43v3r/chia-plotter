
#ifndef INCLUDE_CHIA_STDIOX_HPP_
#define INCLUDE_CHIA_STDIOX_HPP_
#include <stdio.h>

#ifdef _WIN32
#include <locale>
#include <codecvt>
#include <string>
#endif

namespace stdiox {

    inline size_t fwrite(void const* _Buffer, size_t _ElementSize, size_t _ElementCount, FILE* _Stream) {
        return std::fwrite(_Buffer, _ElementSize, _ElementCount, _Stream);
    }

#ifdef _WIN32
    inline FILE* fopen(char const* _FileName, char const* _Mode) {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        std::wstring W_FileName = converter.from_bytes(_FileName);
        std::wstring W_Mode = converter.from_bytes(_Mode);
        return _wfopen(W_FileName.c_str(), W_Mode.c_str());
    }

    inline int fseek(FILE* _Stream, long _Offset, int _Origin) {
        return _fseeki64(_Stream, _Offset, _Origin);
    }

#else
    inline int fseek(FILE* _Stream, long _Offset, int _Origin) {
        return std::fseek(_Stream, _Offset, _Origin);
    }

    inline FILE* fopen(char const* _FileName, char const* _Mode) {
        return std::fopen(_FileName, _Mode);
    }
#endif

}

#endif  // INCLUDE_CHIA_STDIOX_HPP_