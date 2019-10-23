//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#pragma once

#include <set>
#include <map>
#include <mutex>
#include <memory>

#include "../../helpers/internal/log.h"
#include "../../helpers/keeper/filesKeeper.h"
#include "DgiCommon.h"

#define DGI_SETTINGS_DIR_L        L"dgisettings"
#define DGI_TECHREP_DIR_L         L"dgitechrep"

namespace logic
{
	namespace common
	{
		class DgiConf
		{
		public:

			DgiConf(std::wstring _rootdir);
			~DgiConf();

			//
			// Logs staff.
			//

			std::wstring getLogsDir() const;
			std::wstring getLogFile(std::wstring _filename) const;
			logfile& getLog(std::wstring _filename);
			whlp::FilesKeeper getLogsKeeper() const;

			//
			// Settings staff.
			//
			std::wstring getSettingsDir() const;
			std::wstring getSettingsFile(std::wstring _filename) const;
			whlp::FilesKeeper getSettingsKeeper() const;


		protected:

		private:
			std::wstring m_rootdir;

			whlp::FilesKeeper m_logsKeeper;			
			logfile m_null;
			std::map<std::wstring /*  */, std::shared_ptr<logfile> > m_logs;

			mutable std::mutex m_lock;
		};
	}
}
