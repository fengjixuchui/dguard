//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//


#include "DgiTest.h"
#include "../../helpers/internal/helpers.h"
#include "../../logic/erasing/manager/ManagerSecureErase.h"
#include "../../helpers/encryption/cryptoHelpers.h"
#include "../../logic/encryption/files/manager/FileEncoder.h"


#define ERASE_DIRECTORY		L"D:\\work\\test"
#define ERASE_FILE			L"D:\\work\\erase\\file.txt"
#define ERASE_DRIVE			L""
#define ERASE_FILE_DRV		L"C:\\to_erase_secure.txt"

namespace tests
{
	bool testErase()
	{
		bool res1 = testEraseFile();
		bool res2 = testEraseDir();

		return res1 && res2;
	}

	bool testEraseDir()
	{
		bool testSuccess = true;
		printf("\n%s: This is a test for secure directory erasing.\n\n", __FUNCTION__);

		logic::secure_erase::manager::Shredder shredder;

		logic::common::EraseObject toErase;
		std::vector<logic::common::EraseObjectResult> errorsList;

		toErase.objectType = logic::common::EraseObjectType::EOT_Directory;
		toErase.path = ERASE_DIRECTORY;

		printf("\n%s: to erase - %s (%d type)\n", __FUNCTION__, strings::ws_s(toErase.path).c_str(), toErase.objectType);

		auto status = shredder.eraseOneObject(toErase, errorsList);

		printf("\n%s: erasing finished.\n", __FUNCTION__ );

		if (status != logic::common::InternalStatus::Int_Success)
		{
			testSuccess = false;
			printf("\n%s: an error occurred - can't erase the directory, internal status %d\n", __FUNCTION__, status);
		}

		if (!errorsList.empty())
		{
			testSuccess = false;

			printf("\n%s: there was some errors while erasing operation.\n", __FUNCTION__);

			for (auto an_error : errorsList)
			{
				printf("\n%s: %s (%d type) - internal error code is %d\n",
					__FUNCTION__,
					strings::ws_s(an_error.object.path).c_str(),
					an_error.object.objectType,
					an_error.result);
			}
		}

		if (!testSuccess)
		{
			printf("\n%s: (TEST_FAILED)\n", __FUNCTION__);
		}

		return false;
	}

	bool testEraseFile()
	{
		bool testSuccess = true;
		printf("\n%s: This is a test for secure file erasing.\n\n", __FUNCTION__);

		logic::secure_erase::manager::Shredder shredder;

		logic::common::EraseObject toErase;
		std::vector<logic::common::EraseObjectResult> errorsList;

		toErase.objectType = logic::common::EraseObjectType::EOT_File;
		toErase.path = ERASE_FILE;

		printf("\n%s: to erase - %s (%d type)\n", __FUNCTION__, strings::ws_s(toErase.path).c_str(), toErase.objectType);

		auto status = shredder.eraseOneObject(toErase, errorsList);

		printf("\n%s: erasing finished.\n", __FUNCTION__);

		if (status != logic::common::InternalStatus::Int_Success)
		{
			testSuccess = false;
			printf("\n%s: an error happend - can't erase the file, internal status %d\n", __FUNCTION__, status);
		}

		if (!errorsList.empty())
		{
			testSuccess = false;

			printf("\n%s: there was some errors while erasing operation.\n", __FUNCTION__);

			for (auto an_error : errorsList)
			{
				printf("\n%s: %s (%d type) - internal error code is %d\n",
					__FUNCTION__,
					strings::ws_s(an_error.object.path).c_str(),
					an_error.object.objectType,
					an_error.result);
			}
		}

		if (!testSuccess)
		{
			printf("\n%s: (TEST_FAILED)\n", __FUNCTION__);
		}

		return testSuccess;
	}

	bool testEraseFileThroughDriver()
	{
		bool testSuccess = true;
		auto fn = std::string(__FUNCTION__) + ": ";
		auto& log = ::logic::common::DgiEngine::getConf().getLog(L"tests_common.log");

		log.printEx("\n\n%s: This is a test for secure file erasing with erase driver help.\n\n", __FUNCTION__);

		logic::secure_erase::manager::Shredder shredder;

		auto eraseRes = shredder.eraseFile(ERASE_FILE_DRV);

		if (testSuccess = logic::common::IntSuccess(eraseRes) )
		{
			log.print(fn + " file was erased successfully.");
		}
		else
		{
			log.print(fn + " error - file was not erased, result code is " + std::to_string((int)eraseRes) );
		}

		return testSuccess;
	}

