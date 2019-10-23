//
//	Author:
//			Burlutsky Stanislav
//			burluckij@gmail.com
//	
//	Copyright (C) NTTree Ltd. 2018. All Rights Reserved.
//

#pragma once

#include <mutex>
#include <string>
#include <vector>
#include "../../../service/DgiService/helpers/internal/log.h"

#include "DgiCommon.h"
#include "../thrift_client/ThriftCommon.h"


namespace dguard
{
    class Engine
    {
        //
        //  Creates test not-protected session with default password.
        //

    public:

        Engine();

        logfile& getLog();

        std::string login(std::wstring _masterPassword, bool& _mprIsSet, bool& _wrongPassword);

        bool changeMasterPassword(std::wstring _currentPassword, std::wstring _newPassword);

        bool isPasswordSet();

        bool setMasterPassword(std::wstring _password);

        std::string getFastTestSession();

        bool flockGetAll(std::vector<FLOCK_INFO> & _outFlocks);

        bool flockDeleteAll();

        bool flockSetState(std::wstring _filepath, ::dguard::FLockProtectionState _newState);

        bool flockSetStateById(std::string _flockId, ::dguard::FLockProtectionState _newState);

        bool flockIsSupportedFs(std::wstring _path, bool& _outResult);

        bool flockRemove(std::wstring _path);

        bool flockAdd(std::wstring _path, std::string _flockId, ::dguard::FLockFileType _fileType, ::dguard::FLockProtectionState _newState);

//         std::string getCurrentSid();
// 
//         void flush();
//         void close();

    private:
        std::mutex m_lock;
        std::string m_sid;

        logfile m_log;
    };

    Engine& GetEngine();


}
