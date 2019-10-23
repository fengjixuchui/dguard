//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#include <iostream>
#include "ServiceClient.h"
#include "thrift-client/ClientMain.h"
#include "thrift-client/ClientShredder.h"
#include "thrift-client/ClientCards.h"
#include "thrift-client/ClientEncryption.h"
#include "thrift-client/ClientFLock.h"
#include "..\DgiService\helpers\internal\log.h"
#include "../DgiService/helpers/internal/helpers.h"
#include "../DgiService/logic\folderlock\driver/ClientFLockNative.h"


#define MSG_MAIN_MENU		"\nEnter -help to see commands list\n"

#define MSG_COMMANDS		"Management:\tlogin, logout, has_password, set_password, is_password, login, logout, change_password\n"\
	"1. Cards:\tget_cards, get_card, add_card, delete_card\n"\
	"2. Erase:\terase_file, erase_dir\n"\
	"3. Encryption:\tis_encrypted, encrypt_file, decrypt_file, get_encryption_info, encrypt_file_async, decrypt_file_async\n"\
	"4. FLock:\tfl_add, fl_getall, fl_getstate, fl_setstate, fl_present, fl_presentid, fl_remove, fl_removeall, fl_cache, fl_supported\n"\
	"5. FLock-native:\t flock_info, flock_register, flock_unregister, flock_shutdown, flock_set_dbgprint, flock_present, flock_mark_file, flock_read_meta, flock_kill, "\
	"flock_storage_is_open, flock_storage_flush, flock_storage_clear, flock_storage_get, flock_storage_get_all, "\
	"flock_storage_add, flock_storage_remove, flock_storage_present, flock_storage_set_flag, \n"\
	"flock_cache_enable, flock_cache_clear, flock_cache_resize, flock_ctx_enable, flock_ctx_reset.\n"\
	"\n"\
	"-----------------------------------\n" // That's all.


logfile* gLogFlock = NULL;


void help()
{
	std::cout << std::endl << MSG_COMMANDS << std::endl;
}

void mainIsPasswordSet()
{
	thrift_client::ClientMain client(getServiceIp(), getMainServicePort());

	if (client.VerifyIsPasswordSet())
	{
		std::cout << "\tPassword is set.\n";
	}
	else
	{
		std::cout << "\twarning - master password is not set.\n";
	}
}

void mainSetPassword()
{
	thrift_client::ClientMain client(getServiceIp(), getMainServicePort());
	std::wstring newpassword;

	std::cout << "\nPlease enter new password: ";
	newpassword = readlineW();

	if ( client.SetMasterPassword(newpassword) )
	{
		std::cout << "\n\tPassword was set.\n";
	}
	else
	{
		std::cout << "\n\tError - master password password was not set.\n";
	}
}

void mainLogin()
{
	thrift_client::ClientMain client(getServiceIp(), getMainServicePort());
	std::string password, sid;

	std::cout << "\nPlease enter master-password for creating session: ";
	password = readline();

	if ( client.login(password, sid) )
	{
		std::cout << "\n\tSession created:" << sid << std::endl;
	}
	else
	{
		std::cout << "\n\tError - could not create session.\n";
	}
}

void mainLogout()
{
	thrift_client::ClientMain client(getServiceIp(), getMainServicePort());
	std::string sid;

	std::cout << "\nPlease enter session id: ";
	sid = readline();

	if ( client.logout(sid) )
	{
		std::cout << "\n\tSession created:" << sid << std::endl;
	}
	else
	{
		std::cout << "\n\tError - could not create session.\n";
	}
}

void mainIsRightPassword()
{
	thrift_client::ClientMain client(getServiceIp(), getMainServicePort());
	std::string password;

	std::cout << "\nPlease enter password: ";
	password = readline();

	if ( client.isRightPassword(password) )
	{
		std::cout << "\n\tPassword is correct." << std::endl;
	}
	else
	{
		std::cout << "\n\tInvalid password.\n";
	}
}

void mainChangePassword()
{
	thrift_client::ClientMain client(getServiceIp(), getMainServicePort());
	std::wstring password, newpassword;
	std::string sid;

	std::cout << "\nPlease enter sid: ";
	sid = readline();

	std::cout << "Please enter current password: ";
	password = readlineW();

	std::cout << "Please enter new password: ";
	newpassword = readlineW();

	if ( client.changeMasterPassword(sid, password, newpassword) )
	{
		std::cout << "\n\tSuccess." << std::endl;
	}
	else
	{
		std::cout << "\n\tFailed.\n";
	}
}

