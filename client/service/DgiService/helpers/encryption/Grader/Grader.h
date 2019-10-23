//
//
//	Author: 
//			burluckij@gmail.com
//		(c) Burlutsky Stanislav 2006 - 2015
//

#pragma once

#include <iostream>
#include <stdint.h>
#include <Windows.h>

namespace encryption
{
	typedef unsigned long ulong;
	typedef unsigned char uchar;
	static const int SizeOfEncryptionKey = 64;

	typedef struct Data4Bytes
	{
		union
		{
			ulong data_32;

			struct
			{
				uchar l0 : 8;
				uchar l1 : 8;
				uchar l2 : 8;
				uchar l3 : 8;
			};
		};
	} Data4Bytes, *PData4Bytes;

	typedef struct Data8Bytes
	{
		union
		{
			char buf[8];
			uint64_t data64;

			struct
			{
				Data4Bytes low;
				Data4Bytes high;
			};
		};
	} Data8Bytes, *PData8Bytes;

	inline Data4Bytes operator^(const Data4Bytes& rhs, const Data4Bytes& lhs)
	{
		Data4Bytes res = rhs;
		res.data_32 ^= lhs.data_32;
		return res;
	}

	inline Data4Bytes operator^(const Data4Bytes& rhs, ulong lhs)
	{
		Data4Bytes res;
		res.data_32 = rhs.data_32 ^ lhs;
		return res;
	}

	//
	// The class implements simple 64 bit encryption using Feistel network.
	//
	class Grader
	{
	public:

		// Encrypts two 32 bit values.
		static void encrypt4b(ulong* _left, ulong* _right, uint64_t _key, int _rounds = 10);
		// Works with two 8 bit values.
		static void encrypt1b(BYTE* _left, BYTE* _right, uint64_t _key, int _rounds = 10);

		// Decrypts two 32 bit values
		static void decrypt(ulong* _left, ulong* _right, uint64_t _key, int _rounds = 10);
		static void decrypt1b(BYTE* _left, BYTE* _right, uint64_t _key, int _rounds = 10);


		// Encrypts array of bytes
		// Note: length - count of bytes in _pBuffer, must be a multiple to one byte.
		// key - 64 bits 
		static void encryptData(char* _pBuffer, std::size_t _length, uint64_t _key, int _rounds = 10);

		// Decrypts array of bytes
		// Note: length - count of bytes in _pBuffer, must be a multiple to one byte.
		// key - 64 bits 
		static void decryptData(char* _pBuffer, std::size_t _length, uint64_t _key, int _rounds = 10);

	private:

		// Generating function
		static ulong f(ulong _L, ulong _key);
		static ulong f(BYTE _1char, ulong _key);

		// Generate 32 bit key from 64 bit key
		static ulong getKey(uint64_t _key, int _i);
	};
}
