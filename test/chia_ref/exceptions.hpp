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

#ifndef SRC_CPP_EXCEPTIONS_HPP_
#define SRC_CPP_EXCEPTIONS_HPP_

#include <string>

struct InsufficientMemoryException : public std::exception {
    std::string s;
    InsufficientMemoryException(std::string ss) : s(ss) {}
    ~InsufficientMemoryException() throw() {}  // Updated
    const char* what() const throw() { return s.c_str(); }
};

struct InvalidStateException : public std::exception {
    std::string s;
    InvalidStateException(std::string ss) : s(ss) {}
    ~InvalidStateException() throw() {}  // Updated
    const char* what() const throw() { return s.c_str(); }
};

struct InvalidValueException : public std::exception {
    std::string s;
    InvalidValueException(std::string ss) : s(ss) {}
    ~InvalidValueException() throw() {}  // Updated
    const char* what() const throw() { return s.c_str(); }
};

#endif  // SRC_CPP_EXCEPTIONS_HPP
