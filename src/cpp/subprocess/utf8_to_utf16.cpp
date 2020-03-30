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

    size_t strlen16(char16_t* str) {
        size_t size = 0;
        for (; *str; ++str)
            ++size;
        return size;
    }
}