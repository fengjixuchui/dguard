//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//



#define _WINSOCKAPI_
#include <windows.h>
#undef _WINSOCKAPI_

#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include <cryptlib.h>
#include <md5.h>
#include <crc.h>
#include <gost.h>
#include <hex.h>
#include <osrng.h>
#include <pssr.h>
#include <rsa.h>
#include <whrlpool.h>
#include <filters.h>
#include "cryptoHelpers.h"

#include <aes.h>
#include <modes.h>

#pragma comment(lib, "cryptlib.lib")

namespace crypto
{
	template<class HashAlgT, unsigned long digestSize>
	static std::string calculateHash(const byte* _pBuffer, size_t _sizeBuffer)
	{
		std::string hexHash;
		HashAlgT hash;
		byte digest[digestSize] = { 0 };

		hash.CalculateDigest(digest, _pBuffer, _sizeBuffer);

		CryptoPP::HexEncoder encoder;
		encoder.Attach(new CryptoPP::StringSink(hexHash));
		encoder.Put(digest, sizeof(digest));
		encoder.MessageEnd();

		return hexHash;
	}

	template<class HashAlgT, unsigned long digestSize>
	static std::string calculateHashRawResult(const byte* _pBuffer, size_t _sizeBuffer)
	{
		HashAlgT hash;
		byte digest[digestSize] = { 0 };

		hash.CalculateDigest(digest, _pBuffer, _sizeBuffer);

		return std::string((const char*)digest, digestSize);
	}

	template <class TVerifier>
	bool verifyRsaPssr(const std::string &_publicKey, const std::string &_message, const std::string &_signature, std::string& _recovered)
	{
		//
		// Decode and load public key (using pipeline).
		//
		CryptoPP::RSA::PublicKey publicKey;
		publicKey.Load(CryptoPP::StringSource(_publicKey, true).Ref());

		//
		// Verify message.
		//
		bool result = false;
		TVerifier verifier(publicKey);

		std::string  recovered;

		//
		// Without exceptions.
		//
		// 		CryptoPP::StringSource ss2
		// 		(
		// 			_signature + _message,
		// 			true,
		// 			new CryptoPP::SignatureVerificationFilter(verifier,new CryptoPP::ArraySink((byte*)&result,sizeof(result)))
		// 		);
		// 
		// 		//verifier.RecoverMessage()
		// 
		// 		return result;

		try
		{
			CryptoPP::StringSource ss
				(
				_message + _signature,
				true,
				new CryptoPP::SignatureVerificationFilter(
				verifier,
				new CryptoPP::StringSink(recovered),
				CryptoPP::HashVerificationFilter::Flags::THROW_EXCEPTION | CryptoPP::HashVerificationFilter::Flags::PUT_MESSAGE)
				);

		}
		catch (...)
		{
			return false;
		}

		recovered.swap(_recovered);
		return true;
	}

	std::string getMd5(const unsigned char* _pBuffer, size_t _sizeBuffer)
	{
		return calculateHash<CryptoPP::Weak1::MD5, CryptoPP::Weak1::MD5::DIGESTSIZE>((byte*)_pBuffer, _sizeBuffer);
	}

	std::string getMd5Raw(const unsigned char* _pBuffer, size_t _sizeBuffer)
	{
		return calculateHashRawResult<CryptoPP::Weak1::MD5, CryptoPP::Weak1::MD5::DIGESTSIZE>((byte*)_pBuffer, _sizeBuffer);
	}

	std::string getCrc32(const unsigned char* _pBuffer, size_t _sizeBuffer)
	{
		return calculateHash<CryptoPP::CRC32, CryptoPP::CRC32::DIGESTSIZE>((byte*)_pBuffer, _sizeBuffer);
	}

	std::string getCrc32Raw(const unsigned char* _pBuffer, size_t _sizeBuffer)
	{
		return calculateHashRawResult<CryptoPP::CRC32, CryptoPP::CRC32::DIGESTSIZE>((byte*)_pBuffer, _sizeBuffer);
	}

