//
//	Author:
//			Burlutsky Stanislav
//			burluckij@gmail.com
//	
//	Copyright (C) NTTree Ltd. 2018. All Rights Reserved.
//


#pragma once

#include "DgiEngine.h"

#include "../../../service/DgiService/helpers/internal/helpers.h"
#include "../thrift_client/ThriftClientMain.h"
#include "../thrift_client/ThriftClientFLock.h"

namespace dguard
{
    static Engine* g_engine = NULL;
    std::mutex  g_engineLock;

    Engine& GetEngine()
    {
        std::unique_lock<std::mutex> lock(g_engineLock);

        if (!g_engine)
        {
            g_engine = new Engine();
        }

        return *g_engine;
    }

    std::string convertFlockState(::thrift_client::ClientFLock::FLockState _state)
    {
        switch (_state)
        {
        case ::thrift_client::ClientFLock::FLockState::FLock_Hidden:
            return "Hidden";

        case ::thrift_client::ClientFLock::FLockState::FLock_Locked:
            return "Locked";

        case ::thrift_client::ClientFLock::FLockState::FLock_HiddenAndLocked:
            return "Hidden and Locked";

        case ::thrift_client::ClientFLock::FLockState::FLock_Missed:
            return "Missed";

        case ::thrift_client::ClientFLock::FLockState::FLock_Unlocked:
            return "Unlocked";
        }

        return "Unknown";
    }

    ::thrift_client::ClientFLock::FLockState convertFLockStateToThrift(::dguard::FLockProtectionState /*std::string*/ _state)
    {
        switch (_state)
        {
        case ::dguard::FLockProtectionState::FLock_Hidden:
            return ::thrift_client::ClientFLock::FLockState::FLock_Hidden;

        case ::dguard::FLockProtectionState::FLock_Locked:
            return ::thrift_client::ClientFLock::FLockState::FLock_Locked;

        case ::dguard::FLockProtectionState::FLock_HiddenAndLocked:
            return ::thrift_client::ClientFLock::FLockState::FLock_HiddenAndLocked;

        case ::dguard::FLockProtectionState::FLock_Missed:
            return ::thrift_client::ClientFLock::FLockState::FLock_Missed;

        case ::dguard::FLockProtectionState::FLock_Unlocked:
            return ::thrift_client::ClientFLock::FLockState::FLock_Unlocked;
        }

        return ::thrift_client::ClientFLock::FLockState::FLock_UnknownState;
    }

    Engine::Engine():m_log("engine.log")
    {
        // ... 
    }

    logfile& Engine::getLog()
    {
        return m_log;
    }

    std::string Engine::login(std::wstring _masterPassword, bool& _mprIsSet, bool& _wrongPassword)
    {
        std::string fn = __FUNCTION__;
        std::string password = strings::ws_s(_masterPassword, CP_UTF8), sid;
        std::unique_lock<std::mutex> lock(m_lock);
        thrift_client::ClientMain client(THRIFT_SERVICE_ADDRESS, THRIFT_MAIN_PORT);

        if (client.VerifyIsPasswordSet())
        {
            if (client.login(password, sid))
            {
                //  Successfully logged!
                m_sid = sid;

                _mprIsSet = true;
                _wrongPassword = false;

                m_log.print(fn + "Successfully logged with password.");
            }
            else
            {
                if (!client.isRightPassword(password))
                {
                    m_log.print(fn + ": error - failed to login with incorrect password " + password);

                    _mprIsSet = true;
                    _wrongPassword = true;
                }
                else
                {
                    m_log.print(fn + ": error - failed to login by unknown reason.");
                }
            }
        }
        else
        {
            _mprIsSet = false;
        }

        return sid;
    }

    bool Engine::changeMasterPassword(std::wstring _currentPassword, std::wstring _newPassword)
    {
        std::string fn = __FUNCTION__;
        std::string utf8new = strings::ws_s(_newPassword, CP_UTF8);
        std::string utf8current = strings::ws_s(_currentPassword, CP_UTF8);

        std::unique_lock<std::mutex> lock(m_lock);
        thrift_client::ClientMain client(THRIFT_SERVICE_ADDRESS, THRIFT_MAIN_PORT);

        if (client.VerifyIsPasswordSet())
        {
            //
            //  Use already created SID or login with password.
            //

            std::string sid = m_sid;

            if (sid.empty())
            {
                m_log.print(fn + ": info - require to create new user session.");

                if (client.login(utf8current, sid))
                {
                    m_log.print(fn + ": success - new user session was created.");
                }
                else
                {
                    m_log.print(fn + ": error - failed to create new user session using provided password.");
                    return false;
                }
            }

            if (client.changeMasterPassword(sid, _currentPassword, _newPassword))
            {
                //
                //  The password was successfully changed.
                //

                m_log.print(fn + ": success - password was successfully changed.");

                //
                //  Do not update 'm_sid' here.
                //

                return true;
            }
            else
            {
                m_log.print(fn + ": error - failed to change password.");
            }
        }
        else
        {
            m_log.print(fn + ": error - master password was not set yet.");
        }

        return false;
    }

