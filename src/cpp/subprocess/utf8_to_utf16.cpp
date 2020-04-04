#include "utf8_to_utf16.hpp"

#include <locale>
#include <codecvt>

namespace subprocess {

    std::u16string utf8_to_utf16(const std::string& str) {
        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>,char16_t> convert;
        std::u16string dest = convert.from_bytes(str);
        return dest;
    }
    std::string utf16_to_utf8(const std::u16string& str) {
        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>,char16_t> convert;
        std::string dest = convert.to_bytes(str);
        return dest;
    }

    std::wstring utf8_to_utf16_w(const std::string& str) {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>,wchar_t> convert;
        std::wstring dest = convert.from_bytes(str);
        return dest;
    }

    std::string utf16_to_utf8(const std::wstring& str) {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>,wchar_t> convert;
        std::string dest = convert.to_bytes(str);
        return dest;
    }

    size_t strlen16(char16_t* str) {
        size_t size = 0;
        for (; *str; ++str)
            ++size;
        return size;
    }
    size_t strlen16(wchar_t* str) {
        size_t size = 0;
        for (; *str; ++str)
            ++size;
        return size;
    }

    template<typename T>
    std::string lptstr_to_string_t(T str) {
        if (str == nullptr)
            return "";
        if constexpr (sizeof(*str) == 1) {
            static_assert(sizeof(*str) == 1);
            return (const char*)str;
        } else {
            static_assert(sizeof(*str) == 2);
            return utf16_to_utf8((const wchar_t*)str);
        }
    }
#ifdef _WIN32
    std::string lptstr_to_string(LPTSTR str) {
        return lptstr_to_string_t(str);
    }
#endif
}