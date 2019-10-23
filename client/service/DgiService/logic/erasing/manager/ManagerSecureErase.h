//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#pragma once

#include <mutex>
#include <vector>
#include <boost/noncopyable.hpp>
#include "../../common/DgiCommon.h"
#include "../../../helpers/internal/log.h"
#include "../../../helpers/internal/helpers.h"
#include "../../../../../drivers/FileErase/SziFileEraseLib/SziFileEraseDriver.h"
#include "../../common/master-password.h"
#include "../../common/DgiCommonControl.h"
//#include "../../common/DgiEngine.h"


namespace logic
{
	namespace secure_erase
	{
		namespace manager
		{
			using namespace ::logic::common;

			//
			//	Removes files and it's data without an ability to have an opportunity for future recovering.
			//

			class Shredder : 
				public boost::noncopyable,
				public ::logic::common::DgiCommonControl
			{

			public:
				Shredder(std::wstring _logpath = L"shredder.log");

				~Shredder();

				//
				//	Erase secure list of files and saves results in '_outResultList' argument.
				//	Common status returns through return value.
				//

				common::InternalStatus eraseList(const std::vector<EraseObject>& _toEraseList, std::vector<EraseObjectResult>& _outResultList);

				//
				//	_eraseObject - if it's a directory of disk it means all included files will be removed.
				//

				common::InternalStatus eraseOneObject(common::EraseObject _eraseObject, std::vector<EraseObjectResult>& _outResultList);

				common::InternalStatus eraseFile(std::wstring _filePath);

				//
				//	Inherited interface - ::logic::common::DgiCommonControl.
				//

				virtual bool ctrInit() override;
				virtual bool ctrlLateInit() override;
				virtual bool ctrlIsRunning() override;
				virtual bool ctrlShutdown(bool _canWait) override;
				virtual std::string ctrlGetName() override;
				virtual bool ctrlSetPassword(::logic::common::MasterPassword _password) override;
				virtual bool ctrlEventPasswordChanged(::logic::common::MasterPassword _password, ::logic::common::MasterPassword _currentPassword) override;

			protected:

				//
				//	Calls driver to delete data secure.
				//

				common::InternalStatus secureEraseFile(std::wstring _filePath);

				//
				//	Removes all files in directory. 
				//

				void eraseDirHierarchy(const windir::DirEntry& _dir, std::vector<EraseObjectResult>& _outResultList);

				bool configureDriver();

			private:

				logfile m_log;
				std::mutex m_lock;
				bool m_driverConfigured;
			};
		}
	}
}
