#include "pch.h"
#include "Packet.h"

CPacket::CPacket()
	: sHead(0)
	, nLength(0)
	, sCmd(0)
	, sSum(0)
{
}

CPacket::CPacket(const BYTE* pData, size_t& nSize)
{
	if (!pData)
	{
		TRACE("pData is NULL");
		return;
	}
	// head
	size_t i = 0;
	for (; i < nSize; i++)
	{
		if (*(WORD*)(pData + i) == PACKET_HEAD)
		{
			sHead += PACKET_HEAD;
			i += sizeof(sHead); // skip head
			break;
		}
	}

	// length			length + cmd + sum
	if (i + sizeof(nLength) + sizeof(sCmd) + sizeof(sSum) > nSize)
	{
		nSize = 0;
		return; // parse packet error.
	}
	nLength = *(DWORD*)(pData + i);
	i += sizeof(nLength);

	// cmd
	if (nLength + i > nSize)
	{
		// message is not received completely, its larger than the packet
		nSize = 0;
		return;
	}
	sCmd = *(WORD*)(pData + i);
	i += sizeof(sCmd);

	// str data
	DWORD dataLen = nLength - sizeof(sCmd) - sizeof(sSum);
	strData.resize(dataLen);
	memcpy((void*)strData.c_str(), pData + i, dataLen);
	i += dataLen;

	// check sum
	sSum = *(WORD*)(pData + i);
	WORD sum = 0;
	for (size_t j = 0; j < strData.size(); j++)
	{
		sum += BYTE(strData[j]); // &0xFF
	}
	if (sum != sSum)
	{
		nSize = 0;
		return;
	}
	i += sizeof(sSum);
	// parse packet success.
	nSize = i; // head length data
}

CPacket::CPacket(const CPacket& other)
{
	sHead = other.sHead;
	nLength = other.nLength;
	sCmd = other.sCmd;
	strData = other.strData;
	sSum = other.sSum;
}

CPacket& CPacket::operator=(const CPacket& other)
{
	if (this != &other)
	{
		sHead = other.sHead;
		nLength = other.nLength;
		sCmd = other.sCmd;
		strData = other.strData;
		sSum = other.sSum;
	}
	return *this;
}

CPacket::CPacket(DWORD nCmd, const BYTE* pData, size_t nSize)
{
	sHead = PACKET_HEAD;
	nLength = nSize + sizeof(sCmd) + sizeof(sSum);
	sCmd = nCmd;
	if (nSize > 0 && pData)
	{
		strData.resize(nSize);
		memcpy((void*)strData.c_str(), pData, nSize);
	}
	else
	{
		strData.clear();
	}
	sSum = 0;
	for (size_t i = 0; i < strData.size(); i++)
	{
		sSum += BYTE(strData[i]);
	}
}

size_t CPacket::Size() const
{
	return sizeof(sHead) + nLength + sizeof(sCmd) + sizeof(sSum);
}

/**
Only for dump packet data
*/
const char* CPacket::Data()
{
	strOut.resize(Size());
	BYTE* pData = (BYTE*)strOut.c_str();
	*(WORD*)pData = sHead;
	pData += sizeof(sHead);
	*(DWORD*)(pData) = nLength;
	pData += sizeof(nLength);
	*(WORD*)(pData) = sCmd;
	pData += sizeof(sCmd);
	memcpy(pData, strData.c_str(), strData.size());
	pData += strData.size();
	*(WORD*)pData = sSum;
	return strOut.c_str();
}
