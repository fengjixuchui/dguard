//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#pragma once


#include "DgiCrypt.h"
#include "../../helpers/encryption/cryptoHelpers.h"
#include "../../helpers/encryption/Grader/Grader.h"

namespace logic
{
	namespace common
	{

		//
		// Base defined.
		//

		DgiCrypt::DgiCrypt()
		{
			//
			// Be careful with exceptions.
			//
			m_hashers[::logic::common::Hash_Crc32] = new HasherCrc32();
			m_hashers[::logic::common::Hash_Md5] = new HasherMd5();
			m_hashers[::logic::common::Hash_Sha1] = new HasherSha1();
			m_hashers[::logic::common::Hash_Sha256] = new HasherSha256();
			m_hashers[::logic::common::Hash_Sha512] = new HasherSha512();
			m_hashers[::logic::common::Hash_Whirepool] = new HasherWhirepool();

			//
			// Symmetric cryptography algorithms.
			//
			m_symCryptors[::logic::common::CA_Grader] = new EncodingGrader();
			m_symCryptors[::logic::common::CA_Aes] = new EncoderAES();
			m_symCryptors[::logic::common::CA_Aes256] = new EncoderAES256();
		}

		DgiCrypt::~DgiCrypt()
		{
			for (auto hasher : m_hashers)
			{
				if ( hasher.second )
				{
					delete hasher.second;
				}
			}

			m_hashers.clear();

			for (auto cryptor : m_symCryptors)
			{
				if (cryptor.second)
				{
					delete cryptor.second;
				}
			}

			m_symCryptors.clear();
		}

		bool DgiCrypt::isSupported(HashAlgorithm _type)
		{
			std::unique_lock<std::mutex> lock(m_lock);

			return (m_hashers.count(_type) != 0);
		}

		bool DgiCrypt::isSupported(CryptAlgorithm _type)
		{
			std::unique_lock<std::mutex> lock(m_lock);

			return (m_symCryptors.count(_type) != 0);
		}

		Hasher* DgiCrypt::get(HashAlgorithm _type)
		{
			std::unique_lock<std::mutex> lock(m_lock);

			if (m_hashers.count(_type) == 0)
			{
				//
				// Return empty ptr.
				//
				return nullptr;
			}

			std::shared_ptr<Hasher> hasher = nullptr;

			return m_hashers.at(_type);
		}

		SymCryptor* DgiCrypt::get(CryptAlgorithm _type)
		{
			std::unique_lock<std::mutex> lock(m_lock);

			if (m_symCryptors.count(_type) == 0)
			{
				//
				// Return empty ptr.
				//
				return nullptr;
			}

			return (m_symCryptors.at(_type));
		}

		bool DgiCrypt::addHasher(HashAlgorithm _type, Hasher* _hasher)
		{
			std::unique_lock<std::mutex> lock(m_lock);

			if (m_hashers.count(_type) != 0)
			{
				return false;
			}

			m_hashers[_type] = _hasher;

			return true;
		}

		bool DgiCrypt::addSymCryptor(CryptAlgorithm _type, SymCryptor* _cryptor)
		{
			std::unique_lock<std::mutex> lock(m_lock);

			if (m_symCryptors.count(_type) != 0)
			{
				return false;
			}

			m_symCryptors[_type] = _cryptor;

			return true;
		}

		bool DgiCrypt::removeLink(HashAlgorithm _type)
		{
			std::unique_lock<std::mutex> lock(m_lock);

			auto pos = m_hashers.find(_type);
			if ( pos != m_hashers.end() )
			{
				m_hashers.erase(pos);
				return true;
			}

			return false;
		}

		bool DgiCrypt::removeLink(CryptAlgorithm _type)
		{
			std::unique_lock<std::mutex> lock(m_lock);

			auto pos = m_symCryptors.find(_type);
			if (pos != m_symCryptors.end())
			{
				m_symCryptors.erase(pos);
				return true;
			}

			return false;
		}

		bool DgiCrypt::remove(HashAlgorithm _type)
		{
			std::unique_lock<std::mutex> lock(m_lock);

			auto pos = m_hashers.find(_type);
			if (pos != m_hashers.end())
			{
				m_hashers.erase(pos);

				// Destroy and free memory for the object.
				if (pos->second) // Is is unnecesary but I prefer to make additional verification.
				{
					delete pos->second;
				}

				return true;
			}

			return false;
		}