void cardAddCard()
{
	thrift_client::ClientCards client(getServiceIp(), getCardsServicePort());
	std::wstring bankOwner, shortDescription, paySystem, cardNumber, pinCode, cvvCode, holder;
	std::string sid;
	int vdMonth, vdYear;

	std::cout << "\nPlease enter sid: ";
	sid = readline();

	std::cout << "Card number: ";
	cardNumber = readlineW();

	std::cout << "Bank owner:";
	bankOwner = readlineW();

	std::cout << "Holder:";
	holder = readlineW();

	std::cout << "Short description:";
	shortDescription = readlineW();

	std::cout << "Pay system:";
	paySystem = readlineW();

	std::cout << "Pin:";
	pinCode = readlineW();

	std::cout << "CVV:";
	cvvCode = readlineW();

	std::cout << "Valid month:";
	std::cin >> vdMonth;

	std::cout << "Valid year:";
	std::cin >> vdYear;

	dgi::BankCard card;
	card.bankOwner = strings::ws_s(bankOwner, CP_UTF8);
	card.cvvCode = strings::ws_s(cvvCode, CP_UTF8);
	card.holder = strings::ws_s(holder, CP_UTF8);
	card.shortDescription = strings::ws_s(shortDescription, CP_UTF8);
	card.number = strings::ws_s(cardNumber, CP_UTF8);
	card.paySystem = strings::ws_s(paySystem, CP_UTF8);
	card.pinCode = strings::ws_s(pinCode, CP_UTF8);
	card.vd.vd_month = vdMonth;
	card.vd.vd_year = vdYear;

	if ( client.addCard(sid, card) )
	{
		std::cout << "\n\tSuccess." << std::endl;
	}
	else
	{
		std::cout << "\n\tFailed.\n";
	}
}

void cardGetCards()
{
	thrift_client::ClientCards client(getServiceIp(), getCardsServicePort());
	std::string sid;

	std::cout << "\nPlease enter sid: ";
	sid = readline();

	if ( client.printAllCards(sid) )
	{
		std::cout << "\n\tSuccess." << std::endl;
	}
	else
	{
		std::cout << "\n\tFailed.\n";
	}
}

void cardDelete()
{
	thrift_client::ClientCards client(getServiceIp(), getCardsServicePort());
	std::string sid, cardnumber;

	std::cout << "\nPlease enter sid: ";
	sid = readline();

	std::cout << "\nCard number: ";
	cardnumber = readline();

	if (  client.removeCardByNumber(sid, cardnumber)  )
	{
		std::cout << "\n\tSuccess." << std::endl;
	}
	else
	{
		std::cout << "\n\tFailed.\n";
	}
}

void cardGet()
{
	thrift_client::ClientCards client(getServiceIp(), getCardsServicePort());
	std::string sid, cardnumber;
	dgi::BankCard card;

	std::cout << "\nPlease enter sid: ";
	sid = readline();

	std::cout << "\nCard number: ";
	cardnumber = readline();

	if (client.getCard(sid, cardnumber, card))
	{
		std::cout << "\n\tSuccess." << std::endl;

		printf("\nCard details:\n\tNumber: %s \n\tDescription: %s\n\tBank owner: %s\n\tCard holder: %s\n\tPay system: %s\n\tPin: %s\n\tCVV: %s\n\tValid date: %d/%d\n",
			card.number.c_str(),
			card.shortDescription.c_str(),
			card.bankOwner.c_str(),
			card.holder.c_str(),
			card.paySystem.c_str(),
			card.pinCode.c_str(),
			card.cvvCode.c_str(),
			card.vd.vd_month,
			card.vd.vd_year);
	}
	else
	{
		std::cout << "\n\tFailed.\n";
	}
}

void eraseFile()
{
	thrift_client::ClientShredder client(getServiceIp(), getEraseServicePort());
	std::wstring path;

	std::cout << "\nPath: ";
	path = readlineW();

	if ( client.EraseFile(path) )
	{
		std::cout << "\n\tSuccess." << std::endl;
	}
	else
	{
		std::cout << "\n\tFailed.\n";
	}
}

void eraseDir()
{
	thrift_client::ClientShredder client(getServiceIp(), getEraseServicePort());
	dgi::AsyncResponse response;
	dgi::DgiResult result;
	std::wstring path;

	std::cout << "\nPath: ";
	path = readlineW();

	if ( client.EraseDirectory(path, response) )
	{
		std::cout << "\n\tWait until operation finished." << std::endl;

		while (client.GetAsyncState(response.asyncId, result))
		{
			std::cout << ".";

			if (result.status != dgi::DgiStatus::InProcess)
			{
				break;
			}
			
			Sleep(500);
		}

		std::cout << "\nOperation completed with code " << ((int) result.status) << std::endl;
	}
	else
	{
		std::cout << "\n\tFailed.\n";
	}
}

void encryptionIsEncrypted()
{
	thrift_client::ClientEncryption client(getServiceIp(), getEncryptionPort());
	std::wstring path;
	bool encryptedAlready = false;

	std::cout << "\nFile path to encryption: ";
	path = readlineW();

	if ( client.isEncoded(path, encryptedAlready) )
	{
		if (encryptedAlready)
		{
			std::cout << "\n\tFile is encrypted.\n";
		}
		else
		{
			std::cout << "\n\tFile was not encrypted.\n";
		}
	}
	else
	{
		std::cout << "\n\tError - could not read information.\n";
	}
}

