//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#pragma once


#include "FileEncoder.h"


namespace logic
{
	namespace encryption
	{

		FileEncoder::FileEncoder(logfile& _log) :m_log(_log)
		{

		}

		bool FileEncoder::acquireFile(const std::wstring& _filepath, bool _readOnly, HANDLE& _hFile) const
		{
			DWORD accessLevel = _readOnly ? GENERIC_READ : (GENERIC_READ | GENERIC_WRITE);

			HANDLE hFile = CreateFileW(_filepath.c_str(), accessLevel, 0, NULL, OPEN_EXISTING, 0, NULL);

			if (hFile == INVALID_HANDLE_VALUE)
			{
				return false;
			}

			_hFile = hFile;
			return true;
		}

		bool FileEncoder::leaveFile(HANDLE _hFile) const
		{
			return CloseHandle(_hFile) == TRUE;
		}

		logic::common::InternalStatus FileEncoder::isEncoded(const std::wstring& _filepath, bool& _result)
		{
			std::string fn = __FUNCTION__;
			HANDLE hFile = INVALID_HANDLE_VALUE;

			if (acquireFile(_filepath, true, hFile))
			{
				_result = isEncoded(hFile);
				leaveFile(hFile);
			}
			else
			{
				m_log.print(fn + ": error - can't open the file.");
				return logic::common::InternalStatus::Int_NotFound;
			}

			return logic::common::InternalStatus::Int_Success;
		}

		bool FileEncoder::isEncoded(HANDLE _hFile)
		{
			return verifyMetaInfo(_hFile);
		}

		logic::common::InternalStatus FileEncoder::encode(const std::wstring& _filepath,
			common::CryptAlgorithm _cryptAlg,
			const common::MasterPassword& _password,
			EncodingMetaInfo & _information)
		{
			std::string fn = __FUNCTION__;
			HANDLE hFile = INVALID_HANDLE_VALUE;
			logic::common::InternalStatus result = common::InternalStatus::Int_Success;

			if (acquireFile(_filepath, false, hFile))
			{
				result = encode(hFile, _cryptAlg, _password, _information);
				leaveFile(hFile);
			}
			else
			{
				m_log.print(fn + ": error - can't open the file.");
			}

			return result;
		}

		common::InternalStatus FileEncoder::encode(HANDLE _hFile, common::CryptAlgorithm _cryptAlg, const common::MasterPassword& _password, EncodingMetaInfo & _information)
		{
			std::string fn = __FUNCTION__;
			std::string keyEncryption;

			if (!_password.getPasswordBuffer(keyEncryption))
			{
				m_log.print(fn + ": error - can't get password's data for building encryption key.");
				return logic::common::InternalStatus::Int_NotFound;
			}

			logic::common::InternalStatus status = internalEncode(_hFile, _cryptAlg, keyEncryption, _information);

			if (IntSuccess(status))
			{
				_information.usedMasterPassword = true;

				if (!writeInfo(_hFile, _information))
				{
					m_log.print(fn + ": error - can't write meta-encryption info into just encrypted file.");
					status = logic::common::InternalStatus::Int_CriticalError;
				}
			}

			return status;
		}

		common::InternalStatus FileEncoder::encode(const std::wstring& _filepath,
			common::CryptAlgorithm _cryptAlg,
			const std::string& _key,
			EncodingMetaInfo& _information)
		{
			std::string fn = __FUNCTION__;

			HANDLE hFile = INVALID_HANDLE_VALUE;
			logic::common::InternalStatus result = common::InternalStatus::Int_Success;

			if (acquireFile(_filepath, false, hFile))
			{
				result = encode(hFile, _cryptAlg, _key, _information);
				leaveFile(hFile);
			}
			else
			{
				m_log.print(fn + ": error - can't open the file.");
			}

			return result;
		}

		common::InternalStatus FileEncoder::encode(HANDLE _hFile, common::CryptAlgorithm _cryptAlg, const std::string& _key, EncodingMetaInfo& _information)
		{
			std::string fn = __FUNCTION__;

			if (_key.empty())
			{
				m_log.print(fn + ": error - empty encryption key.");
				return logic::common::InternalStatus::Int_NotFound;
			}

			logic::common::InternalStatus status = internalEncode(_hFile, _cryptAlg, _key, _information);

			if (IntSuccess(status))
			{
				_information.usedMasterPassword = false;

				if (!writeInfo(_hFile, _information))
				{
					m_log.print(fn + ": error - can't write meta-encryption info into just encrypted file.");
					return logic::common::InternalStatus::Int_CriticalError;
				}
			}

			return status;
		}

