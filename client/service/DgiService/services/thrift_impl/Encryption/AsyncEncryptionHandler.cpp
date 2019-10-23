//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#pragma once

#include "AsyncEncryptionHandler.h"
#include <memory>
#include <thread>
#include "../type-convertors/TDgiThriftTypesConverter.h"
#include "../type-convertors/TDgiEncryptionConvertor.h"

namespace service
{
	namespace thrift_impl
	{
		AsyncEncryptionDispatcher::AsyncEncryptionDispatcher(logic::encryption::FileEncoder& _encoder, AsyncRequestsKeeper& _requestsKeeper, logfile& _log):
			AsyncDispatcher(_requestsKeeper, _log),
			m_encoder(_encoder)
		{
			m_log.printEx("%s", __FUNCTION__);

			this->setActiveThreadsLimit(20);
		}

		AsyncEncryptionDispatcher::~AsyncEncryptionDispatcher()
		{
			m_log.printEx("%s", __FUNCTION__);

			ignoreNewRequests();

			// What until all threads completed.
			for (; getCountOfActiveHandlers() != 0;)
			{
				Sleep(10);
			}
		}

		logic::encryption::FileEncoder& AsyncEncryptionDispatcher::getEncoder() const
		{
			return m_encoder;
		}

		::dgi::AsyncResponse AsyncEncryptionDispatcher::perform_encryptFile(const ::dgi::RequestEncryptFile& _request)
		{
			m_log.print(std::string(__FUNCTION__));

			::dgi::AsyncResponse error;

			if (limitAchieved() && (!canPerformNewRequest()))
			{
				m_log.print(std::string(__FUNCTION__) + ": error - thread limit achieved or new requests marked as ignored.");
				error.result.status = dgi::DgiStatus::LimitAchieved;
				return error;
			}

			handlerStarted();

			try
			{
				::dgi::AsyncResponse result = createRequest();
				std::thread(AsyncEncryptionDispatcher::handler_encodeFile, result.asyncId, std::ref(*this), _request).detach();
				return result;
			}
			catch (...)
			{
				handlerCompleted();

				m_log.print(std::string(__FUNCTION__) + ": error - can't create thread, have no resources.");
				error.result.status = dgi::DgiStatus::LimitAchieved;
				return error;
			}
		}

		::dgi::AsyncResponse AsyncEncryptionDispatcher::perform_decodeFile(const ::dgi::RequestDecodeFile& _request)
		{
			m_log.print(std::string(__FUNCTION__));

			::dgi::AsyncResponse error;

			if (limitAchieved() && (!canPerformNewRequest()))
			{
				m_log.print(std::string(__FUNCTION__) + ": error - thread limit achieved or new requests marked as ignored.");
				error.result.status = dgi::DgiStatus::LimitAchieved;
				return error;
			}

			handlerStarted();

			try
			{
				::dgi::AsyncResponse result = createRequest();
				std::thread(AsyncEncryptionDispatcher::handler_decodeFile, result.asyncId, std::ref(*this), _request).detach();
				return result;
			}
			catch (...)
			{
				handlerCompleted();

				m_log.print(std::string(__FUNCTION__) + ": error - can't create thread, have no resources.");
				error.result.status = dgi::DgiStatus::LimitAchieved;
				return error;
			}
		}

