//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#pragma once

#include <map>
#include <boost/noncopyable.hpp>
#include "../../../helpers/containers/file/tstorage.h"
#include "../../../logic/common/DgiCommon.h"
#include "../../../helpers/internal/log.h"
#include "../../../logic/common/DgiCrypt.h"
#include "../../common/master-password.h"

namespace logic
{
	namespace banking
	{
		namespace storage
		{
			typedef char utf8_char;

			struct CardValidDate
			{
				unsigned long vd_month;
				unsigned long vd_year;
			};

			struct CardInfo
			{
				::logic::common::Sfci sfci;

				//
				//  Main and logical part.
				//
				//
				//  * All utf8 buffers should contain last zero-symbol - buffer[511] = 0.
				//

                utf8_char name[512];
				utf8_char shortDescription[512]; 
				utf8_char bankOwner[512];
				utf8_char paySystem[512];
				utf8_char cardNumber[128];
				utf8_char cardHolder[256];
				utf8_char pin[64];
				utf8_char cvv[64];

				CardValidDate validDate;
			};

			struct CardEntry
			{
				common::EntryProtect protection;
				CardInfo info;
			};

			//std::string getEncodedPassword(const ::logic::banking::storage::CardProtect& _protection);

			//
			// Storage implementation.
			//

			class Selector{
			public:
				bool operator()(const CardEntry& _entry) const{
					return false;
				}
			};

			typedef TStorage<CardEntry, Selector> CardsStorage;
			typedef std::vector<CardEntry> CardEntryList;
			typedef std::vector<std::string /* card number */, CardInfo> CardList;
			typedef std::map< std::string /* card number */, CardEntry> CardEntryMap;

			class CardsKeeper : private boost::noncopyable
			{
			public:
				CardsKeeper(::logic::common::DgiCrypt& _dgiCrypt, logfile& _log, std::string _storageFile = "cards.storage");
				~CardsKeeper();

				bool add(const CardInfo& _card);
				bool present(std::string _cardNumber);
				bool get(std::string _cardNumber, CardInfo& _cardInfo);
				bool getAll(std::vector<CardInfo>& _cards);
				void remove(const CardInfo& _card);
				void remove(std::string _cardNumber);
				unsigned long length() const;
				bool clear();

				bool setPassword(::logic::common::MasterPassword _password);

				//
				// Re-Encrypt data with new master-password.
				//
				bool reEncrypt(::logic::common::MasterPassword _newPassword, ::logic::common::MasterPassword _currenttPassword);

			private:

				//
				// Verifies connection with storage file - Is it loaded or not? 
				//
				bool isOk();

				bool isPresent(std::string _cardNumber);
				bool take(std::string _cardNumber, CardEntry& _outCard);
				std::string perform(const CardEntry& _card);
				std::string perform(std::string _cardNumber);

				bool decodeEntry(const CardEntry& _entry, CardInfo& _resultInfo, ::logic::common::MasterPassword _password);
				bool encodeEntry(const CardInfo& _cardInfo, CardEntry& _resultEntry, ::logic::common::HashAlgorithm _hash, ::logic::common::CryptAlgorithm _crypt, ::logic::common::MasterPassword _password);

				bool decodeEntry(const CardEntry& _entry, CardInfo& _resultInfo);
				bool encodeEntry(const CardInfo& _cardInfo, CardEntry& _resultEntry, ::logic::common::HashAlgorithm _hash, ::logic::common::CryptAlgorithm _crypt);

				bool loadCards();
				bool flushCards();

				void logMessage(const std::string& _msg);

				//
				// Master-password.
				//
				::logic::common::MasterPassword m_masterPassword;

				CardEntryMap m_entries;
				mutable std::mutex m_lockCards;
				logfile& m_log;

				// Component which does all necessary encryption.
				::logic::common::DgiCrypt& m_crypt;

				// What we use by-default if no other choices.
				::logic::common::HashAlgorithm m_defaultHash;
				::logic::common::CryptAlgorithm m_defaultCrypt;
				
				//
				// Low-level file storage.
				//
				std::string m_storageFile;
				CardsStorage m_storage;
			};
		}
	}
}
