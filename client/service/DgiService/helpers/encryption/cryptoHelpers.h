//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#pragma once

#include <memory>
#include <string>

namespace crypto
{
	struct KeyPair
	{
		std::string publicKey;
		std::string privateKey;
	};

	//
	// Calculates hashes with Crypt++ library.
	//

	// Returns calculated hash in ascii symbols presentation.
	std::string getMd5(const unsigned char* _pBuffer, size_t _sizeBuffer);

	// Returns bytes hash which is not for string representation - raw bytes sequence.
	std::string getMd5Raw(const unsigned char* _pBuffer, size_t _sizeBuffer);

	std::string getCrc32(const unsigned char* _pBuffer, size_t _sizeBuffer);
	std::string getCrc32Raw(const unsigned char* _pBuffer, size_t _sizeBuffer);
	std::string getSha1(const unsigned char* _pBuffer, size_t _sizeBuffer);
	std::string getSha1Raw(const unsigned char* _pBuffer, size_t _sizeBuffer);
	std::string getSha256(const unsigned char* _pBuffer, size_t _sizeBuffer);
	std::string getSha256Raw(const unsigned char* _pBuffer, size_t _sizeBuffer);
	std::string getSha512(const unsigned char* _pBuffer, size_t _sizeBuffer);
	std::string getSha512Raw(const unsigned char* _pBuffer, size_t _sizeBuffer);
	std::string getWhirepool(const unsigned char* _pBuffer, size_t _sizeBuffer);
	std::string getWhirepoolRaw(const unsigned char* _pBuffer, size_t _sizeBuffer);


	//
	// All about RSA cryptography.
	//
	namespace rsa
	{
		//
		// Generates RSA key pair of private and public keys.
		//
		bool genKeys(::crypto::KeyPair& _keys, unsigned long _keySize);

		//
		// Signs data with PSSR schema.
		//
		std::string signPssrSha1(const std::string &_privateKeyBin, const std::string &_message, bool _putTheMessage = false);
		std::string signPssrSha256(const std::string &_privateKeyBin, const std::string &_message, bool _putTheMessage = false);
		std::string signPssrSha512(const std::string &_privateKeyBin, const std::string &_message, bool _putTheMessage = false);
		std::string signPssrWhirepool(const std::string &_privateKeyBin, const std::string &_message, bool _putTheMessage = false);

		//
		// Verify signature by PSSR-signing schema.
		//
		// In functions which have not '_message' argument a '_signature' should already have embedded message body.
		//
		bool verifyPssrGost(const std::string &_publicKey, const std::string &_signature, std::string &_recovered);
		bool verifyPssrGostNoMsg(const std::string &_publicKey, const std::string &_message, const std::string &_signature);
		bool verifyPssrSha1(const std::string &_publicKey, const std::string &_signature, std::string &_recovered);
		bool verifyPssrSha1NoMsg(const std::string &_publicKey, const std::string &_message, const std::string &_signature);
		bool verifyPssrSha256(const std::string &_publicKey, const std::string &_signature, std::string &_recovered);
		bool verifyPssrSha256NoMsg(const std::string &_publicKey, const std::string &_message, const std::string &_signature);
		bool verifyPssrSha512(const std::string &_publicKey, const std::string &_signature, std::string &_recovered);
		bool verifyPssrSha512NoMsg(const std::string &_publicKey, const std::string &_message, const std::string &_signature);
		bool verifyPssrWhirepool(const std::string &_publicKey, const std::string &_signature, std::string &_recovered);
		bool verifyPssrWhirepoolNoMsg(const std::string &_publicKey, const std::string &_message, const std::string &_signature);

	}

	namespace aes
	{
		//AES can use 16, 24, or 32 byte keys (128, 192, and 256 bits respectively). 
		bool encodeCfb128(char* _data, std::size_t _length, const std::string& _key128);
		bool decodeCfb128(char* _data, std::size_t _length, const std::string& _key128);

		bool encodeCfb128(char* _data, std::size_t _length, const std::string& _key128, const std::string& _iv128);
		bool decodeCfb128(char* _data, std::size_t _length, const std::string& _key128, const std::string& _iv128);

		bool encodeCfb256(char* _data, std::size_t _length, const std::string& _key256);
		bool decodeCfb256(char* _data, std::size_t _length, const std::string& _key256);

		bool encodeCfb256(char* _data, std::size_t _length, const std::string& _key256, const std::string& _iv128);
		bool decodeCfb256(char* _data, std::size_t _length, const std::string& _key256, const std::string& _iv128);
	}

	//
	// Verify signature.
	//
	bool verifyRsaSha1(const std::string &_publicKey, const std::string &_message, const std::string &_signature, std::string& _recovered);

}