		logic::common::InternalStatus FileEncoder::internalEncode(HANDLE _hFile,
			/*common::Hasher _hasher,*/ common::CryptAlgorithm _cryptAlg,
			const std::string& _key,
			EncodingMetaInfo& _result)
		{
			std::string fn = __FUNCTION__;
			EncodingMetaInfo metaInfo = getMetaContainer();

			if (_key.empty()){
				m_log.print(fn + ": error - encryption key has zero size.");
				return logic::common::InternalStatus::Int_NotFound;
			}

			common::SymCryptor* encoder = m_cryptor.get(_cryptAlg);
			if (!encoder)
			{
				m_log.print(fn + ": critical error - encoder lost.");
				return logic::common::InternalStatus::Int_NotFound;
			}

			common::Hasher* sha256Hasher = m_cryptor.get(common::HashAlgorithm::Hash_Sha256);
			if (!sha256Hasher)
			{
				m_log.print(fn + ": critical error - have no sha256 hasher object.");
				return logic::common::InternalStatus::Int_NotFound;
			}

			LARGE_INTEGER liFileSize = { 0 };
			if (!GetFileSizeEx(_hFile, &liFileSize))
			{
				m_log.print(fn + ": error - file size is not found.");
				return logic::common::InternalStatus::Int_NotFound;
			}

			std::string keySha256Hash = sha256Hasher->getHash(_key.c_str(), _key.length());
			std::string sha256FileData, sha256EncodedFileData;

			auto encodedSuccess = encodeFileData(_hFile, encoder, sha256Hasher, _key, sha256FileData, sha256EncodedFileData);
			if (!encodedSuccess)
			{
				m_log.print(fn + ": critical error - data was not encoded!");
				return logic::common::InternalStatus::Int_CriticalError;
			}

			metaInfo.originalFileSize = liFileSize.QuadPart;
			metaInfo.encodingAlgorithm = _cryptAlg;
			fill_chars(metaInfo.originalChecksum, sha256FileData.c_str());
			fill_chars(metaInfo.keyChecksum, keySha256Hash.c_str());

			//
			// Return meta information.
			//
			_result = metaInfo;
			return logic::common::InternalStatus::Int_Success;
		}

		logic::common::InternalStatus FileEncoder::internalDecode(HANDLE _hFile,
			const std::string& _key,
			bool& _isCompromisedIntegrity,
			std::string& _decodedDataHash)
		{
			std::string fn = __FUNCTION__, encodedDataHash;
			EncodingMetaInfo metaInfo;

			if (_key.empty()){
				m_log.print(fn + ": error - encryption key has zero size.");
				return logic::common::InternalStatus::Int_NotFound;
			}

			if (!readInfo(_hFile, metaInfo)){
				m_log.print(fn + ": error - file has no meta info, can't read.");
				return logic::common::InternalStatus::Int_NotFound;
			}

			common::Hasher* sha256Hasher = m_cryptor.get(common::HashAlgorithm::Hash_Sha256);
			if (!sha256Hasher){
				m_log.print(fn + ": critical error - have no sha256 hasher object.");
				return logic::common::InternalStatus::Int_NotFound;
			}

			common::SymCryptor* encoder = m_cryptor.get(metaInfo.encodingAlgorithm);
			if (!encoder){
				m_log.print(fn + ": error - cryptography algorithm is not supported, alg number is " + std::to_string((int)metaInfo.encodingAlgorithm));
				return logic::common::InternalStatus::Int_UnknownType;
			}

			// Set last symbols to null because of security issues.
			SET_LAST_ZERO_CHAR(metaInfo.keyChecksum);
			SET_LAST_ZERO_CHAR(metaInfo.originalChecksum);

			std::string keySha256Hash = sha256Hasher->getHash(_key.c_str(), _key.length());

			if (std::string(metaInfo.keyChecksum) != keySha256Hash)
			{
				m_log.print(fn + ": error - just calculated key hash not equal to stored hash.");
				return logic::common::InternalStatus::Int_WrongEncryptionKey;
			}

			// Remove meta info before decryption.
			if (!removeMetaInfo(_hFile))
			{
				m_log.print(fn + ": error - can't remove meta info.");
				return logic::common::InternalStatus::Int_UnknownError;
			}

			bool decodedSuccess = decodeFileData(_hFile, encoder, sha256Hasher, _key, encodedDataHash, _decodedDataHash);
			if (!decodedSuccess)
			{
				m_log.print(fn + ": error - can't decode file body.");
				return logic::common::InternalStatus::Int_UnknownError;
			}

			_isCompromisedIntegrity = (_decodedDataHash != std::string(metaInfo.originalChecksum));
			return logic::common::InternalStatus::Int_Success;
		}