void encryptionInfo()
{
	thrift_client::ClientEncryption client(getServiceIp(), getEncryptionPort());
	std::wstring path;
	dgi::ResponseFileInfo info;

	std::cout << "\nFile path: ";
	path = readlineW();

	if (client.getFileInfo(path, info)   )
	{
		if (info.encryptedAlready)
		{
			printf("\nInfo:\n\tState: %s\n\tEncryption key: %s\n\tOriginal file size: %I64d bytes\n\tEncoding algorithm: %d\n\tOriginal checksum: %s\n\tKey checksum: %s\n",
				(info.encryptedAlready ? "encrypted" : "not encrypted"),
				(info.info.usedMasterPassword ? "used master-password" : "custom user password"),
				info.info.originalFileSize,
				info.info.encodingAlgorithm,
				info.info.originalChecksum.c_str(),
				info.info.keyChecksum.c_str());
		}
		else
		{
			std::cout << "\n\tFile was not encrypted.\n";
		}
	}
	else
	{
		std::cout << "\n\tError - could not get information.\n";
	}
}

void encryptionEncrypt()
{
	thrift_client::ClientEncryption client(getServiceIp(), getEncryptionPort());
	std::wstring path, key;
	std::string ump, algorithm_name;
	bool useMasterPassword = false;

	std::cout << "\nFile path: ";
	path = readlineW();

	std::cout << "\nUse master password? (type yes): ";
	ump = readline();

	useMasterPassword = strings::equalStrings(ump, "yes");

	if (!useMasterPassword)
	{
		std::cout << "\nEncryption key: ";
		key = readlineW();
	}

	std::map< std::string, dgi::EncryptionAlgType::type > algs;
	algs["aes"] = dgi::EncryptionAlgType::EAlg_Aes;
	algs["aes256"] = dgi::EncryptionAlgType::EAlg_Aes256;
	algs["grader"] = dgi::EncryptionAlgType::EAlg_Grader;

	dgi::EncryptionAlgType::type alg = dgi::EncryptionAlgType::EAlg_Unknown;

	bool needAlgorithmName = true;
	while (needAlgorithmName)
	{
		std::cout << "\nPlease type encryption algorithm name? (aes, aes256, grader): ";
		algorithm_name = readline();

		for (auto i : algs){
			if (strings::equalStrings(i.first, algorithm_name)){
				needAlgorithmName = false;
				alg = i.second;
				break;
			}
		}

		if (needAlgorithmName){
			std::cout << "\nSorry, you entered unknown algorithm, try again..\n";
		}
	}


	if (client.encryptFile(path, strings::ws_s( key, CP_UTF8 ), useMasterPassword, alg))
	{
		std::cout << "\n\tSuccessfully encryption.\n";
	}
	else
	{
		std::cout << "\n\tError - file was not encryption.\n";
	}
}

void encryptionDecrypt()
{
	thrift_client::ClientEncryption client(getServiceIp(), getEncryptionPort());
	std::wstring path, key;
	std::string ump;
	bool useMasterPassword = false, ci = false;

	std::cout << "\nFile path: ";
	path = readlineW();

	std::cout << "\nUse master password? (type yes): ";
	ump = readline();

	useMasterPassword = strings::equalStrings(ump, "yes");

	if (!useMasterPassword)
	{
		std::cout << "\nDecryption key: ";
		key = readlineW();
	}

	if (client.decryptFile(path, strings::ws_s(key, CP_UTF8), useMasterPassword, ci))
	{
		if (!ci)
		{
			std::cout << "\n\tSuccessfully decrypted.\n";
		}
		else
		{
			std::cout << "\n\tFile was decrypted, but it's original integrity was compromised.\n";
		}
	}
	else
	{
		std::cout << "\n\tError - file was not decrypted.\n";
	}
}

void encryptionEncryptAsync()
{
	thrift_client::ClientEncryption client(getServiceIp(), getEncryptionPort());
	dgi::AsyncResponse response;
	std::wstring path, key;
	std::string ump, algorithm_name;
	bool useMasterPassword = false;
	dgi::DgiResult asyncResponseResult;

	std::cout << "\nFile path: ";
	path = readlineW();

	std::cout << "\nUse master password? (type yes): ";
	ump = readline();

	useMasterPassword = strings::equalStrings(ump, "yes");

	if (!useMasterPassword)
	{
		std::cout << "\nEncryption key: ";
		key = readlineW();
	}

	std::map< std::string, dgi::EncryptionAlgType::type > algs;
	algs["aes"] = dgi::EncryptionAlgType::EAlg_Aes;
	algs["aes256"] = dgi::EncryptionAlgType::EAlg_Aes256;
	algs["grader"] = dgi::EncryptionAlgType::EAlg_Grader;

	dgi::EncryptionAlgType::type alg = dgi::EncryptionAlgType::EAlg_Unknown;

	bool needAlgorithmName = true;
	while (needAlgorithmName)
	{
		std::cout << "\nPlease type encryption algorithm name? (aes, aes256, grader): ";
		algorithm_name = readline();

		for (auto i : algs){
			if (strings::equalStrings(i.first, algorithm_name)){
				needAlgorithmName = false;
				alg = i.second;
				break;
			}
		}

		if (needAlgorithmName){
			std::cout << "\nSorry, you entered unknown algorithm, try again..\n";
		}
	}

	if (client.encryptFileAsync(path, strings::ws_s(key, CP_UTF8), useMasterPassword, alg, response))
	{
		std::cout << "\n\tRequest for encryption was sent.";

		while (true)
		{
			std::cout << ".";

			if (client.getAsyncState(response.asyncId, asyncResponseResult))
			{
				if (asyncResponseResult.status != dgi::DgiStatus::InProcess)
				{
					std::cout << "\n\toperation completed with status: " << asyncResponseResult.status << std::endl;
					break;
				}
			}
			else
			{
				std::cout << "\nError - could not get async operation state, error number is " << asyncResponseResult.status << std::endl;
				break;
			}

			Sleep(500);
		}
	}
	else
	{
		std::cout << "\n\tError - file was not encrypted.\n";
	}
}

