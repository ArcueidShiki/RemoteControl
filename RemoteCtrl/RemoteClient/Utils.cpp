#include "pch.h"
#include "Utils.h"
#include <atlimage.h>

void CUtils::Dump(BYTE* pData, size_t nSize)
{
    std::string strOut;
    for (size_t i = 0; i < nSize; i++)
    {
        char buf[8] = "";
        if (i > 0 && (i % 16 == 0)) strOut += '\n';
        snprintf(buf, sizeof(buf), "%02X ", pData[i]);
        strOut += buf;
    }
    strOut += '\n';
    OutputDebugStringA(strOut.c_str());
}

int CUtils::Bytes2Image(CImage& img, const std::string& strBuf)
{
	BYTE* pData = (BYTE*)strBuf.c_str();
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
	if (hMem == NULL)
	{
		//TRACE("Insufficient Memory GlobalAlloc failed\n");
		Sleep(1);
		return -1;
	}
	IStream* pStream = NULL;
	HRESULT hRet = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
	if (hRet == S_OK)
	{
		ULONG written = 0;
		ULONG len = (ULONG)strBuf.size();
		pStream->Write(pData, len, &written);
		LARGE_INTEGER li = { 0 };
		pStream->Seek(li, STREAM_SEEK_SET, NULL);
		if ((HBITMAP)img != NULL) img.Destroy();
		img.Load(pStream);
	}
	return hRet;
}
