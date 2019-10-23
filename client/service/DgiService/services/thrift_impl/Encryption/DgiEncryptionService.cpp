//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#include "DgiEncryptionService.h"
#include "../../../logic/session/DgiSession.h"
#include "../../../logic/common/DgiEngine.h"
#include "../../../logic/encryption/files/manager/FileEncoder.h"
#include "../type-convertors/TDgiEncryptionConvertor.h"
#include "../type-convertors/TDgiThriftTypesConverter.h"

namespace service
{
	namespace thrift_impl
	{
		DgiEncryptionService::DgiEncryptionService(std::string _logfile) :
			m_log(_logfile),
			m_fileEncoder(m_log),
			m_asyncDispatcher(m_fileEncoder, m_requestsKeeper, m_log)
		{
			// ...
		}

		DgiEncryptionService::~DgiEncryptionService()
		{
			// ...
		}

		void DgiEncryptionService::isFileEncoded(::dgi::BoolResponse& _return, const std::string& _filepath)
		{
			std::string fn = __FUNCTION__;
			m_log.print(fn);

			std::wstring filepath = strings::s_ws(_filepath, CP_UTF8);
			bool encoded = false;
			::logic::encryption::FileEncoder fe(m_log);

			auto status = fe.isEncoded(filepath, encoded);

			if (IntSuccess(status))
			{
				// Mark that file as encoded already.
				_return.bool_result = encoded;

				_return.errorResult.status = ::dgi::DgiStatus::Success;
			}
			else
			{
				m_log.print(fn + ": error - can't read meta info, may be file is not exist or locked.");
				_return.errorResult.status = ::dgi::DgiStatus::UnknownError;
			}
		}

		void DgiEncryptionService::getFileInfo(::dgi::ResponseFileInfo& _return, const std::string& _filepath)
		{
			std::string fn = __FUNCTION__;
			m_log.print(fn);

			std::wstring filepath = strings::s_ws(_filepath, CP_UTF8);
			bool encoded = false;
			::logic::encryption::FileEncoder fe(m_log);

			auto status = fe.isEncoded(filepath, encoded);

			if (IntSuccess(status))
			{
				if (encoded)
				{
					// Mark that file as encoded already.
					_return.encryptedAlready = true;

					logic::encryption::EncodingMetaInfo info;

					if (fe.readInfo(filepath, info))
					{
						//
						// All main work is going here.
						//

						if (toThrift(info, _return.info))
						{
							_return.result.status = ::dgi::DgiStatus::Success;
						}
						else
						{
							m_log.print(fn + ": error - can't convert types from internal to thrift defined.");
							_return.result.status = ::dgi::DgiStatus::TypeConvertionError;
						}
					}
					else
					{
						m_log.print(fn + ": error - can't read meta info, may be file is not exist or locked.");
						_return.result.status = ::dgi::DgiStatus::UnknownError;
					}
				}
				else
				{
					m_log.print(fn + ": warning - file was not encoded already.");
					_return.result.status = ::dgi::DgiStatus::Success;
					_return.encryptedAlready = false;
				}
			}
			else
			{
				m_log.print(fn + ": error - can't read meta info, may be file is not exist or locked.");
				_return.result.status = ::dgi::DgiStatus::UnknownError;
			}
		}

		void DgiEncryptionService::encryptFileAsync(::dgi::AsyncResponse& _return, const ::dgi::RequestEncryptFile& _request)
		{
			std::string fn = __FUNCTION__;
			m_log.print(fn);

			_return.result.status = dgi::DgiStatus::Success;

			if (_request.encryptionKey.empty())
			{
				m_log.print(fn + ": error - encryption key is empty!");
				_return.result.status = ::dgi::DgiStatus::WrongEncryptionKey;
				return;
			}

			//
			// Perform asynchronously.
			//
			_return = m_asyncDispatcher.perform_encryptFile(_request);
		}

		void DgiEncryptionService::encryptFile(::dgi::DgiResult& _return, const ::dgi::RequestEncryptFile& _file)
		{
			std::string fn = __FUNCTION__;
			m_log.print(fn);

			std::wstring filepath = strings::s_ws(_file.filePath, CP_UTF8);
			::logic::encryption::FileEncoder fe(m_log);
			::logic::encryption::EncodingMetaInfo metaInfo;
			::logic::common::InternalStatus encodingStatus;

			::logic::common::CryptAlgorithm internalCryptAlg;
			if (!fromThrift(_file.encodingAlgorithm, internalCryptAlg))
			{
				m_log.print(fn + ": error - can't convert thrift defined crypto alg to internal type.");
				_return.status = ::dgi::DgiStatus::TypeConvertionError;
				return;
			}

			if (_file.useMasterPassword)
			{
				auto masterPassword = ::logic::common::DgiEngine::getPassword().getPassword();

				if (masterPassword.isSet())
				{
					encodingStatus = fe.encode(filepath, internalCryptAlg, masterPassword, metaInfo);
				}
				else
				{
					m_log.print(fn + ": error - no master password for use.");
					_return.status = ::dgi::DgiStatus::NoMasterPassword;
					return;
				}
			}
			else
			{
				if (_file.encryptionKey.empty())
				{
					m_log.print(fn + ": error - encryption key is empty!");
					_return.status = ::dgi::DgiStatus::WrongEncryptionKey;
					return;
				}

				//
				// Here we decoding UTF-8 string into std::wstring and after that
				// we passes WIDE-chars as encryption key.
				//
				std::wstring encodingKey = strings::s_ws(_file.encryptionKey, CP_UTF8);
				std::string keyBuffer( (const char*)encodingKey.data(), encodingKey.length() * sizeof(wchar_t) );

				encodingStatus = fe.encode(filepath, internalCryptAlg, keyBuffer, metaInfo);
			}

			if (!IntSuccess(encodingStatus))
			{
				m_log.print(fn + ": error - file was not encoded, internal error code is " + std::to_string((int)encodingStatus));
			}

			//
			// Final result.
			//
			_return.status = toThrift(encodingStatus);
		}

