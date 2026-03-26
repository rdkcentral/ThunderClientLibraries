/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 Metrological
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

namespace Thunder {
namespace Compositor {
    class TerminalInput {
    public:
        typedef void (*InputHandler)(char);

        TerminalInput()
            : _original()
            , _originalFlags(0)
            , _valid(SetupTerminal())
        {
        }

        ~TerminalInput()
        {
            if (_valid) {
                RestoreTerminal();
            }
        }

        TerminalInput(const TerminalInput&) = delete;
        TerminalInput& operator=(const TerminalInput&) = delete;
        TerminalInput(TerminalInput&&) = delete;
        TerminalInput& operator=(TerminalInput&&) = delete;

        bool IsValid() const { return _valid; }

        char Read()
        {
            char c = 0;
PUSH_WARNING(DISABLE_WARNING_UNUSED_RESULT)
            read(STDIN_FILENO, &c, 1);
POP_WARNING()
            return c;
        }

    private:
        bool SetupTerminal()
        {
            if (tcgetattr(STDIN_FILENO, &_original) != 0) {
                return false;
            }

            _originalFlags = fcntl(STDIN_FILENO, F_GETFL, 0);
            if (_originalFlags == -1) {
                return false;
            }

            struct termios raw = _original;

            raw.c_lflag &= ~(ICANON | ECHO); // No line buffering, no echo
            raw.c_cc[VMIN] = 0; // Non-blocking read
            raw.c_cc[VTIME] = 0; // No timeout

            if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) != 0) {
                return false;
            }

            if (fcntl(STDIN_FILENO, F_SETFL, _originalFlags | O_NONBLOCK) == -1) {
                tcsetattr(STDIN_FILENO, TCSAFLUSH, &_original);
                return false;
            }

            return true;
        }

        void RestoreTerminal()
        {
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &_original);
            fcntl(STDIN_FILENO, F_SETFL, _originalFlags);
        }

    private:
        struct termios _original;
        int _originalFlags;
        bool _valid;
    };
} // namespace Compositor
} // namespace Thunder