		logic::common::InternalStatus FileEncoder::decode(const std::wstring& _filepath, const common::MasterPassword& _password, bool& _dataCompromised)
		{
			std::string fn = __FUNCTION__, encryptionKey;
			HANDLE hFile = INVALID_HANDLE_VALUE;
			logic::common::InternalStatus result = common::InternalStatus::Int_UnknownError;

			if (acquireFile(_filepath, false, hFile))
			{
				result = decode(hFile, _password, _dataCompromised);
				leaveFile(hFile);
			}
			else
			{
				m_log.print(fn + ": error - can't open the file.");
			}

			return result;
		}


		logic::common::InternalStatus FileEncoder::decode(HANDLE _hFile, const common::MasterPassword& _password, bool& _dataCompromised)
		{
			std::string encryptionKey;
			if (!_password.getPasswordBuffer(encryptionKey))
			{
				m_log.print(std::string(__FUNCTION__) + ": error - can't get master password buffer.");
				return logic::common::InternalStatus::Int_NoMasterPassword;
			}

			return this->decode(_hFile, encryptionKey, _dataCompromised);
		}

		logic::common::InternalStatus FileEncoder::decode(const std::wstring& _filepath, const std::string& _key, bool& _dataCompromised)
		{
			std::string fn = __FUNCTION__;
			HANDLE hFile = INVALID_HANDLE_VALUE;
			logic::common::InternalStatus result = common::InternalStatus::Int_UnknownError;

			if (acquireFile(_filepath, false, hFile))
			{
				result = decode(hFile, _key, _dataCompromised);
				leaveFile(hFile);
			}
			else
			{
				m_log.print(fn + ": error - can't open the file.");
			}

			return result;
		}

		logic::common::InternalStatus FileEncoder::decode(HANDLE _hFile, const std::string& _key, bool& _dataCompromised)
		{
			std::string fn = __FUNCTION__, decodedDataHash;
			logic::common::InternalStatus result = common::InternalStatus::Int_UnknownError;

			result = internalDecode(_hFile, _key, _dataCompromised, decodedDataHash);

			if (IntSuccess(result))
			{
				//
				// Success. Data was decoded.
				//

				// Remove meta information.
				// (!) But meta info was removed in internalDecode() function.
			}
			else
			{
				m_log.print(fn + ": error - can't decode file, internal error is " + std::to_string((int)result));
			}

			return result;
		}

		bool FileEncoder::readInfo(const std::wstring& _filepath, EncodingMetaInfo& _result)
		{
			bool result = false;
			std::string fn = __FUNCTION__;
			HANDLE hFile = INVALID_HANDLE_VALUE;

			if (result = acquireFile(_filepath, false, hFile))
			{
				result = readInfo(hFile, _result);

				leaveFile(hFile);
			}
			else
			{
				m_log.print(fn + ": error - can't open the file.");
			}

			return result;
		}

		bool FileEncoder::readInfo(HANDLE _hFile, EncodingMetaInfo& _result)
		{
			bool result = false;
			std::string fn = __FUNCTION__;

			LARGE_INTEGER liFileSize, liNewPointer, toMoveFromEnd;
			if (GetFileSizeEx(_hFile, &liFileSize)) // A skipped it.
			{
				if (liFileSize.QuadPart > sizeof(EncodingMetaInfo))
				{
					toMoveFromEnd.QuadPart = (-1) * (long)(sizeof(EncodingMetaInfo));

					if (SetFilePointerEx(_hFile, toMoveFromEnd, &liNewPointer, FILE_END))
					{
						DWORD cbRead = 0;
						EncodingMetaInfo buffer;
						if (ReadFile(_hFile, &buffer, sizeof(buffer), &cbRead, NULL))
						{
							bool validHeader = (memcmp(buffer.headerSignature, FE_Signature, sizeof(FE_Signature)) == 0);
							bool validEnd = (memcmp(buffer.endSignature, FE_SignatureEnd, sizeof(FE_SignatureEnd)) == 0);

							if (validHeader && validEnd && (cbRead == sizeof(EncodingMetaInfo)))
							{
								_result = buffer;
								result = true;
							}
						}
					}
				}
				else
				{
					m_log.print(fn + ": error - there is no place for meta info, file size is " + std::to_string(liFileSize.QuadPart));
				}
			}
			else
			{
				m_log.print(fn + ": error - can't get file size.");
			}

			return result;
		}