	std::string getSha1(const unsigned char* _pBuffer, size_t _sizeBuffer)
	{
		return calculateHash<CryptoPP::SHA1, CryptoPP::SHA1::DIGESTSIZE>((byte*)_pBuffer, _sizeBuffer);
	}

	std::string getSha1Raw(const unsigned char* _pBuffer, size_t _sizeBuffer)
	{
		return calculateHashRawResult<CryptoPP::SHA1, CryptoPP::SHA1::DIGESTSIZE>((byte*)_pBuffer, _sizeBuffer);
	}

	std::string getSha256(const unsigned char* _pBuffer, size_t _sizeBuffer)
	{
		return calculateHash<CryptoPP::SHA256, CryptoPP::SHA256::DIGESTSIZE>((byte*)_pBuffer, _sizeBuffer);
	}

	std::string getSha256Raw(const unsigned char* _pBuffer, size_t _sizeBuffer)
	{
		return calculateHashRawResult<CryptoPP::SHA256, CryptoPP::SHA256::DIGESTSIZE>((byte*)_pBuffer, _sizeBuffer);
	}

	std::string getSha512(const unsigned char* _pBuffer, size_t _sizeBuffer)
	{
		return calculateHash<CryptoPP::SHA512, CryptoPP::SHA512::DIGESTSIZE>((byte*)_pBuffer, _sizeBuffer);
	}

	std::string getSha512Raw(const unsigned char* _pBuffer, size_t _sizeBuffer)
	{
		return calculateHashRawResult<CryptoPP::SHA512, CryptoPP::SHA512::DIGESTSIZE>((byte*)_pBuffer, _sizeBuffer);
	}

	std::string getWhirepool(const unsigned char* _pBuffer, size_t _sizeBuffer)
	{
		return calculateHash<CryptoPP::Whirlpool, CryptoPP::Whirlpool::DIGESTSIZE>((byte*)_pBuffer, _sizeBuffer);
	}

	std::string getWhirepoolRaw(const unsigned char* _pBuffer, size_t _sizeBuffer)
	{
		return calculateHashRawResult<CryptoPP::Whirlpool, CryptoPP::Whirlpool::DIGESTSIZE>((byte*)_pBuffer, _sizeBuffer);
	}

	namespace rsa
	{

		bool genKeys(::crypto::KeyPair& _keys, unsigned long _keySize)
		{
			//
			// PGP Random Pool-like generator
			//
			CryptoPP::AutoSeededRandomPool rng;

			//
			// generate keys
			//
			CryptoPP::RSA::PrivateKey privateKey;
			privateKey.GenerateRandomWithKeySize(rng, _keySize);
			CryptoPP::RSA::PublicKey publicKey(privateKey);

			// save keys
			//publicKey.Save(CryptoPP::HexEncoder( new CryptoPP::StringSink(keyPair.publicKey)).Ref());
			//privateKey.Save(CryptoPP::HexEncoder(new CryptoPP::StringSink(keyPair.privateKey)).Ref());

			publicKey.Save(CryptoPP::StringSink(_keys.publicKey).Ref());
			privateKey.Save(CryptoPP::StringSink(_keys.privateKey).Ref());

			return true;
		}

		//
		// Signs data with PSSR schema.
		//

		template <class TSigner>
		bool signRsaPssr(const std::string &_privateKeyBin, const std::string &_message, std::string& _signedMessage, bool _putTheMessage = true)
		{
			try
			{
				CryptoPP::RSA::PrivateKey privateKey;
				privateKey.Load(CryptoPP::StringSource(_privateKeyBin, true).Ref());

				std::string signature;
				//CryptoPP::RSASS<CryptoPP::PSSR, CryptoPP::SHA256>::Signer signer(privateKey);
				TSigner signer(privateKey);

				CryptoPP::AutoSeededRandomPool rng;

				CryptoPP::StringSource ss(
					_message,
					true,
					new CryptoPP::SignerFilter(rng, signer, new CryptoPP::StringSink(signature), _putTheMessage)
					);

				signature.swap(_signedMessage);
			}
			catch (...)
			{
				return false;
			}

			return true;
		}