	bool test_MasterPasswordStorage()
	{
		auto fn = std::string(__FUNCTION__) + ": ";
		auto& log = ::logic::common::DgiEngine::getConf().getLog(L"tests_common.log");
		auto& password = ::logic::common::DgiEngine::getPassword();

		log.print(fn + "Begin tests with master-password.");

		if (!password.isStorageLoaded())
		{
			log.print(fn + "error - storage is not loaded.");
			return false;
		}

		log.print(fn + "Set master password to 'MyPassw0rd'");

		std::wstring strPassword = L"MyPassw0rd";
		::logic::common::MasterPassword mpr(strPassword);

		if ( password.setPassword(mpr) )
		{
			log.print(fn + "success - password successfully set.");
		}
		else
		{
			log.print(fn + "error - can't set the password.");
			return false;
		}

		log.print(fn + "verify equals of the passwords using mpr-hash-storage.");

		if (password.isThePassword(mpr))
		{
			log.print(fn + "success - the passwords are equals.");
		}
		else
		{
			log.print(fn + "error - the passwords are not equal. That is a big mistake.");
			return false;
		}

		log.print(fn + "hash of stored password - " + password.getHash());

		log.print(fn + "verify current master-password with wrong 'myPassw0rd'");

		std::wstring wrongPassword = L"myPassw0rd";
		if (password.isThePassword(wrongPassword))
		{
			log.print(fn + "error - wrong password matched.");
			return false;
		}
		else
		{
			log.print(fn + "success - wrong password is not equal to current master-password.");
		}

		//
		// Set new master-password.
		//

		log.print(fn + "Change master-password to new 'master-password'");

		std::wstring newPassword = L"master-password";

		if (password.setPassword(newPassword))
		{
			log.print(fn + "success - password successfully changed.");
		}
		else
		{
			log.print(fn + "error - can't change the password.");
			return false;
		}

		log.print(fn + "hash of stored password - " + password.getHash());

		log.print(fn + "compare current master-password with wrong password.");

		if (password.isThePassword(wrongPassword))
		{
			log.print(fn + "error - wrong password matched.");
			return false;
		}
		else
		{
			log.print(fn + "success - wrong password is not equal to current master-password.");
		}

		if (password.isThePassword(newPassword))
		{
			log.print(fn + "success - the passwords are equal.");
		}
		else
		{
			log.print(fn + "error - the passwords are not equal.");
			return false;
		}

		log.print(fn + "Complete - test finished.");
		return true;
	}

