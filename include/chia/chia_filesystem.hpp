// Copyright 2020 Chia Network Inc

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef SRC_CPP_CHIA_FILESYSTEM_HPP_
#define SRC_CPP_CHIA_FILESYSTEM_HPP_

#ifdef __APPLE__
// std::filesystem is not supported on Mojave
#include "filesystem.hpp"
namespace fs = ghc::filesystem;
#else
#ifdef __has_include
#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif
#endif

#endif // SRC_CPP_CHIA_FILESYSTEM_HPP_