		template <class TVerifier>
		bool verifyPssr(const std::string &_publicKey, const std::string &_signature, std::string& _recovered)
		{
			std::string  recovered;

			try
			{
				CryptoPP::RSA::PublicKey publicKey;
				publicKey.Load(CryptoPP::StringSource(_publicKey, true).Ref());

				TVerifier verifier(publicKey);

				CryptoPP::StringSource ss
					(
					/*_message + */_signature,
					true,
					new CryptoPP::SignatureVerificationFilter(
					verifier,
					new CryptoPP::StringSink(recovered),
					CryptoPP::HashVerificationFilter::Flags::THROW_EXCEPTION | CryptoPP::HashVerificationFilter::Flags::PUT_MESSAGE)
					);

			}
			catch (...)
			{
				recovered.swap(_recovered);
				return false;
			}

			recovered.swap(_recovered);
			return true;
		}

		template <class TVerifier>
		bool verifyPssrNoMsg(const std::string &_publicKey, const std::string &_message, const std::string &_signature)
		{
			try
			{
				std::string  recovered;

				CryptoPP::RSA::PublicKey publicKey;
				publicKey.Load(CryptoPP::StringSource(_publicKey, true).Ref());

				CryptoPP::RSASS<CryptoPP::PSSR, CryptoPP::SHA1>::Verifier verifier(publicKey);

				CryptoPP::StringSource ss
					(
					_message + _signature,
					true,
					new CryptoPP::SignatureVerificationFilter(verifier, new CryptoPP::StringSink(recovered),
					CryptoPP::HashVerificationFilter::Flags::THROW_EXCEPTION | CryptoPP::HashVerificationFilter::Flags::PUT_MESSAGE)
					);

			}
			catch (...)
			{
				return false;
			}

			return true;
		}

		std::string signPssrSha1(const std::string &_privateKeyBin, const std::string &_message, bool _putTheMessage /*= true*/)
		{
			std::string signature;
			auto res = signRsaPssr<CryptoPP::RSASS<CryptoPP::PSSR, CryptoPP::SHA1>::Signer>(_privateKeyBin, _message, signature, _putTheMessage);
			return signature;
		}

		std::string signPssrSha256(const std::string &_privateKeyBin, const std::string &_message, bool _putTheMessage /*= true*/)
		{
			std::string signature;
			auto res = signRsaPssr<CryptoPP::RSASS<CryptoPP::PSSR, CryptoPP::SHA256>::Signer>(_privateKeyBin, _message, signature);

			if (res)
			{
				// success
			}

			return signature;
		}

		std::string signPssrSha512(const std::string &_privateKeyBin, const std::string &_message, bool _putTheMessage /*= true*/)
		{
			std::string signature;
			auto res = signRsaPssr<CryptoPP::RSASS<CryptoPP::PSSR, CryptoPP::SHA512>::Signer>(_privateKeyBin, _message, signature);
			return signature;
		}

		std::string signPssrWhirepool(const std::string &_privateKeyBin, const std::string &_message, bool _putTheMessage /*= true*/)
		{
			std::string signature;
			auto res = signRsaPssr<CryptoPP::RSASS<CryptoPP::PSSR, CryptoPP::Whirlpool>::Signer>(_privateKeyBin, _message, signature);
			return signature;
		}
	
		
		//
		// Digital signature verification.
		//

		bool verifyPssrSha1(const std::string &_publicKey, const std::string &_signature, std::string& _recovered)
		{
			return verifyPssr<CryptoPP::RSASS<CryptoPP::PSSR, CryptoPP::SHA1>::Verifier>(_publicKey, _signature, _recovered);
		}

		bool verifyPssrSha1NoMsg(const std::string &_publicKey, const std::string &_message, const std::string &_signature)
		{
			return verifyPssrNoMsg<CryptoPP::RSASS<CryptoPP::PSSR, CryptoPP::SHA1>::Verifier>(_publicKey, _message, _signature);
		}

