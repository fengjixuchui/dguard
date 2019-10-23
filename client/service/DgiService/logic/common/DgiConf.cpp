//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#include "DgiConf.h"
#include "../../helpers/internal/helpers.h"

namespace logic
{
	namespace common
	{

		DgiConf::DgiConf(std::wstring _rootdir) :
			m_rootdir(_rootdir),
			m_null("null.log"),
			m_logsKeeper(L"")
		{
			windir::removeLastSeparator(m_rootdir);

			// Associate logs directory with files keeper.
			//
			m_logsKeeper = whlp::FilesKeeper(getLogsDir());

			whlp::FilesKeeper(getSettingsDir());
		}

		DgiConf::~DgiConf()
		{
			// ...
		}

		std::wstring DgiConf::getLogsDir() const
		{
			return m_rootdir + L"\\" + DGI_TECHREP_DIR_L;
		}

		std::wstring DgiConf::getLogFile(std::wstring _filename) const
		{
			strings::toLower(_filename);

			return getLogsDir() + L"\\" + _filename;
		}

		logfile& DgiConf::getLog(std::wstring _filename)
		{
			std::unique_lock<std::mutex> lock(m_lock);

			strings::toLower(_filename);

			auto pos = m_logs.find(_filename);
			if (pos != m_logs.end())
			{
				return *pos->second.get();
			}
			else
			{
				// Need to perform because of compatibility issues.
				auto logfilepath = strings::ws_s(getLogsDir()) + "\\" + strings::ws_s(_filename);

				std::shared_ptr<logfile> log(new (std::nothrow) logfile(logfilepath));

				if (log.get() != nullptr)
				{
					m_logs[_filename] = log;

					return *log.get();
				}
			}

			// In case of errors returns null log.
			return m_null;
		}

		whlp::FilesKeeper DgiConf::getLogsKeeper() const
		{
			return whlp::FilesKeeper(getLogsDir());
		}

		std::wstring DgiConf::getSettingsDir() const
		{
			return m_rootdir + L"\\" + DGI_SETTINGS_DIR_L;
		}

		std::wstring DgiConf::getSettingsFile(std::wstring _filename) const
		{
			strings::toLower(_filename);

			return getSettingsDir() + L"\\" + _filename;
		}

		whlp::FilesKeeper DgiConf::getSettingsKeeper() const
		{
			return whlp::FilesKeeper(getSettingsDir());
		}

	}
}