		bool FileEncoder::writeInfo(const std::wstring& _filepath, const EncodingMetaInfo& _metaInfo)
		{
			bool result = false;
			std::string fn = __FUNCTION__;
			HANDLE hFile = INVALID_HANDLE_VALUE;

			if (result = acquireFile(_filepath, false, hFile))
			{
				result = writeInfo(hFile, _metaInfo);

				leaveFile(hFile);
			}
			else
			{
				m_log.print(fn + ": error - can't open the file.");
			}

			return result;
		}

		bool FileEncoder::writeInfo(HANDLE _hFile, const EncodingMetaInfo& _metaInfo)
		{
			bool result = false;
			std::string fn = __FUNCTION__;

			LARGE_INTEGER  liNewPointer = { 0 }, toMove = { 0 };
			toMove.QuadPart = 0;

			if (SetFilePointerEx(_hFile, toMove, &liNewPointer, FILE_END)) // Move pointer in the end.
			{
				DWORD cbWritten = 0;
				if (WriteFile(_hFile, &_metaInfo, sizeof(_metaInfo), &cbWritten, NULL))
				{
					if (cbWritten == sizeof(_metaInfo))
					{
						result = true;
					}
				}
			}

			return result;
		}

		bool FileEncoder::removeMetaInfo(const std::wstring& _filepath)
		{
			bool result = false;
			std::string fn = __FUNCTION__;
			HANDLE hFile = INVALID_HANDLE_VALUE;

			if(result = acquireFile(_filepath, false, hFile))
			{
				result = removeMetaInfo(hFile);
				leaveFile(hFile);
			}
			else
			{
				m_log.print(fn + ": error - can't open the file.");
			}

			return result;
		}

		bool FileEncoder::removeMetaInfo(HANDLE _hFile)
		{
			bool result = false;
			std::string fn = __FUNCTION__;

			LARGE_INTEGER liFileSize = { 0 }, liNewPointer = { 0 }, toMoveFromEnd = { 0 };
			if (GetFileSizeEx(_hFile, &liFileSize))
			{
				if (liFileSize.QuadPart > sizeof(EncodingMetaInfo))
				{
					toMoveFromEnd.QuadPart = (-1) * (long)(sizeof(EncodingMetaInfo));

					if (SetFilePointerEx(_hFile, toMoveFromEnd, &liNewPointer, FILE_END))
					{
						DWORD cbRead = 0;
						EncodingMetaInfo buffer;
						if (ReadFile(_hFile, &buffer, sizeof(buffer), &cbRead, NULL))
						{
							bool validHeader = (memcmp(buffer.headerSignature, FE_Signature, sizeof(FE_Signature)) == 0);
							bool validEnd = (memcmp(buffer.endSignature, FE_SignatureEnd, sizeof(FE_SignatureEnd)) == 0);

							if (validHeader && validEnd && (cbRead == sizeof(EncodingMetaInfo)))
							{
								//
								// Meta info was found and now we can remove it from file.
								//
								if (SetFilePointerEx(_hFile, liNewPointer, &liFileSize, FILE_BEGIN))
								{
									result = SetEndOfFile(_hFile) == TRUE;
								}
							}
						}
					}
				}
				else
				{
					m_log.print(fn + ": error - there is no place for meta info, file size is " + std::to_string(liFileSize.QuadPart));
				}
			}
			else
			{
				m_log.print(fn + ": error - can't get file size.");
			}

			return result;
		}

		bool FileEncoder::verifyMetaInfo(const std::wstring& _filepath)
		{
			bool result = false;
			std::string fn = __FUNCTION__;
			HANDLE hFile = INVALID_HANDLE_VALUE;

			if (result = acquireFile(_filepath, true, hFile))
			{
				result = verifyMetaInfo(hFile);
				leaveFile(hFile);
			}
			else
			{
				m_log.print(fn + ": error - can't open the file.");
			}

			return result;
		}