    bool Engine::isPasswordSet()
    {
        std::string fn = __FUNCTION__;
        std::unique_lock<std::mutex> lock(m_lock);
        thrift_client::ClientMain client(THRIFT_SERVICE_ADDRESS, THRIFT_MAIN_PORT);

        bool hasPass = client.VerifyIsPasswordSet();

        if (hasPass)
        {
            m_log.print(fn + ": info - master-password is set.");
        }
        else
        {
            m_log.print(fn + ": info - master-password is not set.");
        }

        return hasPass;
    }

    bool Engine::setMasterPassword(std::wstring _password)
    {
        std::string fn = __FUNCTION__;
        std::unique_lock<std::mutex> lock(m_lock);
        thrift_client::ClientMain client(THRIFT_SERVICE_ADDRESS, THRIFT_MAIN_PORT);

        if (client.VerifyIsPasswordSet())
        {
            m_log.print(fn + ": error - master-password is set.");
        }
        else
        {
            if (client.SetMasterPassword(_password))
            {
                m_log.print(fn + ": success - master-password is set.");

                return true;
            }
            else
            {
                m_log.print(fn + ": error - failed to set master-password.");
            }
        }

        return false;
    }

    std::string Engine::getFastTestSession()
    {
        std::string fn = __FUNCTION__;
        std::string password = "123", sid;
        std::unique_lock<std::mutex> lock(m_lock);
        thrift_client::ClientMain client(THRIFT_SERVICE_ADDRESS, THRIFT_MAIN_PORT);

        if (m_sid.empty())
        {
            if (client.VerifyIsPasswordSet())
            {
                if(client.login(password, sid))
                {
                    //  Successfully logged!
                    m_sid = sid;

                    m_log.print(fn + "Successfully logged with password: " + password);
                }
                else
                {
                    if (!client.isRightPassword(password))
                    {
                        m_log.print(fn + ": error - failed to login with incorrect password " + password);
                    }
                    else
                    {
                        m_log.print(fn + ": error - failed to login by unknown reason.");
                    }
                }
            }
            else
            {
                if (client.SetMasterPassword(strings::s_ws(password)))
                {
                    m_log.print(fn + ": success - master password was set successfully to " + password);

                    if (client.login(password, sid))
                    {
                        //  Successfully logged!
                        m_sid = sid;

                        m_log.print(fn + "Successfully logged with password: " + password);
                    }
                    else
                    {
                        if (!client.isRightPassword(password))
                        {
                            m_log.print(fn + ": error - failed to login with incorrect password " + password);
                        }
                        else
                        {
                            m_log.print(fn + ": error - failed to login by unknown reason.");
                        }
                    }
                }
                else
                {
                    m_log.print(fn + ": error - failed to set password to " + password);
                }
            }
        }

        return m_sid;
    }

    bool Engine::flockGetAll(std::vector<FLOCK_INFO> & _outFlocks)
    {
        std::string fn = __FUNCTION__;
        std::string sid = this->getFastTestSession();

        if (sid.empty())
        {
            m_log.print(fn + ": error - sid is not received.");
            return false;
        }

        thrift_client::ClientFLock flock(THRIFT_SERVICE_ADDRESS, THRIFT_FLOCK_PORT);
        thrift_client::ClientFLock::FLocks protectedFiles;

        bool success = flock.getFlocks(sid, protectedFiles);

        if (success)
        {
            for (auto f : protectedFiles)
            {
                FLOCK_INFO fi;
                fi.id = f.uniqueId;
                fi.filepath = f.filePath;
                fi.state = convertFlockState(f.state);
                fi.ftype = (f.type == thrift_client::ClientFLock::FLock_File ? FLockType_File : FLockType_Dir);

                _outFlocks.push_back(fi);
            }
        }
        else
        {
            m_log.print(fn + ": error - flocks were not received.");
        }

        return success;
    }

