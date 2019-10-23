//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#pragma once

#include <set>
#include <map>
#include <mutex>
#include <memory>

#include "DgiCommon.h"

namespace logic
{
	namespace common
	{
		std::string performKey(unsigned long _needSize, const unsigned char* _key, unsigned long _keysize);

		class Hasher
		{
		public:
			virtual ~Hasher(){}

			//
			// That function calculates hash and returns it in 'std::string' container.
			//
			virtual std::string getHash(const char* _ptrData, std::size_t _dataSize) = 0;
			virtual std::string getRawHash(const char* _ptrData, std::size_t _dataSize) = 0;
		};

		//
		// For symmetrical algorithms which are not changes the size of data.
		//
		class SymCryptor
		{
		public:

			virtual bool decrypt(char* _ptrData, std::size_t _dataSize, // Data which is going to be decrypted.
								const char* _key, unsigned long _keySize // Decryption key.
							) = 0;

			virtual bool encrypt(char* _ptrData, std::size_t _dataSize, // Data which is going to be encrypted.
				const char* _key, unsigned long _keySize // Encryption key.
				) = 0;

			virtual unsigned long keySize() = 0; // returns the size of key in bytes.
		};

		class DgiCrypt: public boost::noncopyable
		{
		public:

			//
			// By default that object has a couple of defined hashing algorithms.
			//
			DgiCrypt();

			// 
			// Destroys and frees all cryptographic objects - hashers, cryptors.
			// 
			~DgiCrypt();

			//
			// Returns true if algorithm is supported.
			//
			bool isSupported(HashAlgorithm _type);
			bool isSupported(CryptAlgorithm _type);


			Hasher* get(HashAlgorithm);
			SymCryptor* get(CryptAlgorithm);

			bool addHasher(HashAlgorithm _type, Hasher* _hasher);
			bool addSymCryptor(CryptAlgorithm _type, SymCryptor* _cryptor);

			// Removes only link without object destroying and memory free.
			//
			bool removeLink(HashAlgorithm _type);
			bool removeLink(CryptAlgorithm _type);

			// Deletes cryptographic object from the internal container.
			//
			bool remove(HashAlgorithm _type);
			bool remove(CryptAlgorithm _type);

		protected:
			std::mutex m_lock;

			std::map<HashAlgorithm, Hasher*> m_hashers;
			std::map<CryptAlgorithm, SymCryptor*> m_symCryptors;
		};

		class HasherCrc32 : public Hasher
		{
		public:
			virtual std::string getHash(const char* _ptrData, std::size_t _dataSize);
			virtual std::string getRawHash(const char* _ptrData, std::size_t _dataSize);
		};

		//
		// Md5
		//
		class HasherMd5 : public Hasher
		{
		public:
			virtual std::string getHash(const char* _ptrData, std::size_t _dataSize);
			virtual std::string getRawHash(const char* _ptrData, std::size_t _dataSize);
		};

		//
		// Sha1
		//
		class HasherSha1 : public Hasher
		{
		public:
			virtual std::string getHash(const char* _ptrData, std::size_t _dataSize);
			virtual std::string getRawHash(const char* _ptrData, std::size_t _dataSize);
		};

		//
		// Sha256
		//
		class HasherSha256 : public Hasher
		{
		public:
			virtual std::string getHash(const char* _ptrData, std::size_t _dataSize);
			virtual std::string getRawHash(const char* _ptrData, std::size_t _dataSize);
		};

		//
		// Sha512
		//
		class HasherSha512 : public Hasher
		{
		public:
			virtual std::string getHash(const char* _ptrData, std::size_t _dataSize);
			virtual std::string getRawHash(const char* _ptrData, std::size_t _dataSize);
		};

		//
		// Whirepool hashing algorithm
		//
		class HasherWhirepool : public Hasher
		{
		public:
			virtual std::string getHash(const char* _ptrData, std::size_t _dataSize);
			virtual std::string getRawHash(const char* _ptrData, std::size_t _dataSize);
		};


		//
		// Symmetric 64bit encoding algorithm.
		//
		class EncodingGrader : public SymCryptor
		{
		public:

			virtual bool decrypt(char* _ptrData, std::size_t _dataSize,
				const char* _key, unsigned long _keySize);

			virtual bool encrypt(char* _ptrData, std::size_t _dataSize,
				const char* _key, unsigned long _keySize);

			virtual unsigned long keySize();

		protected:

			//
			// Performs long key to short 64 bit key.
			//
			std::string get64BitKey(const char* _key, unsigned long _keySize);
			uint64_t perform(const std::string& _key);
		};

		//
		// AES with 128 bit key.
		//
		class EncoderAES : public SymCryptor
		{
		public:

			virtual bool decrypt(char* _ptrData, std::size_t _dataSize,
				const char* _key, unsigned long _keySize);

			virtual bool encrypt(char* _ptrData, std::size_t _dataSize,
				const char* _key, unsigned long _keySize);

			virtual unsigned long keySize();

		protected:

			//
			// Performs long key to short 128 bit key.
			//
			std::string get128BitKey(const char* _key, unsigned long _keySize);
		};

		//
		// AES with 128 bit key.
		//
		class EncoderAES256 : public SymCryptor
		{
		public:

			virtual bool decrypt(char* _ptrData, std::size_t _dataSize,
				const char* _key, unsigned long _keySize);

			virtual bool encrypt(char* _ptrData, std::size_t _dataSize,
				const char* _key, unsigned long _keySize);

			virtual unsigned long keySize();

		protected:

			//
			// Performs long key to short 128 bit key.
			//
			std::string get256BitKey(const char* _key, unsigned long _keySize);
		};
	}
}