		void AsyncEncryptionDispatcher::handler_decodeFile(dgi::AsyncId _requestId, AsyncEncryptionDispatcher& _dispatcherBkward, ::dgi::RequestDecodeFile _request)
		{
			std::string fn = __FUNCTION__;
			_dispatcherBkward.log().print(fn);

			std::wstring filepath = strings::s_ws(_request.filePath, CP_UTF8);
			::logic::encryption::FileEncoder fe(_dispatcherBkward.log());
			::logic::encryption::EncodingMetaInfo metaInfo;
			::logic::common::InternalStatus encodingStatus;
			bool integrityCompromised = false;

			if (!fe.readInfo(filepath, metaInfo))
			{
				_dispatcherBkward.log().print(fn + ": error - can't read meta info from file.");
				_dispatcherBkward.completeRequest(_requestId, ::dgi::DgiStatus::NotFound);
				return;
			}

			if (_request.useMasterPassword)
			{
				auto masterPassword = ::logic::common::DgiEngine::getPassword().getPassword();

				if (masterPassword.isSet())
				{
					encodingStatus = fe.decode(filepath, masterPassword, integrityCompromised);
				}
				else
				{
					_dispatcherBkward.log().print(fn + ": error - no master password for use.");
					_dispatcherBkward.completeRequest(_requestId, ::dgi::DgiStatus::NoMasterPassword);
					return;
				}
			}
			else
			{
				//
				// Here we decoding UTF-8 string into std::wstring and after that
				// we passes WIDE-chars as encryption key.
				//
				std::wstring encodingKey = strings::s_ws(_request.encryptionKey, CP_UTF8);
				std::string keyBuffer((const char*)encodingKey.data(), encodingKey.length() * sizeof(wchar_t));

				encodingStatus = fe.decode(filepath, keyBuffer, integrityCompromised);
			}

			auto finalRequestStatus = ::service::thrift_impl::toThrift(encodingStatus);

			if (IntSuccess(encodingStatus))
			{
				//
				// In case a file data was decoded successfully but the integrity was compromised,
				// signal about that through special status code, which is - DecodedButIntegrityCompromised.
				//
				finalRequestStatus = (integrityCompromised ? ::dgi::DgiStatus::DecodedButIntegrityCompromised : finalRequestStatus);
			}
			else
			{
				_dispatcherBkward.log().print(fn + ": error - file was not encoded, internal error code is " + std::to_string((int)encodingStatus));
			}

			//
			// Final request.
			//
			_dispatcherBkward.completeRequest(_requestId, finalRequestStatus);
		}

		void AsyncEncryptionDispatcher::handler_encodeFile(dgi::AsyncId _requestId, AsyncEncryptionDispatcher& _dispatcherBkward, ::dgi::RequestEncryptFile _request)
		{
			std::string fn = __FUNCTION__;
			_dispatcherBkward.log().print(fn);

			std::wstring filepath = strings::s_ws(_request.filePath, CP_UTF8);
			::logic::encryption::FileEncoder fe(_dispatcherBkward.log());
			::logic::encryption::EncodingMetaInfo metaInfo;
			::logic::common::InternalStatus encodingStatus;

			::logic::common::CryptAlgorithm internalCryptAlg;
			if (!fromThrift(_request.encodingAlgorithm, internalCryptAlg))
			{
				_dispatcherBkward.log().print(fn + ": error - can't convert thrift defined crypto alg to internal type.");
				_dispatcherBkward.completeRequest(_requestId, ::dgi::DgiStatus::TypeConvertionError);
				return;
			}

			if (_request.useMasterPassword)
			{
				auto masterPassword = ::logic::common::DgiEngine::getPassword().getPassword();

				if (masterPassword.isSet())
				{
					encodingStatus = fe.encode(filepath, internalCryptAlg, masterPassword, metaInfo);
				}
				else
				{
					_dispatcherBkward.log().print(fn + ": error - no master password for use.");
					_dispatcherBkward.completeRequest(_requestId, ::dgi::DgiStatus::NoMasterPassword);
					return;
				}
			}
			else
			{
				//
				// Here we decoding UTF-8 string into std::wstring and after that
				// we passes WIDE-chars as encryption key.
				//
				std::wstring encodingKey = strings::s_ws(_request.encryptionKey, CP_UTF8);
				std::string keyBuffer((const char*)encodingKey.data(), encodingKey.length() * sizeof(wchar_t));

				encodingStatus = fe.encode(filepath, internalCryptAlg, keyBuffer, metaInfo);
			}

			auto finalRequestStatus = ::service::thrift_impl::toThrift(encodingStatus);

			if (!IntSuccess(encodingStatus))
			{
				_dispatcherBkward.log().print(fn + ": error - file was not encoded, internal error code is " + std::to_string((int)encodingStatus));
			}

			//
			// Final request.
			//
			_dispatcherBkward.completeRequest(_requestId, finalRequestStatus);
		}
	}
}