		void DgiEncryptionService::getState(::dgi::DgiResult& _return, const ::dgi::AsyncId _asyncId)
		{
			std::string fn = __FUNCTION__;
			m_log.print(fn);

			//
			// Return only request state without data for response.
			//

			dgi::DgiStatus::type state;
			if (m_requestsKeeper.getState(_asyncId, state))
			{
				_return.status = state;

//				if (state != dgi::DgiStatus::InProcess)				
//				{
// 					// Ok, request handled and we can receive a response
// 					if (m_asyncDispatcher.getResponse_eraseFiles(_asyncId, _return)){
// 						// Remove response data. Second call of this method returns nothing.
// 						m_asyncDispatcher.deleteResponseData(_asyncId);
// 					} else {
// 						// Request is handled but response is absent.
// 						_return.result.status = ::dgi::DgiStatus::HaveNoResponse;
// 						m_log.print(std::string(__FUNCTION__) + ": error - request handled, but response is absent, async id " + std::to_string(_asyncId));
// 					}
// 				}
// 				else
// 				{
// 					// Still busy.
// 					_return.status = state;
// 				}
			}
			else
			{
				m_log.print(std::string(__FUNCTION__) + ": error - async id not registered " + std::to_string(_asyncId));
				_return.status = ::dgi::DgiStatus::NotFound;
			}
		}

		void DgiEncryptionService::decodeFile(::dgi::ResponseDecodeFile& _return, const ::dgi::RequestDecodeFile& _decodeRequest)
		{
			std::string fn = __FUNCTION__;
			m_log.print(fn);

			std::wstring filepath = strings::s_ws(_decodeRequest.filePath, CP_UTF8);
			::logic::encryption::FileEncoder fe(m_log);
			::logic::encryption::EncodingMetaInfo metaInfo;
			::logic::common::InternalStatus encodingStatus;
			bool integrityCompromised = false;

			if (!fe.readInfo(filepath, metaInfo))
			{
				m_log.print(fn + ": error - can't read meta info from file.");
				_return.result.status = ::dgi::DgiStatus::NotFound;
				return;
			}

			//::logic::common::CryptAlgorithm internalCryptAlg;
// 			if (!fromThrift(metaInfo.encodingAlgorithm, internalCryptAlg))
// 			{
// 				m_log.print(fn + ": error - can't convert thrift defined crypto alg to internal type.");
// 				_return.status = ::dgi::DgiStatus::TypeConvertionError;
// 				return;
// 			}

			if (_decodeRequest.useMasterPassword)
			{
				auto masterPassword = ::logic::common::DgiEngine::getPassword().getPassword();

				if (masterPassword.isSet())
				{
					encodingStatus = fe.decode(filepath, masterPassword, integrityCompromised);
				}
				else
				{
					m_log.print(fn + ": error - no master password for use.");
					_return.result.status = ::dgi::DgiStatus::NoMasterPassword;
					return;
				}
			}
			else
			{
				if (_decodeRequest.encryptionKey.empty())
				{
					m_log.print(fn + ": error - encryption key is empty!");
					_return.result.status = ::dgi::DgiStatus::WrongEncryptionKey;
					return;
				}

				//
				// Here we decoding UTF-8 string into std::wstring and after that
				// we passes WIDE-chars as encryption key.
				//
				std::wstring encodingKey = strings::s_ws(_decodeRequest.encryptionKey, CP_UTF8);
				std::string keyBuffer((const char*)encodingKey.data(), encodingKey.length() * sizeof(wchar_t));

				encodingStatus = fe.decode(filepath, keyBuffer, integrityCompromised);
			}

			if (!IntSuccess(encodingStatus))
			{
				m_log.print(fn + ": error - file was not encoded, internal error code is " + std::to_string((int)encodingStatus));
			}

			//
			// Final result.
			//
			_return.integrityCompromised = integrityCompromised;
			_return.result.status = toThrift(encodingStatus);
		}

		void DgiEncryptionService::decodeFileAsync(::dgi::AsyncResponse& _return, const ::dgi::RequestDecodeFile& _request)
		{
			std::string fn = __FUNCTION__;
			m_log.print(fn);

			_return.result.status = dgi::DgiStatus::Success;

			if (_request.encryptionKey.empty())
			{
				m_log.print(fn + ": error - encryption key is empty!");
				_return.result.status = ::dgi::DgiStatus::WrongEncryptionKey;
				return;
			}

			//
			// Perform asynchronously.
			//
			_return = m_asyncDispatcher.perform_decodeFile(_request);
		}

	}
}