void encryptionDecryptAsync()
{
	thrift_client::ClientEncryption client(getServiceIp(), getEncryptionPort());
	dgi::AsyncResponse asyncResponse;
	dgi::DgiResult result;
	std::wstring path, key;
	std::string ump;
	bool useMasterPassword = false, ci = false;

	std::cout << "\nFile path: ";
	path = readlineW();

	std::cout << "\nUse master password? (type yes): ";
	ump = readline();

	useMasterPassword = strings::equalStrings(ump, "yes");

	if (!useMasterPassword)
	{
		std::cout << "\nDecryption key: ";
		key = readlineW();
	}

	if (client.decryptFileAsync(path, strings::ws_s(key, CP_UTF8), useMasterPassword, asyncResponse))
	{
		while (true)
		{
			std::cout << ".";

			if (client.getAsyncState(asyncResponse.asyncId, result))
			{
				if (result.status != dgi::DgiStatus::InProcess)
				{
					std::cout << "\n\tOperation completed with status: " << result.status << std::endl;
					break;
				}
			}
			else
			{
				std::cout << "\nError - could not get async operation state, error number is " << result.status << std::endl;
				break;
			}

			Sleep(500);
		}
	}
	else
	{
		std::cout << "\n\tError - file was not decrypted.\n";
	}
}

std::string getBool(bool _logic)
{
	return _logic ? "true" : "false";
}

void flockGetInfo()
{
	::driver::ClientFLock client(*gLogFlock);
	::driver::FLOCK_COMMON_INFO drvInfo = { 0 };

	if (client.getDriverInfo(drvInfo) )
	{
		std::cout << "\nDriver internal info: \n";
		std::cout << "------------------------------\n";

        std::cout << "\n\tDriver version: " << drvInfo.version;
		std::cout << "\n\tService process id: " << drvInfo.serviceProcessId;

		std::cout << "\n\tCache enabled (bool): " << getBool( drvInfo.cache.enabled);
		std::cout << "\n\tCache capacity: " << drvInfo.cache.capacity;
		std::cout << "\n\tCache occupancy limit: " << drvInfo.cache.occupancyLimit;
		std::cout << "\n\tCache current size: " << drvInfo.cache.currentSize;
		std::cout << "\n\tCache collision max resolve offset: " << drvInfo.cache.collisionMaxResolveOffset;
		std::cout << "\n\tCache collision resolve space border: " << drvInfo.cache.collisionResolveIfNoPlaceBorder;

		std::cout << "\n\n\tContext enabled (bool): " << getBool(drvInfo.ctxEnabled);
		std::cout << "\n\tContext time stamp: " << drvInfo.ctxLastStamp.stamp.QuadPart;

		std::cout << "\n\tFlocks count: " << drvInfo.flocksCount;
		std::cout << "\n\tTrace flags: " << drvInfo.traceFlags;
		std::cout << "\n\tStorage loaded (bool): " << getBool(drvInfo.storageLoaded);
		std::cout << "\n\tStop all (bool): " << getBool(drvInfo.stopAll);
		std::cout << "\n\tStorage loader finished (bool): " << getBool(drvInfo.storageLoaderFinished);
		std::cout << "\n\tProcess notification registered (bool): " << getBool(drvInfo.createProcessNotificatorRegistered);
		std::cout << "\n------------------------------\n";
	}
	else
	{
		std::cout << "\n\tError - could not get internal driver info.\n";
	}
}

void flockRegister()
{
	::driver::ClientFLock client(*gLogFlock);

	if (client.registerAsService())
	{
		std::cout << "\n\tSuccess - our process was registered as a managing service.\n";
	}
	else
	{
		std::cout << "\n\tError - could not register the service.\n";
	}
}

void flockUnregister()
{
	::driver::ClientFLock client(*gLogFlock);

	if (client.unregisterAsService())
	{
		std::cout << "\n\tSuccess - our process was unregistered as a managing service.\n";
	}
	else
	{
		std::cout << "\n\tError - could not unregister the service.\n";
	}
}

void flockPresent()
{
	::driver::ClientFLock client(*gLogFlock);
	std::wstring path;

	std::cout << "\nPlease enter file path to verify flock's meta: ";
	path = readlineW();

	if (client.verifyMeta(path))
	{
		std::cout << "\n\tFLock's meta successfully found!\n";
	}
	else
	{
		std::cout << "\n\tError - flock's attributes was not found.\n";
	}
}

