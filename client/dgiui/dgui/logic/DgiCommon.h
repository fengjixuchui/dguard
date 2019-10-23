//
//	Author:
//			Burlutsky Stanislav
//			burluckij@gmail.com
//	

#pragma once

#define TXT_FLOCK_STATUS_LOCKED             "Locked";
#define TXT_FLOCK_STATUS_HIDDEN             "Hidden";
#define TXT_FLOCK_STATUS_HIDDEN_LOCKED      "Hiden and Locked";
#define TXT_FLOCK_STATUS_UNLOCKED           "Unlocked";
#define TXT_FLOCK_STATUS_MISSED             "Missed";
#define TXT_FLOCK_STATUS_UNKNOWN            "Unknown";


#define FLOCK_ID_LENGTH         16

namespace dguard
{
    enum FLockProtectionState
    {
        FLock_UnknownState = 0,
        FLock_Missed = 1,
        FLock_Locked = 2,
        FLock_Unlocked = 3,
        FLock_Hidden = 4,
        FLock_HiddenAndLocked = 5
    };

    enum FLockFileType
    {
        FLockType_Unknown = 0,
        FLockType_File = 1,
        FLockType_Dir = 2
    };

    struct FLOCK_INFO
    {
        std::string id;
        std::wstring filepath;
        std::string state;
        FLockFileType ftype;
    };
}
