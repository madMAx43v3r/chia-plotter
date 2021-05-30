// Copyright 2018 Chia Network Inc

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef SRC_CPP_ENCODING_HPP_
#define SRC_CPP_ENCODING_HPP_

#include <cmath>
#include <map>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include <FSE/lib/fse.h>
#include <FSE/lib/hist.h>
#include <FSE/lib/error_public.h>

#include "bits.hpp"
#include "exceptions.hpp"
#include "util.hpp"

#include <mutex>

class TMemoCache {
public:
    ~TMemoCache() 
    {
        // Clean up global entries on destruction
        std::map<double, FSE_CTable *>::iterator itc;
        for (itc = CT_MEMO.begin(); itc != CT_MEMO.end(); itc++) {
            FSE_freeCTable(itc->second);
        }
        std::map<double, FSE_DTable *>::iterator itd;
        for (itd = DT_MEMO.begin(); itd != DT_MEMO.end(); itd++) {
            FSE_freeDTable(itd->second);
        }
    }

    bool CTExists(double R)
    {
        std::lock_guard<std::mutex> l(memoMutex);
        return (CT_MEMO.find(R) != CT_MEMO.end());
    }

    bool DTExists(double R)
    {
        std::lock_guard<std::mutex> l(memoMutex);
        return (DT_MEMO.find(R) != DT_MEMO.end());
    }   

    void CTAssign(double R, FSE_CTable *ct)
    {
        std::lock_guard<std::mutex> l(memoMutex);
        CT_MEMO[R] = ct;
    }

    void DTAssign(double R, FSE_DTable *dt)
    {
        std::lock_guard<std::mutex> l(memoMutex);
        DT_MEMO[R] = dt;
    }

    FSE_CTable *CTGet(double R)
    {
        std::lock_guard<std::mutex> l(memoMutex);
        return CT_MEMO[R];
    }

    FSE_DTable *DTGet(double R)
    {
        std::lock_guard<std::mutex> l(memoMutex);
        return DT_MEMO[R];
    }

private:
    mutable std::mutex memoMutex; // Mutex to ensure map thread safety
    std::map<double, FSE_CTable *> CT_MEMO;
    std::map<double, FSE_DTable *> DT_MEMO;
};

TMemoCache tmCache;

class Encoding {
public:
    // Calculates x * (x-1) / 2. Division is done before multiplication.
    static uint128_t GetXEnc(uint64_t x)
    {
        uint64_t a = x, b = x - 1;

        if (a % 2 == 0)
            a /= 2;
        else
            b /= 2;

        return (uint128_t)a * b;
    }

    // Encodes two max k bit values into one max 2k bit value. This can be thought of
    // mapping points in a two dimensional space into a one dimensional space. The benefits
    // of this are that we can store these line points efficiently, by sorting them, and only
    // storing the differences between them. Representing numbers as pairs in two
    // dimensions limits the compression strategies that can be used.
    // The x and y here represent table positions in previous tables.
    static uint128_t SquareToLinePoint(uint64_t x, uint64_t y)
    {
        // Always makes y < x, which maps the random x, y  points from a square into a
        // triangle. This means less data is needed to represent y, since we know it's less
        // than x.
        if (y > x) {
            std::swap(x, y);
        }

        return GetXEnc(x) + y;
    }

    // Does the opposite as the above function, deterministicaly mapping a one dimensional
    // line point into a 2d pair. However, we do not recover the original ordering here.
    static std::pair<uint64_t, uint64_t> LinePointToSquare(uint128_t index)
    {
        // Performs a square root, without the use of doubles, to use the precision of the
        // uint128_t.
        uint64_t x = 0;
        for (int8_t i = 63; i >= 0; i--) {
            uint64_t new_x = x + ((uint64_t)1 << i);
            if (GetXEnc(new_x) <= index)
                x = new_x;
        }
        return std::pair<uint64_t, uint64_t>(x, index - GetXEnc(x));
    }