	bool test_MasterPasswordModifications()
	{
		auto fn = std::string(__FUNCTION__) + ": ";
		auto& log = ::logic::common::DgiEngine::getConf().getLog(L"tests_common.log");
		auto& passwordKeeper = ::logic::common::DgiEngine::getPassword();

		log.print(fn + "\n\nBegin tests with master-password.");

		if (!passwordKeeper.isStorageLoaded())
		{
			log.print(fn + "error - storage is not loaded.");
		}
		else
		{
			log.print(fn + "current master-password hash is " + passwordKeeper.getHash());
		}

		log.print(fn + "clear all cards in wallet");

		auto res = ::logic::common::DgiEngine::getWallet().clear();
		if (!::logic::common::IntSuccess(res))
		{
			log.print(fn + "error - wallet wasn't cleared.");
		}

		log.print(fn + "clear all flocks");

		res = ::logic::common::DgiEngine::getFlock().clear();
		if (!::logic::common::IntSuccess(res))
		{
			log.print(fn + "error - flocks wasn't cleared.");
		}

		log.print(fn + "Remove old master-password and set new to 'mypass'");

		std::wstring strPassword = L"mypass";
		::logic::common::MasterPassword mpr(strPassword);

		if (passwordKeeper.setPassword(mpr))
		{
			log.print(fn + "success - password successfully set.");
		}
		else
		{
			log.print(fn + "critical error - can't set the password.");
			return false;
		}

		log.print(fn + "current master-password hash is " + passwordKeeper.getHash());

		if(::logic::common::DgiEngine::initMasterPassword(mpr))
		{
			log.print(fn + "success - new master-password helped as to authenticate.");
		}
		else
		{
			log.print(fn + "critical error - can't init master-password.");
			return false;
		}


		auto& flock = ::logic::common::DgiEngine::getFlock();
		auto& wallet = ::logic::common::DgiEngine::getWallet();

		log.print(fn + "Add data to Wallet.");

		::logic::banking::storage::CardInfo card;

		card.validDate.vd_year = 2019;
		card.validDate.vd_month = 7;
		fill_chars(card.bankOwner, "tinkoff");
		fill_chars(card.cardHolder, "Egor Doshko");
		fill_chars(card.cardNumber, "4586 4210 4549 0056");
		fill_chars(card.cvv, "729");
		fill_chars(card.paySystem, "MasterCard");
		fill_chars(card.pin, "8594");
		fill_chars(card.shortDescription, "*My own bank card*");

		if (::logic::common::IntSuccess(wallet.addCard(card)))
		{
			log.print(fn + "success - card was added to Wallet.");
		}
		else
		{
			log.print(fn + "error - can't add new card to Wallet.");
			return false;
		}

		log.print(fn + "Add one card twice ");

		if ( !::logic::common::IntSuccess(wallet.addCard(card)) )
		{
			log.print(fn + "success - card was not added.");
		}
		else
		{
			log.print(fn + "error - card was added.");
			return false;
		}

		log.print(fn + "Wallet length is " + std::to_string(wallet.length()));

		fill_chars(card.cardNumber, "1111 2222 5412 4549");
		fill_chars(card.shortDescription, "* My brothers *");
		fill_chars(card.cardHolder, "* Tom Jefferson *");

		log.print(fn + "Add card to Wallet.");

		if (::logic::common::IntSuccess(wallet.addCard(card)))
		{
			log.print(fn + "success - card was added to Wallet.");
		}
		else
		{
			log.print(fn + "error - can't add new card to Wallet.");
			return false;
		}

		log.print(fn + "Wallet length is " + std::to_string(wallet.length()));

		//
		// Change master-password - it should affect on wallet and flock objects.
		//
		
		log.print(fn + "Change master password to 'newpass'");

		std::wstring newPassword = L"newpass";
		bool state = ::logic::common::DgiEngine::changePassword(strPassword, newPassword);

		if (state)
		{
			log.print(fn + "success - master-password is changed.");
		}
		else
		{
			log.print(fn + "critical error - can't change the master-password.");
			return false;
		}

		log.print(fn + "Wallet length is " + std::to_string(wallet.length()));

		std::vector<::logic::banking::storage::CardInfo> bankCards;
		if (!::logic::common::IntSuccess( wallet.getCards(bankCards) ))
		{
			log.print(fn + "critical error - can't read cards, storage is corrupted.");
			return false;
		}

		for (auto cardInfo : bankCards)
		{
			std::string description = std::string("card info: ") + cardInfo.shortDescription + " " + cardInfo.cardHolder + " " + cardInfo.cardNumber;

			log.print(fn + description);
		}

		log.print(fn + "Complete - test finished.\n\n");
		return true;
	}


	bool test_Aes()
	{
		char buffer[] = "This is a text.";

		printf("\n\n%s: before encryption - %s\n\n", __FUNCTION__, buffer);

		bool success = crypto::aes::encodeCfb128(buffer, sizeof(buffer), "-8-");

		if (success)
		{
			printf("\n\n%s:Success data is encrypted.\n\n", __FUNCTION__);
			printf("\n%s: Data after encryption - ", __FUNCTION__);

			for (auto i : buffer){
				printf("%c", i);
			}

			if (crypto::aes::decodeCfb128(buffer, sizeof buffer, "-8-"))
			{
				printf("\n%s: Decoded data is - ", __FUNCTION__);

				for (auto i : buffer){
					printf("%c", i);
				}
			}
			else
			{
				printf("\n%s: Decoding failed.\n", __FUNCTION__);

				return false;
			}

			return true;
		}
		else
		{
			printf("\n\n%s: Error.\n\n", __FUNCTION__);
		}

		return false;
	}

	bool test_FileEncoder()
	{
		auto fn = std::string(__FUNCTION__) + ": ";
		auto& log = ::logic::common::DgiEngine::getConf().getLog(L"tests_common.log");
		std::wstring filepath = L"D:\\work\\testfile.txt";

		::logic::encryption::FileEncoder encoder(log);

		::logic::encryption::EncodingMetaInfo info;
		auto status = encoder.encode(filepath, ::logic::common::CryptAlgorithm::CA_Aes, "-8-", info);

		if (::logic::common::IntSuccess(status))
		{
			bool dataCompromisedFlag;
			status = encoder.decode(filepath, "0-8-", dataCompromisedFlag);

			if (::logic::common::IntSuccess(status))
			{
				log.print(fn + ": success - file was decoded.");
				return true;
			}
			else
			{
				log.print(fn + ": error - file was not decoded.");
			}
		}
		else
		{
			log.print(fn + ": error - test failed. Can't encode the file.");
		}

		return false;
	}