		bool DgiCrypt::remove(CryptAlgorithm _type)
		{
			std::unique_lock<std::mutex> lock(m_lock);

			auto pos = m_symCryptors.find(_type);
			if (pos != m_symCryptors.end())
			{
				m_symCryptors.erase(pos);

				// Destroy and free memory for the object.
				if (pos->second)
				{
					delete pos->second;
				}

				return true;
			}

			return false;
		}

		//////////////////////////////////////////////////////////////////////////

		std::string HasherCrc32::getHash(const char* _ptrData, std::size_t _dataSize)
		{
			if (_ptrData == nullptr){
				return "";
			}

			return crypto::getCrc32((const unsigned char*)_ptrData, _dataSize);
		}

		std::string HasherCrc32::getRawHash(const char* _ptrData, std::size_t _dataSize)
		{
			if (_ptrData == nullptr){
				return "";
			}

			return crypto::getCrc32Raw((const unsigned char*)_ptrData, _dataSize);
		}

		//
		// Implementation for Md5 hash calculating method. 
		//

		std::string HasherMd5::getHash(const char* _ptrData, std::size_t _dataSize)
		{
			if (_ptrData == nullptr){
				return ""; // empty string as an error.
			}

			return crypto::getMd5( (const unsigned char*) _ptrData, _dataSize);
		}

		std::string HasherMd5::getRawHash(const char* _ptrData, std::size_t _dataSize)
		{
			if (_ptrData == nullptr)
			{
				return ""; // empty string as an error.
			}

			return crypto::getMd5Raw((const unsigned char*)_ptrData, _dataSize);
		}

		std::string HasherSha1::getHash(const char* _ptrData, std::size_t _dataSize)
		{
			if (_ptrData == nullptr)
			{
				return ""; // empty string as an error.
			}

			return crypto::getSha1((const unsigned char*)_ptrData, _dataSize);
		}

		std::string HasherSha1::getRawHash(const char* _ptrData, std::size_t _dataSize)
		{
			if (_ptrData == nullptr){
				return "";
			}

			return crypto::getSha1Raw((const unsigned char*)_ptrData, _dataSize);
		}

		std::string HasherSha256::getHash(const char* _ptrData, std::size_t _dataSize)
		{
			if (_ptrData == nullptr){
				return "";
			}

			return crypto::getSha256((const unsigned char*)_ptrData, _dataSize);
		}

		std::string HasherSha256::getRawHash(const char* _ptrData, std::size_t _dataSize)
		{
			if (_ptrData == nullptr){
				return "";
			}

			return crypto::getSha256Raw((const unsigned char*)_ptrData, _dataSize);
		}

		std::string HasherSha512::getHash(const char* _ptrData, std::size_t _dataSize)
		{
			if (_ptrData == nullptr){
				return ""; // empty string as an error.
			}

			return crypto::getSha512((const unsigned char*)_ptrData, _dataSize);
		}

		std::string HasherSha512::getRawHash(const char* _ptrData, std::size_t _dataSize)
		{
			if (_ptrData == nullptr){
				return "";
			}

			return crypto::getSha512Raw((const unsigned char*)_ptrData, _dataSize);
		}

		std::string HasherWhirepool::getHash(const char* _ptrData, std::size_t _dataSize)
		{
			if (_ptrData == nullptr)
			{
				return ""; // empty string as an error.
			}

			return crypto::getWhirepool((const unsigned char*)_ptrData, _dataSize);
		}

		std::string HasherWhirepool::getRawHash(const char* _ptrData, std::size_t _dataSize)
		{
			if (_ptrData == nullptr){
				return "";
			}

			return crypto::getWhirepoolRaw((const unsigned char*)_ptrData, _dataSize);
		}

		//
		// Symmetrical 64bit key encryption.
		//

		bool EncodingGrader::decrypt(char* _ptrData, std::size_t _dataSize, const char* _key, unsigned long _keySize)
		{
			if (!_ptrData || !_key)
			{
				return false;
			}

			std::string performedKey = get64BitKey(_key, _keySize);

			uint64_t key = perform(performedKey);

			encryption::Grader::decryptData(_ptrData, _dataSize, key, 16);

			return true;
		}

