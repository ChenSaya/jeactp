#ifndef _STRINGCONV_H_
#define _STRINGCONV_H_

#include <string>
#include <vector>
#include <fstream>
#include <iconv.h>
enum {
    CHAR_SET_ANSI, 
    CHAR_SET_UTF8
};
typedef long long __int64;

namespace jealib
{

class StringConvert
{
public:
    static std::string AnsiToUtf8(const std::string& str, const std::string& strDefault = "");
    static std::string AnsiToUtf8(const char* pcStr, const std::string& strDefault = "");
    static std::string Utf8ToAnsi(const std::string& str, const std::string& strDefault = "");
    static std::string Convert(const std::string& str, unsigned int CodeFrom, unsigned int CodeTo, const std::string& strDefault);
    static std::string GetCharSetName(unsigned int code);
};

};
#endif