		bool verifyPssrSha256(const std::string &_publicKey, const std::string &_signature, std::string &_recovered)
		{
			return verifyPssr<CryptoPP::RSASS<CryptoPP::PSSR, CryptoPP::SHA256>::Verifier>(_publicKey, _signature, _recovered);
		}

		bool verifyPssrSha256NoMsg(const std::string &_publicKey, const std::string &_message, const std::string &_signature)
		{
			return verifyPssrNoMsg<CryptoPP::RSASS<CryptoPP::PSSR, CryptoPP::SHA256>::Verifier>(_publicKey, _message, _signature);
		}

		bool verifyPssrSha512(const std::string &_publicKey, const std::string &_signature, std::string &_recovered)
		{
			return verifyPssr<CryptoPP::RSASS<CryptoPP::PSSR, CryptoPP::SHA512>::Verifier>(_publicKey, _signature, _recovered);
		}

		bool verifyPssrSha512NoMsg(const std::string &_publicKey, const std::string &_message, const std::string &_signature)
		{
			return verifyPssrNoMsg<CryptoPP::RSASS<CryptoPP::PSSR, CryptoPP::SHA512>::Verifier>(_publicKey, _message, _signature);
		}

		bool verifyPssrWhirepool(const std::string &_publicKey, const std::string &_signature, std::string &_recovered)
		{
			return verifyPssr<CryptoPP::RSASS<CryptoPP::PSSR, CryptoPP::Whirlpool>::Verifier>(_publicKey, _signature, _recovered);
		}

		bool verifyPssrWhirepoolNoMsg(const std::string &_publicKey, const std::string &_message, const std::string &_signature)
		{
			return verifyPssrNoMsg<CryptoPP::RSASS<CryptoPP::PSSR, CryptoPP::Whirlpool>::Verifier>(_publicKey, _message, _signature);
		}

	}


	namespace aes
	{
		static const byte IV_16[CryptoPP::AES::BLOCKSIZE /* 16 */ ] = {
			0xa3, 0x1b, 0x8c, 0xdd, /* 4 bytes*/
			0xff, 0x1e, 0xde, 0x7e, /* 8 bytes */
			0x01, 0x08, 0x50, 0x4d, /* 12 bytes */
			0xc7, 0x1e, 0x44, 0x00 /* 16 bytes */
		};

		bool encodeCfb128(char* _data, std::size_t _length, const std::string& _key128)
		{
			std::string iv((const char*)IV_16, sizeof(IV_16));

			return encodeCfb128(_data, _length, _key128, iv);
		}

		bool decodeCfb128(char* _data, std::size_t _length, const std::string& _key128)
		{
			std::string iv((const char*)IV_16, sizeof(IV_16));

			return decodeCfb128(_data, _length, _key128, iv);
		}

		bool encodeCfb256(char* _data, std::size_t _length, const std::string& _key256)
		{
			std::string iv((const char*)IV_16, sizeof(IV_16));

			return encodeCfb256(_data, _length, _key256, iv);
		}

		bool encodeCfb256(char* _data, std::size_t _length, const std::string& _key256, const std::string& _iv128)
		{
			const unsigned long encryptionKeyLength = CryptoPP::AES::MAX_KEYLENGTH; // 32 bytes
			const unsigned long blockSize = CryptoPP::AES::BLOCKSIZE; // 16 bytes

			byte key[encryptionKeyLength], iv[blockSize];
			memset(key, 0x00, encryptionKeyLength);
			memset(iv, 0x00, blockSize);

			//
			// The following example uses CFB mode and in-place encryption.
			// Since the mode is not ECB or CBC, the data's length does not need to be a multiple of AES's block size.
			//

			if ( _key256.empty() /*|| (_key256.length() > blockSize)*/ )
			{
				return false;
			}

			if (_iv128.length() != blockSize)
			{
				return false;
			}

			memcpy(iv, _iv128.data(), blockSize);
			memcpy(key, _key256.data(), _key256.length());

			try
			{
				CryptoPP::CFB_Mode<CryptoPP::AES>::Encryption cfbEncryption(key, encryptionKeyLength, iv);
				cfbEncryption.ProcessData((byte*)_data, (byte*)_data, _length);
			}
			catch (...)
			{
				return false;
			}

			return true;
		}

