/*
 * Copyright (C) 2013-2016 Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "nuclear_bits/util/demangle.hpp"


// Windows symbol demangler
#ifdef _WIN32

#include "nuclear_bits/util/windows_includes.hpp"
#include <mutex>
#include <Dbghelp.h>

#pragma comment(lib, "Dbghelp.lib")

namespace NUClear {
    namespace util {

        bool symInitialised = false;
        std::mutex symbolMutex;

        void initSymbols() {

            HANDLE hProcess;

            SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);

            hProcess = GetCurrentProcess();

            if (!SymInitialize(hProcess, nullptr, true)) {
                // SymInitialize failed
                throw std::system_error(GetLastError(), std::system_category(), "SymInitialise failed");
            }

            symInitialised = true;
        }

        std::string demangle(const char* symbol) {
            std::lock_guard<std::mutex> lock(symbolMutex);

            // Initialise the symbols if we have to
            if (!symInitialised) {
                initSymbols();
            }

            char name[256];

            if(int len = UnDecorateSymbolName(symbol, name, sizeof(name), 0)) {
                return std::string(name, len);
            }
            else {
                return symbol;
            }
        }
    }
}

// GNU/Clang symbol demangler
#else

#include <cxxabi.h>
#include <cstdlib>
#include <memory>

namespace NUClear {
    namespace util {

        /**
         * @brief Demangles the passed symbol to a string, or returns it if it cannot demangle it
         *
         * @param symbol the symbol to demangle
         *
         * @return the demangled symbol, or the original string if it could not be demangeld
         */
        std::string demangle(const char* symbol) {

            int status = -4; // some arbitrary value to eliminate the compiler warning
            std::unique_ptr<char, void(*)(void*)> res {
                abi::__cxa_demangle(symbol, nullptr, nullptr, &status),
                std::free
            };

            return std::string(status == 0 ? res.get() : symbol);
        }
    }
}
#endif  // _MSC_VER
