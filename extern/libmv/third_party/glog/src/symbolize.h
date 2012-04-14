// Copyright (c) 2006, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Author: Satoru Takabayashi
//
// This library provides Symbolize() function that symbolizes program
// counters to their corresponding symbol names on linux platforms.
// This library has a minimal implementation of an ELF symbol table
// reader (i.e. it doesn't depend on libelf, etc.).
//
// The algorithm used in Symbolize() is as follows.
//
//   1. Go through a list of maps in /proc/self/maps and find the map
//   containing the program counter.
//
//   2. Open the mapped file and find a regular symbol table inside.
//   Iterate over symbols in the symbol table and look for the symbol
//   containing the program counter.  If such a symbol is found,
//   obtain the symbol name, and demangle the symbol if possible.
//   If the symbol isn't found in the regular symbol table (binary is
//   stripped), try the same thing with a dynamic symbol table.
//
// Note that Symbolize() is originally implemented to be used in
// FailureSignalHandler() in base/google.cc.  Hence it doesn't use
// malloc() and other unsafe operations.  It should be both
// thread-safe and async-signal-safe.

#ifndef BASE_SYMBOLIZE_H_
#define BASE_SYMBOLIZE_H_

#include "utilities.h"
#include "config.h"
#include <glog/logging.h>

#ifdef HAVE_SYMBOLIZE

#if defined(__ELF__)  // defined by gcc on Linux
#include <elf.h>
#include <link.h>  // For ElfW() macro.

// If there is no ElfW macro, let's define it by ourself.
#ifndef ElfW
# if SIZEOF_VOID_P == 4
#  define ElfW(type) Elf32_##type
# elif SIZEOF_VOID_P == 8
#  define ElfW(type) Elf64_##type
# else
#  error "Unknown sizeof(void *)"
# endif
#endif

_START_GOOGLE_NAMESPACE_

// Gets the section header for the given name, if it exists. Returns true on
// success. Otherwise, returns false.
bool GetSectionHeaderByName(int fd, const char *name, size_t name_len,
                            ElfW(Shdr) *out);

_END_GOOGLE_NAMESPACE_

#endif  /* __ELF__ */

_START_GOOGLE_NAMESPACE_

// Installs a callback function, which will be called right before a symbol name
// is printed. The callback is intended to be used for showing a file name and a
// line number preceding a symbol name.
// "fd" is a file descriptor of the object file containing the program
// counter "pc". The callback function should write output to "out"
// and return the size of the output written. On error, the callback
// function should return -1.
typedef int (*SymbolizeCallback)(int fd, void *pc, char *out, size_t out_size,
                                 uint64 relocation);
void InstallSymbolizeCallback(SymbolizeCallback callback);

_END_GOOGLE_NAMESPACE_

#endif

_START_GOOGLE_NAMESPACE_

// Symbolizes a program counter.  On success, returns true and write the
// symbol name to "out".  The symbol name is demangled if possible
// (supports symbols generated by GCC 3.x or newer).  Otherwise,
// returns false.
bool Symbolize(void *pc, char *out, int out_size);

_END_GOOGLE_NAMESPACE_

#endif  // BASE_SYMBOLIZE_H_
