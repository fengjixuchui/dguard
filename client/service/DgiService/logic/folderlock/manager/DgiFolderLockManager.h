//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#pragma once

#include <mutex>
#include "../../../helpers/internal/log.h"
//#include "../type-convertors/TDgiBankingConvertor.h"
#include "../storage/DgiFolderLockStorage.h"
#include "../../common/master-password.h"
#include "../driver/ClientFLockNative.h"
#include "../../common/DgiCommonControl.h"

namespace logic
{
	namespace folderlock
	{
		namespace manager
		{
			//
			//	Controls access to files. 
			//

			class FileGuard : public ::logic::common::DgiCommonControl
			{
			public:
				FileGuard(std::wstring _logfilepath = L"flock.log");
				~FileGuard();

				//////////////////////////////////////////////////////////////////////////
				//
				//	That block of methods is responsible for manipulating with user-mode flock list.
				//

				//	Adds new File-system-Lock access policy.
				common::InternalStatus add(::logic::folderlock::storage::FLockObject& _flock);

				//	Returns information about flock's presence by its file system path.
				common::InternalStatus present(const std::wstring& _path);

				//	Returns information about flock's presence by 16 bytes unique id.
				common::InternalStatus presentById(const std::string& _flockId);

                common::InternalStatus getById(const std::string& _flockId /* AF098DC3100EACB23CDE59 */, ::logic::folderlock::storage::FLockObject& _outFlock);
				common::InternalStatus get(const std::wstring& _path /* x:\files\my.doc */, ::logic::folderlock::storage::FLockObject& _outFlock);
				common::InternalStatus getAll(std::vector<::logic::folderlock::storage::FLockObject>& _flocks);
				common::InternalStatus remove(const ::logic::folderlock::storage::FLockObject& _flock);
				common::InternalStatus remove(std::wstring _path /* c:\myown\private\files */);
				common::InternalStatus removeById(const std::string& _flockId /* AF098DC3100EACB23CDE59 */);
				common::InternalStatus changeState(std::wstring _path /* c:\myown\dx\salary.doc */, ::logic::folderlock::storage::FLockState _newState);
                common::InternalStatus changeStateById(const std::string& _flockId /* AF098DC3100EACB23CDE59 */, ::logic::folderlock::storage::FLockState _newState);
				common::InternalStatus clear();
				//////////////////////////////////////////////////////////////////////////

				//	Returns true if driver is accessible from our service.
				bool verifyDriverConnection();

				//
				//	This method sends information about all flocks from user-mode list do the driver.
				//	Driver has to receive that flocks and apply them in kernel.
				//
				//	(?) And also we should keep in mind that in that method we clear driver's list of flocks
				//	and replace it with user-mode copy list of flocks.
				//
				//	(!): We need to call that method each time after modifying list of flocks.
				//

				logic::common::InternalStatus sendFlocksToDriver(bool _refreshCache = true);

				//
				//	1. Gets flocks list from driver.
				//	2. Compare two lists - drivers vs user-mode service.
				//	3. Update driver's list only if lists are not equal.
				//

				logic::common::InternalStatus mergeFlocksWithDriver(bool _refreshCache = true);

				bool reloadStorage();

                //
                //  This call lead to:
                //
                //      1. Erases driver's internal cache;
                //      2. Invalids all volume's and files filter-contexts (That leads to refreshing all contexts).
                //
                bool refreshDriverCaches();

                //
                //  The method returns 'false' if an error occurred, otherwise it returns true and final result in  '_supported'.
                //
                bool isSupportedFs(std::wstring _path, bool& _supported);

				//
				//	Inherited interface - ::logic::common::DgiCommonControl.
				//
				
				virtual bool ctrInit() override;
				virtual bool ctrlLateInit() override;
				virtual bool ctrlIsRunning() override;
				virtual std::string ctrlGetName() override;
				virtual bool ctrlShutdown(bool _canWait) override;
				virtual bool ctrlSetPassword(::logic::common::MasterPassword _password) override;
				virtual bool ctrlEventPasswordChanged(::logic::common::MasterPassword _password, ::logic::common::MasterPassword _currentPassword) override;

			protected:
				DWORD getLowLevelPolicy(::logic::folderlock::storage::FLockState _policy);

				logic::common::InternalStatus markParentDirHasFLocks(std::wstring _path);

				bool isVolume(std::wstring _path);

			private:
				logfile m_log;
				//std::mutex m_lock;

				::driver::ClientFLock m_driver;
				::logic::folderlock::storage::Keeper m_storage;
			};
		}
	}
}