		bool FileEncoder::verifyMetaInfo(HANDLE _hFile)
		{
			bool result = false;
			std::string fn = __FUNCTION__;

			LARGE_INTEGER liFileSize = { 0 }, liNewPointer = { 0 }, toMoveFromEnd = { 0 };
			if (GetFileSizeEx(_hFile, &liFileSize))
			{
				if (liFileSize.QuadPart > sizeof(EncodingMetaInfo))
				{
					toMoveFromEnd.QuadPart = (-1) * (long)(sizeof(EncodingMetaInfo));

					if (SetFilePointerEx(_hFile, toMoveFromEnd, &liNewPointer, FILE_END))
					{
						DWORD cbRead = 0;
						EncodingMetaInfo buffer = { 0 };
						if (ReadFile(_hFile, &buffer, sizeof(buffer), &cbRead, NULL))
						{
							bool validHeader = (memcmp(buffer.headerSignature, FE_Signature, sizeof(FE_Signature)) == 0);
							bool validEnd = (memcmp(buffer.endSignature, FE_SignatureEnd, sizeof(FE_SignatureEnd)) == 0);

							if (validHeader && validEnd && (cbRead == sizeof(EncodingMetaInfo)))
							{
								result = true;
							}
						}
					}
				}
				else
				{
					m_log.print(fn + ": error - there is no place for meta info, file size is " + std::to_string(liFileSize.QuadPart));
				}
			}
			else
			{
				m_log.print(fn + ": error - can't get file size.");
			}

			return result;
		}

		logic::encryption::EncodingMetaInfo FileEncoder::getMetaContainer()
		{
			EncodingMetaInfo container = { 0 };

			memcpy(container.headerSignature, FE_Signature, sizeof(FE_Signature));
			memcpy(container.endSignature, FE_SignatureEnd, sizeof(FE_SignatureEnd));

			return container;
		}

		bool FileEncoder::encodeFileData(HANDLE _hFile,
			common::SymCryptor* _cryptor,
			common::Hasher* _hasher,
			const std::string& _encryptionKey,
			std::string& _strOriginalDataHash,
			std::string& _strEncodedDataHash)
		{
			std::string fn = __FUNCTION__;
			std::string totalFileDataHash, totalEncodedFileDataHash;

			LARGE_INTEGER liFileSize, liPos = { 0 };
			liFileSize.LowPart = 0;
			liFileSize.HighPart = 0;

			if (_encryptionKey.empty())
			{
				m_log.print(fn + ": error - empty encryption key.");
				return false;
			}

			if (!GetFileSizeEx(_hFile, &liFileSize))
			{
				m_log.print(fn + ": error - can't get file size.");
				return false;
			}

			// We can't map empty files into virtual memory.
			if (liFileSize.QuadPart == 0){
				m_log.print(fn + ": error - we can't with empty files.");
				return false;
			}

			// Start all from beginning.
			if (!windir::setFilePointerToBegin(_hFile)){
				return false;
			}

			HANDLE hMap = CreateFileMappingW(_hFile, NULL, PAGE_READWRITE, liFileSize.HighPart, liFileSize.LowPart, NULL);
			if (!hMap){
				m_log.print(fn + ": error - couldn't create file mapping object.");
				return false;
			}

			bool wellDone = true; // set false if an error happened in while loop.

			while (wellDone && (liFileSize.QuadPart > liPos.QuadPart) )
			{
				auto leftSize = liFileSize.QuadPart - liPos.QuadPart;

				SIZE_T workChunkSize = MinEncodingChunkSize;
				if (leftSize < MinEncodingChunkSize)
				{
					workChunkSize = (SIZE_T)leftSize;
				}

				PVOID pMappedTo = MapViewOfFile(hMap, (FILE_MAP_READ | FILE_MAP_WRITE), liPos.HighPart, liPos.LowPart, workChunkSize);
				if (pMappedTo)
				{
					// Calculate hash for original data (not encoded).
					// total hash is = get_hash( get_hash(chunk_64MB) + previously_calculated_hash)
					auto chunkHash = _hasher->getHash((const char*)pMappedTo, workChunkSize);
					auto newTotalHash = totalFileDataHash + chunkHash;
					totalFileDataHash = _hasher->getHash((const char*)newTotalHash.data(), newTotalHash.length());

					// Encrypt data.
					wellDone = _cryptor->encrypt((char*)pMappedTo, workChunkSize, _encryptionKey.data(), _encryptionKey.length());

					// Calculate hash for just encoded data.
					auto encodedChunkHash = _hasher->getHash((const char*)pMappedTo, workChunkSize);
					auto newEncodedTotalHash = totalEncodedFileDataHash + encodedChunkHash;
					totalEncodedFileDataHash = _hasher->getHash(newEncodedTotalHash.data(), newEncodedTotalHash.length());

					UnmapViewOfFile(pMappedTo);
				}
				else
				{
					// Failed to create mapping view of file.
					// ...
					m_log.print(fn + ": error - failed to create map view for size " + std::to_string(workChunkSize) + " in pos " + std::to_string(liPos.QuadPart));

					wellDone = false;
				}

				// Go to next iteration.
				liPos.QuadPart += workChunkSize;
			}

			if (wellDone)
			{
				_strEncodedDataHash = totalEncodedFileDataHash;
				_strOriginalDataHash = totalFileDataHash;
			}

			CloseHandle(hMap);
			return wellDone;
		}