void flockStorageClear()
{
	::driver::ClientFLock client(*gLogFlock);

	if (client.storageClear())
	{
		std::cout << "\n\tSuccess - driver flock storage was cleared.\n";
	}
	else
	{
		std::cout << "\n\tError - could not clear driver flock storage.\n";
	}
}

void flockStorageIsOpen()
{
	::driver::ClientFLock client(*gLogFlock);

	if (client.storageIsOpen())
	{
		std::cout << "\n\tSuccess - driver's flock storage opened.\n";
	}
	else
	{
		std::cout << "\n\tError - driver storage is not opened.\n";
	}
}

void flockMarkFile()
{
	::driver::ClientFLock client(*gLogFlock);
	::driver::FLOCK_META fm = { 0 };
	UCHAR signature[] = FLOCK_META_SIGNATURE;
	memcpy(fm.signature, signature, sizeof(signature));

	std::wstring path;

	std::cout << "\nPlease enter file path to verify flock's meta: ";
	path = readlineW();

	std::string flockId;

	std::cout << "\nPlease enter unique flock id (not longer then 16 symbols): ";
	flockId = readline();

	int lim = ((flockId.length() > 16) ? 16 : flockId.length());

	for (int i = 0; i < lim; ++i)
	{
		fm.uniqueId[i] = flockId.at(i);
	}

	if (client.writeFileMeta(path, fm))
	{
		std::cout << "\n\tSuccess - data was successfully written in file's EAs.\n";
	}
	else
	{
		std::cout << "\n\tError - could not mark file with FLock meta.\n";
	}
}

void flockKill()
{
	::driver::ClientFLock client(*gLogFlock);
	std::wstring path;

	std::cout << "\nPlease enter file path (\\??\\c:\\dir1\\f.txt) to make flock invalid: ";
	path = readlineW();

	if (client.makeMetaInvalid(path))
	{
		std::cout << "\n\tSuccess - FLock meta were removed.\n";
	}
	else
	{
		std::cout << "\n\tError - could not remove FLock meta.\n";
	}
}

void flockReadMeta()
{
	::driver::ClientFLock client(*gLogFlock);
	std::wstring path;
	::driver::FLOCK_META fm = { 0 };

	std::cout << "\nPlease enter file path to verify flock's meta: ";
	path = readlineW();

	if (client.readFileMeta(path, fm))
	{
		std::cout << "\n\tFLock's meta successfully found.\n\tUnique id:\t";

		for (int i = 0; i < sizeof(fm.uniqueId); ++i)
		{
			//printf("0x%x", fm.uniqueId[i]);

			std::cout << fm.uniqueId[i] << " ";
		}

		std::cout << "\n\n";
	}
	else
	{
		std::cout << "\n\tError - flock's attributes was not found.\n";
	}
}

void flockStorageGetAll()
{
	::driver::ClientFLock client(*gLogFlock);
	std::vector<::driver::FLOCK_STORAGE_ENTRY> flocks;

	if (client.storageGetAll(flocks))
	{
		std::cout << "\n\tSuccess - driver handled request, count flocks is " << flocks.size() << std::endl;

		int n = 0;
		for (auto flock : flocks)
		{
			n++;

			printf("\n%d.\tVersion: %d\n\tFlags: 0x%x\n\tId: ", n, flock.version, flock.flockFlag);

			for (int i = 0; i < sizeof(flock.id); ++i) {
				std::cout << std::hex << flock.id[i] << " ";
			}
		}
	}
	else
	{
		std::cout << "\n\tError - could not read flocks from driver.\n";
	}
}

void flockStorageGet()
{
	::driver::ClientFLock client(*gLogFlock);
	std::string flockId;
	::driver::FLOCK_STORAGE_ENTRY fse = { 0 };

	std::cout << "\nPlease enter unique flock id (not longer then 16 symbols): ";
	flockId = readline();

	while (flockId.size() > 16) {
		flockId.pop_back();
	}

	if (client.storageGetOne(flockId, fse))
	{
		std::cout << "\n\tSuccess - flock storage entry just read. Details are below: \n" << std::endl;
		
		printf("\tVersion: %d\n\tFlags: 0x%x\n\tId: ", fse.version, fse.flockFlag);

		for (int i = 0; i < sizeof(fse.id); ++i) {
			std::cout << std::hex << fse.id[i] << " ";
		}

		std::cout << "\n" << std::endl;
	}
	else
	{
		std::cout << "\n\tError - could not read flocks from driver.\n";
	}
}

void flockStorageFlush()
{
	::driver::ClientFLock client(*gLogFlock);

	if (client.storageFlush())
	{
		std::cout << "\n\tSuccess - driver flock storage was flushed.\n";
	}
	else
	{
		std::cout << "\n\tError - could not flush.\n";
	}
}