    bool test_FLockChangeState()
    {
        bool testResult = false;
        auto fn = std::string(__FUNCTION__) + ": ";
        auto& log = ::logic::common::DgiEngine::getConf().getLog(L"tests_common.log");
        auto& passwordKeeper = ::logic::common::DgiEngine::getPassword();

        log.print(fn + "\n\nBegin tests with master-password.");

        if (!passwordKeeper.isStorageLoaded())
        {
            log.print(fn + "error - storage is not loaded.");
        }
        else
        {
            log.print(fn + "current master-password hash is " + passwordKeeper.getHash());
        }

        log.print(fn + "clear all flocks");

        auto res = ::logic::common::DgiEngine::getFlock().clear();
        if (!::logic::common::IntSuccess(res))
        {
            log.print(fn + "error - flocks wasn't cleared.");
        }

        log.print(fn + "Remove old master-password and set new to '123'");

        std::wstring strPassword = L"123";
        ::logic::common::MasterPassword mpr(strPassword);

        if (passwordKeeper.setPassword(mpr))
        {
            log.print(fn + "success - password successfully set.");
        }
        else
        {
            log.print(fn + "critical error - can't set the password.");
            return false;
        }

        log.print(fn + "current master-password hash is " + passwordKeeper.getHash());

        if (::logic::common::DgiEngine::initMasterPassword(mpr))
        {
            log.print(fn + "success - new master-password helped as to authenticate.");
        }
        else
        {
            log.print(fn + "critical error - can't init master-password.");
            return false;
        }


        auto& flock = ::logic::common::DgiEngine::getFlock();

        if (flock.reloadStorage())
        {
            log.print(fn + "flocks - were reloaded.");
        }
        else
        {
            log.print(fn + "critical error - flocks were not reloaded.");
            return false;
        }

        log.print(fn + "Start work with FLock.");

        std::vector<::logic::folderlock::storage::FLockObject> listOfFLocks;

        if (!IntSuccess(flock.getAll(listOfFLocks)))
        {
            log.print(fn + "error - failed to get all flocks from user-mode storage.");
            return false;
        }

        for (auto f : listOfFLocks)
        {
            std::string description = "File path: " + strings::ws_s(f.path) + ", protection: " + std::to_string((int)f.state) + " (Unknown 0, Missed 1, Locked 2, Unlocked 3, Hidden 4, HiddenAndLocked 5)";
            log.print(fn + ">> " + description);
        }

        WCHAR newFilePath[] = L"C:\\wspace\\data.txt";

        log.print(fn + "Add new flock - C:\\wspace\\data.txt.");

        ::logic::folderlock::storage::FLockObject newFlock = { 0 };
        fill_wchars(newFlock.path, newFilePath);
        newFlock.cbPathLength = sizeof(newFilePath);
        newFlock.state = logic::folderlock::storage::FLock_Locked;
        newFlock.type = logic::folderlock::storage::FLock_File;
        newFlock.uniqueId[0] = '1';
        newFlock.uniqueId[1] = '2';

        if (flock.add(newFlock))
        {
            log.print(fn + "success - new flock was added.");
        }
        else
        {
            log.print(fn + "warning - new flock was not added.");
        }

        if (flock.changeState(newFilePath, logic::folderlock::storage::FLock_Hidden))
        {
            log.print(fn + "success - state was changed to FLock_Hidden.");
        }
        else
        {
            log.print(fn + "error - state was not changed to FLock_Hidden.");
        }

        log.print(fn + "Reload storage data.");

        listOfFLocks.clear();

        if (!IntSuccess(flock.getAll(listOfFLocks)))
        {
            log.print(fn + "error - failed to get all flocks from user-mode storage.");
        }
        else
        {
            for (auto f : listOfFLocks)
            {
                std::string description = "File path: " + strings::ws_s(f.path) + ", protection: " + std::to_string((int)f.state) + " (Unknown 0, Missed 1, Locked 2, Unlocked 3, Hidden 4, HiddenAndLocked 5)";
                log.print(fn + ">> " + description);

                if (strings::equalStrings(std::wstring(f.path), newFilePath))
                {
                    testResult = f.state == logic::folderlock::storage::FLock_Hidden;
                }
            }
        }

        if (testResult)
        {
            log.print(fn + "FLock state was changed correctly.\n");
        }
        else
        {
            log.print(fn + "Error - FLock state was not changed.\n");
        }

        log.print(fn + "Complete - test finished.\n");
        return testResult;
    }

}
