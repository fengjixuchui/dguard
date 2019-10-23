//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#include "Grader.h"

//
// Disabled because we advisedly converting data from ULONG to UCHAR.
//
#pragma warning( disable : 4244 ) 

namespace encryption
{
	ulong Grader::f(ulong _L, ulong _key)
	{
		Data4Bytes val;
		ulong modLeft = _L ^ _key;
		Data4Bytes* ptr32Data = reinterpret_cast<Data4Bytes*>(&modLeft);

		val.l0 = ptr32Data->l0 ^ ptr32Data->l3;
		val.l1 = ptr32Data->l2 ^ ptr32Data->l3;
		val.l2 = ptr32Data->l1 ^ ptr32Data->l3;
		val.l3 = ptr32Data->l2 ^ ptr32Data->l3;

		return val.data_32;
	}

	ulong Grader::f(BYTE _1char, ulong _key)
	{
		Data4Bytes val;
		ulong modLeft = _1char ^ _key;
		Data4Bytes* ptr32Data = reinterpret_cast<Data4Bytes*>(&modLeft);

		val.l0 = ptr32Data->l0 ^ ptr32Data->l3;
		val.l1 = ptr32Data->l2 ^ ptr32Data->l3;
		val.l2 = ptr32Data->l1 ^ ptr32Data->l3;
		val.l3 = ptr32Data->l2 ^ ptr32Data->l3;

		return val.data_32;
	}

	ulong Grader::getKey(uint64_t _key, int _i)
	{
		PData8Bytes keyBuffer = reinterpret_cast<PData8Bytes>(&_key);
		int n = (_i * 2) % (SizeOfEncryptionKey);

		_key = (_key << n) | (_key >> (SizeOfEncryptionKey - n));
		return keyBuffer->low.data_32;
	}

	void Grader::encrypt4b(ulong* _left, ulong* _right, uint64_t _key, int _rounds)
	{
		for (int i = 0; i < _rounds; i++)
		{
			ulong temp = *_right ^ f(*_left, getKey(_key, i));
			*_right = *_left;
			*_left = temp;
		}
	}

	void Grader::encrypt1b(BYTE* _left, BYTE* _right, uint64_t _key, int _rounds)
	{
		for (int i = 0; i < _rounds; i++)
		{
			BYTE temp = *_right ^ f(*_left, Grader::getKey(_key, i));
			*_right = *_left;
			*_left = temp;
		}
	}

	void Grader::decrypt(ulong* _left, ulong* _right, uint64_t _key, int _rounds)
	{
		for (int i = _rounds - 1; i >= 0; i--)
		{
			ulong temp = *_left ^ f(*_right, getKey(_key, i));
			*_left = *_right;
			*_right = temp;
		}
	}

	void Grader::decrypt1b(BYTE* _left, BYTE* _right, uint64_t _key, int _rounds)
	{
		for (int i = _rounds - 1; i >= 0; i--)
		{
			BYTE temp = *_left ^ f(*_right, getKey(_key, i));
			*_left = *_right;
			*_right = temp;
		}
	}

	void Grader::encryptData(char* _pBuffer, std::size_t _length, uint64_t _key, int _rounds)
	{
		const char* pEndOfBuffer = _pBuffer + _length;

		//
		// Обработать первые байты кодируемых данных, в случае если его размер не кратен восьми.
		//
		int residueKey = _length % sizeof(_key);
		if (residueKey)
		{
			char* tmpBufPtr = _pBuffer;
			int residue2 = residueKey % 2;
			if (residue2)
			{
				Data8Bytes* p64Key = reinterpret_cast<Data8Bytes*>(&_key);
				BYTE xorKeyFoFirstByte = p64Key->low.l0 ^ p64Key->low.l1 ^ p64Key->low.l2 ^ _rounds;
				*tmpBufPtr = *tmpBufPtr ^ xorKeyFoFirstByte;
				tmpBufPtr++;
			}

			for (const char* pBorder = tmpBufPtr + residueKey - residue2; tmpBufPtr < pBorder; tmpBufPtr += 2)
			{
				encrypt1b((BYTE*)tmpBufPtr, ((BYTE*)(tmpBufPtr + sizeof(BYTE))), _key, _rounds);
			}
		}

		for (_pBuffer += residueKey; _pBuffer < pEndOfBuffer; _pBuffer += sizeof(ulong) * 2)
		{
			encrypt4b((ulong*)_pBuffer, ((ulong*)(_pBuffer + sizeof(ulong))), _key, _rounds);
		}
	}

	void Grader::decryptData(char* _pBuffer, std::size_t _length, uint64_t _key, int _rounds)
	{
		const char* pEndOfBuffer = _pBuffer + _length;
		int residueKey = _length % sizeof(_key);
		if (residueKey)
		{
			char* tmpBufPtr = _pBuffer;
			int residue2 = residueKey % 2;
			if (residue2)
			{
				Data8Bytes* p64Key = reinterpret_cast<Data8Bytes*>(&_key);
				BYTE xorKey = p64Key->low.l0 ^ p64Key->low.l1 ^ p64Key->low.l2 ^ _rounds;
				*tmpBufPtr = *tmpBufPtr ^ xorKey;
				tmpBufPtr++;
			}

			for (const char* pBorder = tmpBufPtr + residueKey - residue2; tmpBufPtr < pBorder; tmpBufPtr += 2)
			{
				decrypt1b((BYTE*)tmpBufPtr, ((BYTE*)(tmpBufPtr + sizeof(BYTE))), _key, _rounds);
			}
		}

		for (_pBuffer += residueKey; _pBuffer < pEndOfBuffer; _pBuffer += sizeof(ulong) * 2)
		{
			decrypt((ulong*)_pBuffer, ((ulong*)(_pBuffer + sizeof(ulong))), _key, _rounds);
		}
	}
}