void flockStorageAdd()
{
	::driver::ClientFLock client(*gLogFlock);
	::driver::FLOCK_STORAGE_ENTRY fse = { 0 };
	std::string flockId;

	std::cout << "\nPlease enter flock id (not longer then 16 symbols): ";
	flockId = readline();

	int lim = ((flockId.length() > 16) ? 16 : flockId.length());

	for (int i = 0; i < lim; ++i) {
		fse.id[i] = flockId.at(i);
	}

	std::cout << "\nPlease enter flock state flags (it is a DWORD number): ";
	std::cin >> fse.flockFlag;

	if (client.storageAdd(fse))
	{
		std::cout << "\n\tSuccess - flock successfully added.\n";
	}
	else
	{
		std::cout << "\n\tError - flock was not added.\n";
	}
}

void flockStorageSetFlag()
{
	::driver::ClientFLock client(*gLogFlock);
	std::string flockId, strToSet;
	ULONG32 flags = 0;
	bool toSet = false;

	std::cout << "\nDo you want to set flag? (enter true for set, false otherwise): ";
	std::cin >> strToSet;
	toSet = strings::equalStrings(strToSet, "true");

	std::cout << "\nPlease enter flock id (not longer then 16 symbols): ";
	flockId = readline();

	std::cout << "\nPlease enter flock's flags (it is a DWORD number): ";
	std::cin >> flags;

	if (client.storageSetFlags(flockId, toSet, flags))
	{
		std::cout << "\n\tSuccess.\n";
	}
	else
	{
		std::cout << "\n\tError - did not set.\n";
	}
}

void flockStoragePresent()
{
	::driver::ClientFLock client(*gLogFlock);
	std::string flockId;

	std::cout << "\nPlease enter unique flock id (not longer then 16 symbols): ";
	flockId = readline();

	while (flockId.size() > 16) {
		flockId.pop_back();
	}

	if ( client.storagePresent(flockId) )
	{
		std::cout << "\n\tSuccess - flock is present.\n" << std::endl;
	}
	else
	{
		std::cout << "\n\tError - is not present.\n";
	}
}

void flockStorageRemove()
{
	::driver::ClientFLock client(*gLogFlock);
	std::string flockId;

	std::cout << "\nPlease enter unique flock id (not longer than 16 symbols): ";
	flockId = readline();

	while (flockId.size() > 16) {
		flockId.pop_back();
	}

	if (client.storageRemove(flockId))
	{
		std::cout << "\n\tSuccess - flock is removed.\n" << std::endl;
	}
	else
	{
		std::cout << "\n\tError - is not removed.\n";
	}
}

void flockSetDbgPrint()
{
	::driver::ClientFLock client(*gLogFlock);
	ULONG flags, oldFlags;

	std::cout << "\nPlease set debug flags (unsigned long 4 bytes value):";
	std::cin >> flags;

	if (client.setDbgPrintFlags(flags, oldFlags))
	{
		std::cout << "\n\tSuccess. Debug flags changed, old flags is " << std::hex << oldFlags << std::endl;
	}
	else
	{
		std::cout << "\n\tError - could not set.\n";
	}
}

void flockShutdown()
{
	::driver::ClientFLock client(*gLogFlock);

	std::cout << "\nRequest to stop the driver was sent...\n";

	if ( client.shutdown() )
	{
		std::cout << "\n\tSuccess. Driver was stopped." << std::endl;
	}
	else
	{
		std::cout << "\n\tError - could not stop the driver.\n";
	}
}

void flockCacheEnable()
{
	::driver::ClientFLock client(*gLogFlock);

	std::cout << "\nTo enable internal driver's cache enter 'enable':\n";
	std::string enable = readline();

	if (client.cacheEnable( strings::equalStrings(enable, "enable") ))
	{
		std::cout << "\n\tSuccess. Cache was cleared." << std::endl;
	}
	else
	{
		std::cout << "\n\tError - cache was not cleared.\n";
	}
}

void flockCacheClear()
{
	::driver::ClientFLock client(*gLogFlock);

	std::cout << "\nRequest to clear internal driver flock cache...\n";

	if (client.cacheClear())
	{
		std::cout << "\n\tSuccess. Cache was cleared." << std::endl;
	}
	else
	{
		std::cout << "\n\tError - cache was not cleared.\n";
	}
}

void flockContextEnable()
{
	::driver::ClientFLock client(*gLogFlock);

	std::cout << "\nRequest to enable context help - enter 'enable' or disable: ";
	std::string enable = readline();

	if (client.ctxEnable( strings::equalStrings(enable, "enable") ))
	{
		std::cout << "\n\tSuccess.\n" << std::endl;
	}
	else
	{
		std::cout << "\n\tError.\n";
	}
}

void flockContextReset()
{
	::driver::ClientFLock client(*gLogFlock);

	std::cout << "\nRequest to reset contexts.\n";

	if (client.ctxReset())
	{
		std::cout << "\n\tSuccess. Ctx - was updated." << std::endl;
	}
	else
	{
		std::cout << "\n\tError - ctx was not updated.\n";
	}
}

//////////////////////////////////////////////////////////////////////////
//	FLock-thrift client.
//////////////////////////////////////////////////////////////////////////