		bool decodeCfb256(char* _data, std::size_t _length, const std::string& _key256)
		{
			std::string iv((const char*)IV_16, sizeof(IV_16));

			return decodeCfb256(_data, _length, _key256, iv);
		}

		bool decodeCfb256(char* _data, std::size_t _length, const std::string& _key256, const std::string& _iv128)
		{
			const unsigned long encryptionKeyLength = CryptoPP::AES::MAX_KEYLENGTH; // 32
			const unsigned long blockSize = CryptoPP::AES::BLOCKSIZE; // 16

			byte key[encryptionKeyLength], iv[blockSize];
			memset(key, 0x00, encryptionKeyLength);
			memset(iv, 0x00, blockSize);


			//
			// The following example uses CFB mode and in-place decryption.
			// Since the mode is not ECB or CBC, the data's length does not need to be a multiple of AES's block size.
			//

			//
			// !!!!!!!!!
			//
// 			if (_key256.empty() || (_key256.length() > blockSize))
// 			{
// 				return false;
// 			}

			if (_key256.empty())
			{
				return false;
			}

			if (_iv128.length() != blockSize)
			{
				return false;
			}

			memcpy(iv, _iv128.data(), blockSize);
			memcpy(key, _key256.data(), _key256.length());

			try
			{
				CryptoPP::CFB_Mode<CryptoPP::AES>::Decryption cfbDecryption(key, encryptionKeyLength, iv);
				cfbDecryption.ProcessData((byte*)_data, (byte*)_data, _length);
			}
			catch (...)
			{
				return false;
			}

			return true;
		}

		bool encodeCfb128(char* _data, std::size_t _length, const std::string& _key128, const std::string& _iv128)
		{
			const unsigned long encryptionKeyLength = CryptoPP::AES::DEFAULT_KEYLENGTH;
			const unsigned long blockSize = CryptoPP::AES::BLOCKSIZE;

			byte key[encryptionKeyLength], iv[blockSize];
			memset(key, 0x00, encryptionKeyLength);
			memset(iv, 0x00, blockSize);

			if (_key128.empty())
			{
				return false;
			}
			
			//
			// Does it need to be here? I can't remember that.
			//

			if (_iv128.length() != blockSize)
			{
				return false;
			}

			memcpy(iv, _iv128.data(), blockSize);
			memcpy(key, _key128.data(), _key128.length());

			try
			{
				CryptoPP::CFB_Mode<CryptoPP::AES>::Encryption cfbEncryption(key, encryptionKeyLength, iv);
				cfbEncryption.ProcessData((byte*)_data, (byte*)_data, _length);
			}
			catch (...)
			{
				return false;
			}

			return true;
		}

		bool decodeCfb128(char* _data, std::size_t _length, const std::string& _key128, const std::string& _iv128)
		{
			const unsigned long encryptionKeyLength = CryptoPP::AES::DEFAULT_KEYLENGTH;
			const unsigned long blockSize = CryptoPP::AES::BLOCKSIZE;

			byte key[encryptionKeyLength], iv[blockSize];
			memset(key, 0x00, encryptionKeyLength);
			memset(iv, 0x00, blockSize);

			//
			// I forgot why I used line
			//
			// /*|| (_key128.length() > blockSize*/
			//
			if (_key128.empty() /*|| (_key128.length() > blockSize)*/)
			{
				return false;
			}

			if (_iv128.length() != blockSize)
			{
				return false;
			}

			memcpy(iv, _iv128.data(), blockSize);
			memcpy(key, _key128.data(), _key128.length());

			try
			{
				CryptoPP::CFB_Mode<CryptoPP::AES>::Decryption cfbDecryption(key, encryptionKeyLength, iv);
				cfbDecryption.ProcessData((byte*)_data, (byte*)_data, _length);
			}
			catch (...)
			{
				return false;
			}

			return true;
		}

	}
	

}
