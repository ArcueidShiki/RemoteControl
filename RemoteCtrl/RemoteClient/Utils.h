#pragma once
#include <string>
class CUtils
{
public:
    static void Dump(BYTE* pData, size_t nSize);
    static int Bytes2Image(CImage& img, const std::string& strBuf);
};