void flAdd()
{
	system("Color 0B");

	thrift_client::ClientFLock client(getServiceIp(), getFlockServicePort());
	thrift_client::ClientFLock::FLockObject flock;

	std::cout << "\nPlease enter flock path: ";
	flock.filePath = readlineW();

	std::cout << "Please enter flock state (0-unknown, 1-missed, 2-locked, 3-unlocked, 4-hidden, 5-hiddenAndLocked): ";
	int state = 0;
	std::cin >> state;

	std::cout << "Please enter flock type (0-unknown, 1-file, 2-directory, 3-disk): ";
	int flock_type = 0;
	std::cin >> flock_type;

	unsigned char idbuf[16] = { 0 };
	std::cout << "Please enter flock id (16bytes): ";
	std::string id = readline();

	for (int i = 0; i < sizeof(idbuf) && i < id.size(); ++i)
	{
		idbuf[i] = id.at(i);
	}

	flock.uniqueId = std::string((const char*)idbuf, sizeof(idbuf));

	if (!(flock_type >= 0 && flock_type <= 3))
	{
		std::cout << "\nYou entered wrong flock's type. It is not unsupported value.\n";
		return;
	}

	if (!(state >= 0 && state <= 5))
	{
		std::cout << "\nYou entered wrong flock's type. It is not unsupported value.\n";
		return;
	}

	flock.state = (thrift_client::ClientFLock::FLockState) state;
	flock.type = (thrift_client::ClientFLock::FLockObjectType) flock_type;

	if (client.add("", flock))
	{
		std::cout << "\nSuccess - new flock was added! Verify our protection ;-) \n";
	}
	else
	{
		std::cout << "\nError - Could not add file into protection area.\n";
	}
}

void flGetAll()
{
	system("Color 0B");

	thrift_client::ClientFLock client(getServiceIp(), getFlockServicePort());
	thrift_client::ClientFLock::FLocks flocks;

	if ( !client.getFlocks("", flocks) )
	{
		std::cout << "\nError - could not receive list of flocks.\n";
		return;
	}

	std::cout << "\nCount of flocks: " << flocks.size() << std::endl;

	int n = 0;
	for (auto flock : flocks)
	{
		std::cout << n <<  ". ------------------------------\n";
		std::cout << "\tId: " << flock.uniqueId << std::endl;
		std::wcout << "\tFile path: " << flock.filePath << std::endl;
		std::cout << "\tType: " << flock.type << std::endl;
		std::cout << "\tState: " << flock.state << std::endl;
		std::cout << "---------------------------------.\n";
		n++;
	}
}

void flGetState()
{
	system("Color 0B");

	thrift_client::ClientFLock client(getServiceIp(), getFlockServicePort());
	thrift_client::ClientFLock::FLockState state;

	std::cout << "\nPlease enter flock path: ";
	std::wstring flockPath = readlineW();

	if (client.getState("", flockPath, state))
	{
		std::cout << "\nFlock is " << state << " (0-unknown, 1-missed, 2-locked, 3-unlocked, 4-hidden, 5-hiddenAndLocked) " << std::endl;
	}
	else
	{
		std::cout << "\nError - information is not found.\n";
	}
}

void flSetState()
{
	system("Color 0B");

	thrift_client::ClientFLock client(getServiceIp(), getFlockServicePort());
	thrift_client::ClientFLock::FLocks flocks;

	std::cout << "\nPlease enter flock path: ";
	std::wstring flockPath = readlineW();
	std::cout << "Please enter new state (0-unknown, 1-missed, 2-locked, 3-unlocked, 4-hidden, 5-hiddenAndLocked): ";
	int state;
	std::cin >> state;

	if (!(state >= 0 && state <= 5))
	{
		std::cout << "\nError - you entered unsupported flock's state!\n";
		return;
	}

	if (client.setState("", flockPath, (thrift_client::ClientFLock::FLockState) state))
	{
		std::cout << "\nSuccess - state was changed.\n";
	}
	else
	{
		std::cout << "\nError - state was not changed.\n";
	}
}

void flPresent()
{
	system("Color 0B");

	thrift_client::ClientFLock client(getServiceIp(), getFlockServicePort());
	thrift_client::ClientFLock::FLocks flocks;

	std::cout << "\nPlease enter flock path: ";
	std::wstring flockPath = readlineW();

	if (client.present("", flockPath))
	{
		std::cout << "\nSuccess - flock is present.\n";
	}
	else
	{
		std::cout << "\nError - flock is not present.\n";
	}
}

void flPresentId()
{
	system("Color 0B");

	thrift_client::ClientFLock client(getServiceIp(), getFlockServicePort());
	thrift_client::ClientFLock::FLocks flocks;

	std::cout << "\nPlease enter flock's id (16bytes): ";
	std::string flockId = readline();

	if (client.presentId("", flockId))
	{
		std::cout << "\nSuccess - flock is present.\n";
	}
	else
	{
		std::cout << "\nError - flock is not present.\n";
	}
}