    static std::vector<short> CreateNormalizedCount(double R)
    {
        std::vector<double> dpdf;
        int N = 0;
        double E = 2.718281828459;
        double MIN_PRB_THRESHOLD = 1e-50;
        int TOTAL_QUANTA = 1 << 14;
        double p = 1 - pow((E - 1) / E, 1.0 / R);

        while (p > MIN_PRB_THRESHOLD && N < 255) {
            dpdf.push_back(p);
            N++;
            p = (pow(E, 1.0 / R) - 1) * pow(E - 1, 1.0 / R);
            p /= pow(E, ((N + 1) / R));
        }

        std::vector<short> ans(N, 1);
        auto cmp = [&dpdf, &ans](int i, int j) {
            return dpdf[i] * (log2(ans[i] + 1) - log2(ans[i])) <
                   dpdf[j] * (log2(ans[j] + 1) - log2(ans[j]));
        };

        std::priority_queue<int, std::vector<int>, decltype(cmp)> pq(cmp);
        for (int i = 0; i < N; ++i) pq.push(i);

        for (int todo = 0; todo < TOTAL_QUANTA - N; ++todo) {
            int i = pq.top();
            pq.pop();
            ans[i]++;
            pq.push(i);
        }

        for (int i = 0; i < N; ++i) {
            if (ans[i] == 1) {
                ans[i] = (short)-1;
            }
        }
        return ans;
    }

    static size_t ANSEncodeDeltas(std::vector<unsigned char> deltas, double R, uint8_t *out)
    {
        if (!tmCache.CTExists(R)) {
            std::vector<short> nCount = Encoding::CreateNormalizedCount(R);
            unsigned maxSymbolValue = nCount.size() - 1;
            unsigned tableLog = 14;

            if (maxSymbolValue > 255)
                throw std::invalid_argument("maxSymbolValue > 255");
            FSE_CTable *ct = FSE_createCTable(maxSymbolValue, tableLog);
            size_t err = FSE_buildCTable(ct, nCount.data(), maxSymbolValue, tableLog);
            if (FSE_isError(err)) {
                throw InvalidStateException(FSE_getErrorName(err));
            }
            tmCache.CTAssign(R, ct);
        }

        FSE_CTable *ct = tmCache.CTGet(R);
        return FSE_compress_usingCTable(
            out, deltas.size() * 8, static_cast<void *>(deltas.data()), deltas.size(), ct);
    }

    static void ANSFree(double R)
    {
        // Cache all entries, only free on close
    }

    static std::vector<uint8_t> ANSDecodeDeltas(
        const uint8_t *inp,
        size_t inp_size,
        int numDeltas,
        double R)
    {
        if (!tmCache.DTExists(R)) {
            std::vector<short> nCount = Encoding::CreateNormalizedCount(R);
            unsigned maxSymbolValue = nCount.size() - 1;
            unsigned tableLog = 14;

            FSE_DTable *dt = FSE_createDTable(tableLog);
            size_t err = FSE_buildDTable(dt, nCount.data(), maxSymbolValue, tableLog);
            if (FSE_isError(err)) {
                throw InvalidStateException(FSE_getErrorName(err));
            }
            tmCache.DTAssign(R, dt);
        }

        FSE_DTable *dt = tmCache.DTGet(R);

        std::vector<uint8_t> deltas(numDeltas);
        size_t err = FSE_decompress_usingDTable(&deltas[0], numDeltas, inp, inp_size, dt);

        if (FSE_isError(err)) {
            throw InvalidStateException(FSE_getErrorName(err));
        }

        for (uint32_t i = 0; i < deltas.size(); i++) {
            if (deltas[i] == 0xff) {
                throw InvalidStateException("Bad delta detected");
            }
        }
        return deltas;
    }
};

#endif  // SRC_CPP_ENCODING_HPP_
