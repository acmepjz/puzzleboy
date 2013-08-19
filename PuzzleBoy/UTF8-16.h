#pragma once

#include <string>

typedef std::string u8string;
typedef std::basic_string<unsigned short> u16string;

u8string toUTF8(const u16string& src);
u16string toUTF16(const u8string& src);
