#include <string>


namespace subprocess {
    std::u16string utf8_to_utf16(const std::string& str);
    std::string utf16_to_utf8(const std::u16string& str);
    size_t strlen16(char16_t* str);
}