    bool Engine::flockDeleteAll()
    {
        std::string fn = __FUNCTION__;
        std::string sid = this->getFastTestSession();

        if (sid.empty())
        {
            m_log.print(fn + ": error - sid is not received.");
            return false;
        }

        thrift_client::ClientFLock flock(THRIFT_SERVICE_ADDRESS, THRIFT_FLOCK_PORT);
        thrift_client::ClientFLock::FLocks protectedFiles;

        bool success = flock.removeAll(sid);

        if (success)
        {
            m_log.print(fn + ": success - all flocks were removed.");
        }
        else
        {
            m_log.print(fn + ": error - flocks were not received.");
        }

        return success;
    }

    bool Engine::flockSetState(std::wstring _filepath, ::dguard::FLockProtectionState _newState)
    {
        std::string fn = __FUNCTION__;
        thrift_client::ClientFLock flock(THRIFT_SERVICE_ADDRESS, THRIFT_FLOCK_PORT);
        auto newStateThriftDefined = convertFLockStateToThrift(_newState);

        std::string sid = this->getFastTestSession();

        if (sid.empty())
        {
            m_log.print(fn + ": error - sid is not received.");
            return false;
        }

        bool success = flock.setState(sid, _filepath, newStateThriftDefined);

        if (success)
        {
            m_log.print(fn + ": success - changed state to flock.");
        }
        else
        {
            m_log.print(fn + ": error - failed to set new state to flock.");
        }

        return success;
    }

    bool Engine::flockSetStateById(std::string _flockId, ::dguard::FLockProtectionState _newState)
    {
        std::string fn = __FUNCTION__;
        thrift_client::ClientFLock flock(THRIFT_SERVICE_ADDRESS, THRIFT_FLOCK_PORT);
        auto newStateThriftDefined = convertFLockStateToThrift(_newState);

        std::string sid = this->getFastTestSession();

        if (sid.empty())
        {
            m_log.print(fn + ": error - sid is not received.");
            return false;
        }

        bool success = flock.setStateById(sid, _flockId, newStateThriftDefined);

        if (success)
        {
            m_log.print(fn + ": success - flock state was changed.");
        }
        else
        {
            m_log.print(fn + ": error - failed to set new flock state.");
        }

        return success;
    }

    bool Engine::flockIsSupportedFs(std::wstring _path, bool& _outResult)
    {
        thrift_client::ClientFLock flock(THRIFT_SERVICE_ADDRESS, THRIFT_FLOCK_PORT);

        return flock.isSupportedFs(_path, _outResult);
    }

    bool Engine::flockRemove(std::wstring _path)
    {
        std::string fn = __FUNCTION__;
        thrift_client::ClientFLock flock(THRIFT_SERVICE_ADDRESS, THRIFT_FLOCK_PORT);
        std::string sid = this->getFastTestSession();

        if (sid.empty())
        {
            m_log.print(fn + ": error - sid is not received.");
            return false;
        }

        bool success = flock.remove(sid, _path);

        if (success)
        {
            m_log.print(fn + ": success - flock removed.");
        }
        else
        {
            m_log.print(fn + ": error - failed to remove flock.");
        }

        return success;
    }

    bool Engine::flockAdd(std::wstring _path, std::string _flockId, ::dguard::FLockFileType _fileType, ::dguard::FLockProtectionState _newState)
    {
        std::string fn = __FUNCTION__;
        thrift_client::ClientFLock flock(THRIFT_SERVICE_ADDRESS, THRIFT_FLOCK_PORT);
        std::string sid = this->getFastTestSession();

        if (sid.empty())
        {
            m_log.print(fn + ": error - sid is not received.");
            return false;
        }

        ::thrift_client::ClientFLock::FLockObject newFlock;

        newFlock.filePath = _path;
        newFlock.uniqueId = _flockId;
        newFlock.type = (_fileType == FLockType_File ? ::thrift_client::ClientFLock::FLockObjectType::FLock_File : ::thrift_client::ClientFLock::FLockObjectType::FLock_Directory);
        newFlock.state = convertFLockStateToThrift(_newState);

        bool success = flock.add(sid, newFlock);

        if (success)
        {
            m_log.print(fn + ": success - flock was added.");
        }
        else
        {
            m_log.print(fn + ": error - failed to add flock.");
        }

        return success;
    }

}
