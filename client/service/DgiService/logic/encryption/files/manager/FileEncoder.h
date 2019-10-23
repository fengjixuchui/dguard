//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#pragma once

#include "../../../../logic/common/DgiEngine.h"
#include "../../../../logic/common/master-password.h"


namespace logic
{
	namespace encryption
	{
		const unsigned char FE_Signature[16] = {
			0xf3, 0xa1, 0xcd, 0x00, /* 4 bytes*/
			0xf1, 0x19, 0x42, 0xe9, /* 8 bytes */
			0x14, 0x98, 0x89, 0x5d, /* 12 bytes */
			0xaa, 0x10, 0x3d, 0x00 /* 16 bytes */
		};

		const unsigned char FE_SignatureEnd[16] = {
			0xaa, 0xbb, 0xcc, 0xdd, /* 4 bytes*/
			0xff, 0x11, 0xde, 0xee, /* 8 bytes */
			0x13, 0x78, 0x55, 0x40, /* 12 bytes */
			0x00, 0x1e, 0xc4, 0x00 /* 16 bytes */
		};

		const unsigned long MinEncodingChunkSize = 1024 * 1024 * 64; // 64 MB it's a minimal size for chunk.


#pragma pack(push, 1)
		
		//
		//	That information will be in file header like IMAGE_NT_HEADERS in PE format.
		//

		struct EncodingMetaInfo
		{
			//
			//	This array has unique byte sequence which are a signature,
			//	that indicate this structure with encoding-meta-data.
			//

			unsigned char headerSignature[16]; // FE_Signature container for
			::logic::common::Sfci sfc;

			//char dateTime[50 /* including last zero symbol */]; // "11:11:11 12.09.2019" time when data were encoded
			bool usedMasterPassword;
			common::CryptAlgorithm encodingAlgorithm;
			LONGLONG originalFileSize;
			char originalChecksum[64 + 1 /* including last zero symbol */ ]; // sha-256
			char keyChecksum[64 + 1 /* including last zero symbol */ ]; // sha-256

			unsigned char endSignature[16]; // FE_SignatureEnd container for
		};

#pragma pack(pop)

		class FileEncoder
		{
		public:
			FileEncoder(logfile& _log);

			//
			//	Opens file and forbids an access to the file.
			//	It protects against multiple access to one file from many different processes.
			//

			bool acquireFile(const std::wstring& _filepath, bool _readOnly, HANDLE& _hFile) const;
			bool leaveFile(HANDLE _hFile) const;

			logic::common::InternalStatus isEncoded(const std::wstring& _filepath, bool & _result);
			bool isEncoded(HANDLE _hFile);

			//
			//	Encodes data using master password.
			//

			common::InternalStatus encode(const std::wstring& _filepath, common::CryptAlgorithm _cryptAlg, const common::MasterPassword& _password, EncodingMetaInfo & _information);
			common::InternalStatus encode(HANDLE _hFile, common::CryptAlgorithm _cryptAlg, const common::MasterPassword& _password, EncodingMetaInfo & _information);
			common::InternalStatus encode(const std::wstring& _filepath, common::CryptAlgorithm _cryptAlg, const std::string& _key, EncodingMetaInfo& _information);
			common::InternalStatus encode(HANDLE _hFile, common::CryptAlgorithm _cryptAlg, const std::string& _key, EncodingMetaInfo& _information);

			//
			//	Decodes data using custom encryption key.
			//

			common::InternalStatus decode(const std::wstring& _filepath, const std::string& _key, bool& _dataCompromised);
			common::InternalStatus decode(HANDLE _hFile, const std::string& _key, bool& _dataCompromised);
			common::InternalStatus decode(const std::wstring& _filepath, const common::MasterPassword& _password, bool& _dataCompromised);
			common::InternalStatus decode(HANDLE _hFile, const common::MasterPassword& _password, bool& _dataCompromised);

			bool readInfo(const std::wstring& _filepath, EncodingMetaInfo& _result);
			bool readInfo(HANDLE _hFile, EncodingMetaInfo& _result);

			bool writeInfo(const std::wstring& _filepath, const EncodingMetaInfo& _metaInfo);
			bool writeInfo(HANDLE _hFile, const EncodingMetaInfo& _metaInfo);

			bool removeMetaInfo(const std::wstring& _filepath);
			bool removeMetaInfo(HANDLE _hFile);

			bool verifyMetaInfo(const std::wstring& _filepath);
			bool verifyMetaInfo(HANDLE _hFile);

			//
			//	Returns empty container with sent signature fields for future filling with actual information.
			//

			EncodingMetaInfo getMetaContainer();

		protected:

			common::InternalStatus internalDecode(
				HANDLE _hFile,
				const std::string& _key,
				bool& _isCompromisedIntegrity,
				std::string& _decodedDataHash
				);

			common::InternalStatus internalEncode(
				HANDLE _hFile, /*common::Hasher _hasher,*/
				common::CryptAlgorithm m_cryptAlg,
				const std::string& _key,
				EncodingMetaInfo& _result
				);

			bool encodeFileData(
				HANDLE _hFile,
				common::SymCryptor* _cryptor,
				common::Hasher* _hasher,
				const std::string& _encryptionKey,
				std::string& _strOriginalDataHash,
				std::string& _strEncodedDataHash
				);

			bool decodeFileData(
				HANDLE _hFile,
				common::SymCryptor* _cryptor,
				common::Hasher* _hasher,
				const std::string& _encryptionKey,
				std::string& _strEncodedDataHash,
				std::string& _strOriginalDataHash
				);

		private:
			logfile& m_log;
			::logic::common::DgiCrypt m_cryptor;
		};
	}
}
