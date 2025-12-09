#include "utf8_to_utf16.hpp"

#include <locale>
#include <codecvt>

#ifdef _WIN32
#include <windows.h>
#endif

#include <assert.h>

namespace subprocess {

#ifdef _WIN32
    // these 3 functions are addapted from reproc
    // https://github.com/DaanDeMeyer/reproc/blob/master/reproc/src/utf.windows.c

    // we only need these on windows
    std::u16string utf8_to_utf16(const std::string& string) {
        static_assert(sizeof(wchar_t) == 2, "wchar_t must be of size 2");
        static_assert(sizeof(wchar_t) == sizeof(char16_t), "wchar_t must be of size 2");
        int size = (int)string.size()+1;
        // Determine wstring size (`MultiByteToWideChar` returns the required size if
        // its last two arguments are `NULL` and 0).
        int r = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
            string.c_str(), size, NULL, 0);
        if (r == 0) {
            return {};
        }
        assert(r > 0);

        // `MultiByteToWideChar` does not return negative values so the cast to
        // `size_t` is safe.
        wchar_t *wstring = new wchar_t[r];
        if (wstring == NULL) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return {};
        }

        // Now we pass our allocated string and its size as the last two arguments
        // instead of `NULL` and 0 which makes `MultiByteToWideChar` actually perform
        // the conversion.
        r = MultiByteToWideChar(CP_UTF8, 0, string.c_str(), size,
            wstring, r);
        if (r == 0) {
            delete [] wstring;
            return {};
        }
        std::u16string result((char16_t*)wstring, r-1);
        delete [] wstring;
        return result;
    }

#ifndef WC_ERR_INVALID_CHARS
    // mingw doesn't define this
    constexpr int WC_ERR_INVALID_CHARS = 0;
#endif

    std::string utf16_to_utf8(const std::u16string& wstring) {
        int size = (int)wstring.size()+1;
        // Determine wstring size (`MultiByteToWideChar` returns the required size if
        // its last two arguments are `NULL` and 0).
        int r = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
            (wchar_t*)wstring.c_str(), size, NULL, 0, NULL, NULL);

        if (r == 0) {
            return {};
        }
        assert(r > 0);

        // `WideCharToMultiByte` does not return negative values so the cast to
        // `size_t` is safe.
        char *string = new char[r];
        if (string == nullptr) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return {};
        }

        // Now we pass our allocated string and its size as the last two arguments
        // instead of `NULL` and 0 which makes `MultiByteToWideChar` actually perform
        // the conversion.
        r = WideCharToMultiByte(CP_UTF8, 0, (wchar_t*)wstring.c_str(), size,
            string, r, NULL, NULL);
        if (r == 0) {
            delete [] string;
            return {};
        }
        std::string result(string, r-1);
        delete [] string;
        return result;
    }

    std::string utf16_to_utf8(const std::wstring& wstring) {
        int size = (int)wstring.size()+1;
        // Determine wstring size (`MultiByteToWideChar` returns the required size if
        // its last two arguments are `NULL` and 0).
        int r = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
            (wchar_t*)wstring.c_str(), size, NULL, 0, NULL, NULL);

        if (r == 0) {
            return {};
        }
        assert(r > 0);

        // `WideCharToMultiByte` does not return negative values so the cast to
        // `size_t` is safe.
        char *string = new char[r];
        if (string == nullptr) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return {};
        }

        // Now we pass our allocated string and its size as the last two arguments
        // instead of `NULL` and 0 which makes `MultiByteToWideChar` actually perform
        // the conversion.
        r = WideCharToMultiByte(CP_UTF8, 0, (wchar_t*)wstring.c_str(), size,
            string, r, NULL, NULL);
        if (r == 0) {
            delete [] string;
            return {};
        }
        std::string result(string, r-1);
        delete [] string;
        return result;
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

    std::string lptstr_to_string(LPTSTR str) {
        return lptstr_to_string_t(str);
    }
#endif
}