		bool EncodingGrader::encrypt(char* _ptrData, std::size_t _dataSize, const char* _key, unsigned long _keySize)
		{
			if (!_ptrData || !_key)
			{
				return false;
			}

			std::string performedKey = get64BitKey(_key, _keySize);

			uint64_t key = perform(performedKey);

			encryption::Grader::encryptData(_ptrData, _dataSize, key, 16);

			return true;
		}

		unsigned long EncodingGrader::keySize()
		{
			//
			// The max size of key is limited to 64 bits which is 8 bytes.
			//
			return 8;
		}


		std::string EncodingGrader::get64BitKey(const char* _key, unsigned long _keySize)
		{
			return performKey(8, (const unsigned char*) _key, _keySize);
		}

		uint64_t EncodingGrader::perform(const std::string& _key)
		{
			uint64_t key = 0;
			
			std::string performedKey = performKey(8, (const unsigned char*)_key.c_str(), _key.size());

			memcpy(&key, performedKey.c_str(), sizeof(key));
			
			return key;
		}

		std::string performKey(unsigned long _needSize, const unsigned char* _key, unsigned long _currentKeySize)
		{
			//
			// Create zeroed key.
			//
			std::string performedKey(_needSize, 0);

			// If the length of a key is smaller than the need size - copy it and live last part filled with zeros.
			// Translate small key into big.
			if (_currentKeySize <= _needSize)
			{
				for (unsigned long i = 0; i < _currentKeySize; ++i)
				{
					performedKey[i] = _key[i];
				}
			}
			else // if (_currentKeySize > _needSize) Big key -> to small.
			{
				// Enumerate each first sizeof(key) elements.
				for (unsigned long i = 0; i < _needSize; ++i)
				{
					byte encodedByte = _key[i];

					// i xor i = 0 do not forget please =) 
					//
					// Xor each 'i' byte with all last.
					//
					for (unsigned long k = i + 1; k < _currentKeySize; ++k)
					{
						encodedByte = encodedByte ^ _key[k];
					}

					performedKey[i] = encodedByte;
				}

				for (unsigned long i = 0; i < _needSize; ++i)
				{
					performedKey[i] = (byte)performedKey[i] ^ _key[i];
				}

			}

			return performedKey;
		}

		bool EncoderAES::decrypt(char* _ptrData, std::size_t _dataSize, const char* _key, unsigned long _keySize)
		{
			if (!(_ptrData && _key && _dataSize && _keySize))
			{
				return false;
			}

			std::string performedKey = get128BitKey(_key, _keySize);

			return crypto::aes::decodeCfb128(_ptrData, _dataSize, performedKey.data());
		}

		bool EncoderAES::encrypt(char* _ptrData, std::size_t _dataSize, const char* _key, unsigned long _keySize)
		{
			if (!(_ptrData && _key && _dataSize && _keySize))
			{
				return false;
			}

			std::string performedKey = get128BitKey(_key, _keySize);

			return crypto::aes::encodeCfb128(_ptrData, _dataSize, performedKey.data());
		}

		unsigned long EncoderAES::keySize()
		{
			return 16U;
		}

		std::string EncoderAES::get128BitKey(const char* _key, unsigned long _keySize)
		{
			//
			// Returns md5 hash of the key. It's suit because the key can be bigger or shorter than 128 bits.
			//
			return crypto::getMd5Raw((const unsigned char*)_key, _keySize);
		}

		bool EncoderAES256::decrypt(char* _ptrData, std::size_t _dataSize, const char* _key, unsigned long _keySize)
		{
			if (!(_ptrData && _key && _dataSize && _keySize))
			{
				return false;
			}

			std::string performedKey = get256BitKey(_key, _keySize);

			return crypto::aes::decodeCfb256(_ptrData, _dataSize, performedKey.data());
		}

		bool EncoderAES256::encrypt(char* _ptrData, std::size_t _dataSize, const char* _key, unsigned long _keySize)
		{
			if (!(_ptrData && _key && _dataSize && _keySize))
			{
				return false;
			}

			std::string performedKey = get256BitKey(_key, _keySize);

			return crypto::aes::encodeCfb256(_ptrData, _dataSize, performedKey.data());
		}

		unsigned long EncoderAES256::keySize()
		{
			return 32U;
		}

		std::string EncoderAES256::get256BitKey(const char* _key, unsigned long _keySize)
		{
			return crypto::getSha256Raw((const unsigned char*)_key, _keySize);
		}

	}
}