		bool FileEncoder::decodeFileData(HANDLE _hFile,
			common::SymCryptor* _cryptor,
			common::Hasher* _hasher,
			const std::string& _encryptionKey,
			std::string& _strEncodedDataHash,
			std::string& _strDecodedDataHash)
		{
			std::string fn = __FUNCTION__;
			std::string totalEncodedFileDataHash, totalDecodedFileDataHash;

			LARGE_INTEGER liFileSize, liPos = { 0 };
			liFileSize.LowPart = 0;
			liFileSize.HighPart = 0;

			if (_encryptionKey.empty())
			{
				m_log.print(fn + ": error - empty encryption key.");
				return false;
			}

			if (!GetFileSizeEx(_hFile, &liFileSize)){
				m_log.print(fn + ": error - can't get file size.");
				return false;
			}

			// We can't map empty files into virtual memory.
			if (liFileSize.QuadPart == 0){
				m_log.print(fn + ": error - we can't with empty files.");
				return false;
			}

			// Start all from beginning.
			if (!windir::setFilePointerToBegin(_hFile)){
				m_log.print(fn + ": error - can't set file pointer into the end.");
				return false;
			}

			HANDLE hMap = CreateFileMappingW(_hFile, NULL, PAGE_READWRITE, liFileSize.HighPart, liFileSize.LowPart, NULL);
			if (!hMap){
				m_log.print(fn + ": error - couldn't create file mapping object.");
				return false;
			}

			bool wellDone = true; // set false if an error happened in while(..) loop.

			while (wellDone && (liFileSize.QuadPart > liPos.QuadPart))
			{
				auto leftSize = liFileSize.QuadPart - liPos.QuadPart;

				SIZE_T workChunkSize = MinEncodingChunkSize;
				if (leftSize < MinEncodingChunkSize)
				{
					workChunkSize = (SIZE_T)leftSize;
				}

				PVOID pMappedTo = MapViewOfFile(hMap, (FILE_MAP_READ | FILE_MAP_WRITE), liPos.HighPart, liPos.LowPart, workChunkSize);
				if (pMappedTo)
				{
					// Calculate hash for original data (not encoded).
					// total hash is = get_hash( get_hash(chunk_64MB) + previously_calculated_hash)
					auto chunkHash = _hasher->getHash((const char*)pMappedTo, workChunkSize);
					auto newTotalEncodedHash = totalEncodedFileDataHash + chunkHash;
					totalEncodedFileDataHash = _hasher->getHash((const char*)newTotalEncodedHash.data(), newTotalEncodedHash.length());

					wellDone = _cryptor->decrypt((char*)pMappedTo, workChunkSize, _encryptionKey.data(), _encryptionKey.length());

					// Calculate hash for just encoded data.
					auto decodedChunkHash = _hasher->getHash((const char*)pMappedTo, workChunkSize);
					auto newDecodedTotalHash = totalDecodedFileDataHash + decodedChunkHash;
					totalDecodedFileDataHash = _hasher->getHash(newDecodedTotalHash.data(), newDecodedTotalHash.length());

					UnmapViewOfFile(pMappedTo);
				}
				else
				{
					// Failed to create mapping view of file.
					// ...
					m_log.print(fn + ": error - failed to create map view for size " + std::to_string(workChunkSize) + " in pos " + std::to_string(liPos.QuadPart));

					wellDone = false;
				}

				// Go to next iteration.
				liPos.QuadPart += workChunkSize;
			}

			if (wellDone)
			{
				_strEncodedDataHash = totalEncodedFileDataHash;
				_strDecodedDataHash = totalDecodedFileDataHash;
			}

			CloseHandle(hMap);
			return wellDone;
		}

	}
}
