#include "stringconv.h"
#include <string.h>

namespace jealib
{

std::string StringConvert::AnsiToUtf8(const std::string& str, const std::string& strDefault)
{
   return Convert(str, CHAR_SET_ANSI, CHAR_SET_UTF8, strDefault);
}

std::string StringConvert::AnsiToUtf8(const char* pcStr, const std::string& strDefault)
{
   std::string str(pcStr);
   return Convert(str, CHAR_SET_ANSI, CHAR_SET_UTF8, strDefault);
}

std::string StringConvert::Utf8ToAnsi(const std::string& str, const std::string& strDefault)
{
    return Convert(str, CHAR_SET_UTF8, CHAR_SET_ANSI, strDefault);
}

std::string StringConvert::Convert(const std::string& str, unsigned int CodeFrom, unsigned int CodeTo, const std::string& strDefault = "")
{
    std::string strCodeFrom = GetCharSetName(CodeFrom);
    if (strCodeFrom == "") return "";
    std::string strCodeTo = GetCharSetName(CodeTo);
    if (strCodeTo == "") return "";
    iconv_t cd = iconv_open(strCodeTo.c_str(), strCodeFrom.c_str());
    if (cd == 0) return "";
    std::vector<char> oBuf(str.length() * 6 + 1);
    memset(&oBuf[0], 0, oBuf.size());
    const char * pstr = str.c_str();
    char * pdst = &oBuf[0];
    size_t iSrcLen = str.length();
    size_t iDstLen = oBuf.size();
    if ((size_t)(-1) != iconv(cd, (char **)&pstr, &iSrcLen, &pdst, &iDstLen)) 
    {
        iconv_close(cd);
        return &oBuf[0];
    }
    iconv_close(cd);
    return strDefault;
}

std::string StringConvert::GetCharSetName(unsigned int code)
{
    switch (code) 
    {
    case CHAR_SET_ANSI:
        return "gb2312";
    case CHAR_SET_UTF8:
        return "utf-8";
    default:
        return "";
    }
}

};

