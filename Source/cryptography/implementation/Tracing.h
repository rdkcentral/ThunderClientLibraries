#pragma once

#include "../Module.h"

namespace Implementation {
namespace Trace {
    class ImplementationInfo {
    public:
        ImplementationInfo(const ImplementationInfo& a_Copy) = delete;
        ImplementationInfo& operator=(const ImplementationInfo& a_RHS) = delete;

        ImplementationInfo(const TCHAR formatter[], ...)
        {
            va_list ap;
            va_start(ap, formatter);
            WPEFramework::Trace::Format(_text, formatter, ap);
            va_end(ap);
        }
        explicit ImplementationInfo(const string& text)
            : _text(WPEFramework::Core::ToString(text))
        {
        }
        ~ImplementationInfo()
        {
        }

    public:
        inline const char* Data() const
        {
            return (_text.c_str());
        }
        inline uint16_t Length() const
        {
            return (static_cast<uint16_t>(_text.length()));
        }

    private:
        std::string _text;
    };

    class ImplementationError {
    public:
        ImplementationError(const ImplementationError& a_Copy) = delete;
        ImplementationError& operator=(const ImplementationError& a_RHS) = delete;

        ImplementationError(const TCHAR formatter[], ...)
        {
            va_list ap;
            va_start(ap, formatter);
            WPEFramework::Trace::Format(_text, formatter, ap);
            va_end(ap);
        }
        explicit ImplementationError(const string& text)
            : _text(WPEFramework::Core::ToString(text))
        {
        }
        ~ImplementationError()
        {
        }

    public:
        inline const char* Data() const
        {
            return (_text.c_str());
        }
        inline uint16_t Length() const
        {
            return (static_cast<uint16_t>(_text.length()));
        }

    private:
        std::string _text;
    };
} // namespace WPEFramework
} // namespace Trace