void flRemove()
{
	system("Color 0B");

	thrift_client::ClientFLock client(getServiceIp(), getFlockServicePort());
	thrift_client::ClientFLock::FLocks flocks;

	std::cout << "\nPlease enter flock's path: ";
	std::wstring path = readlineW();

	if (client.remove("", path))
	{
		std::cout << "\nSuccess - flock was removed.\n";
	}
	else
	{
		std::cout << "\nError - flock was not removed.\n";
	}
}

void flRemoveAll()
{
	system("Color 0B");

	thrift_client::ClientFLock client(getServiceIp(), getFlockServicePort());

	if (client.removeAll(""))
	{
		std::cout << "\nSuccess - flocks storage was cleared.\n";
	}
	else
	{
		std::cout << "\nError - could not clear flocks.\n";
	}
}

void flCache()
{
	system("Color 0B");

	thrift_client::ClientFLock client(getServiceIp(), getFlockServicePort());

	std::cout << "\n Not implemented yet. \n";
}

void flSupported()
{
    system("Color 0B");

    thrift_client::ClientFLock client(getServiceIp(), getFlockServicePort());

    std::cout << "\nPlease enter volume's path (for example, x:\\file\\f.txt): ";
    std::wstring path = readlineW();

    bool supported = false;
    if (client.isSupportedFs(path, supported))
    {
        if (supported)
        {
            std::cout << "\n Yes, target file system is supported.\n";
        }
        else
        {
            std::cout << "\n No, target file system is not supported.\n";
        }
    }
    else
    {
        std::cout << "\nAn error occurred while verification process. \n";
    }
}


//////////////////////////////////////////////////////////////////////////

void DataGuardConsole()
{
	gLogFlock = new logfile("Flock native client.log");

	typedef void(*pfnHandler)();
	typedef std::map<std::string, pfnHandler> handlers;

	initConsoleClient();

	handlers h;

	//	Managing service.
	h["-help"] = help;
	h["has_password"] = mainIsPasswordSet;
	h["set_password"] = mainSetPassword;
	h["login"] = mainLogin;
	h["logout"] = mainLogout;
	h["is_password"] = mainIsRightPassword;
	h["change_password"] = mainChangePassword;

	//	Bank cards thrift-defined service.
	h["get_cards"] = cardGetCards;
	h["add_card"] = cardAddCard;
	h["delete_card"] = cardDelete;
	h["get_card"] = cardGet;

	//	Secure erase thrift-defined service.
	h["erase_file"] = eraseFile;
	h["erase_dir"] = eraseDir;

	//	File-encryption thrift-defined service.
	h["is_encrypted"] = encryptionIsEncrypted;
	h["get_encryption_info"] = encryptionInfo;
	h["encrypt_file"] = encryptionEncrypt;
	h["decrypt_file"] = encryptionDecrypt;
	h["encrypt_file_async"] = encryptionEncryptAsync;
	h["decrypt_file_async"] = encryptionDecryptAsync;

	//	FLock thrift-defined service.
	h["fl_add"] = flAdd;
	h["fl_getall"] = flGetAll;
	h["fl_getstate"] = flGetState;
	h["fl_setstate"] = flSetState;
	h["fl_present"] = flPresent;
	h["fl_presentid"] = flPresentId;
	h["fl_remove"] = flRemove;
	h["fl_removeall"] = flRemoveAll;
	h["fl_cache"] = flCache;
    h["fl_supported"] = flSupported;

	//	FLock native client. This client works directly with kernel driver through IOCTL codes. 
	h["flock_info"] = flockGetInfo;
	h["flock_register"] = flockRegister;
	h["flock_unregister"] = flockUnregister;
	h["flock_present"] = flockPresent;
	h["flock_storage_clear"] = flockStorageClear;
	h["flock_storage_is_open"] = flockStorageIsOpen;
	h["flock_storage_get_all"] = flockStorageGetAll;
	h["flock_storage_get"] = flockStorageGet;
	h["flock_mark_file"] = flockMarkFile;
	h["flock_read_meta"] = flockReadMeta;
	h["flock_storage_flush"] = flockStorageFlush;
	h["flock_storage_add"] = flockStorageAdd;
	h["flock_storage_set_flag"] = flockStorageSetFlag;
	h["flock_storage_present"] = flockStoragePresent;
	h["flock_storage_remove"] = flockStorageRemove;
	h["flock_set_dbgprint"] = flockSetDbgPrint;
	h["flock_shutdown"] = flockShutdown;
	h["flock_kill"] = flockKill;

	//	Cache managing.
	h["flock_cache_clear"] = flockCacheClear;
	h["flock_cache_enable"] = flockCacheEnable;
	h["flock_cache_resize"] = flockCacheEnable;
	h["flock_cache_info"] = flockCacheEnable;

	//	Contexts optimization managing.
	h["flock_ctx_enable"] = flockContextEnable;
	h["flock_ctx_reset"] = flockContextReset;


	while (true)
	{
		system("Color 07");

		std::string action;
		std::cout << MSG_MAIN_MENU;

		std::cin >> action;

		if (h.count(action))
		{
			system("Color 02");

			h[action]();
		}
		else
		{
			std::cout << "\nYou entered unknown action. try again..\n\n";
		}
	}

	getchar();
}
