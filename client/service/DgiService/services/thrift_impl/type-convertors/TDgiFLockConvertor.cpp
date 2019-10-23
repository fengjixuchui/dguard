//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#include "TDgiFLockConvertor.h"
#include "../../../helpers/internal/helpers.h"
#include "TDgiThriftTypesConverter.h"


namespace service
{
	namespace thrift_impl
	{
		::dgi::FLockObjectType::type toThrift(::logic::folderlock::storage::FLockObjectType _type)
		{
			if (_type == ::logic::folderlock::storage::FLockObjectType::FLock_Directory)
			{
				return ::dgi::FLockObjectType::FLock_Directory;
			}
			else if (_type == ::logic::folderlock::storage::FLockObjectType::FLock_File)
			{
				return ::dgi::FLockObjectType::FLock_File;
			}
			else if (_type == ::logic::folderlock::storage::FLockObjectType::FLock_HardDisk)
			{
				return ::dgi::FLockObjectType::FLock_HardDisk;
			}

			return ::dgi::FLockObjectType::FLock_Unknown;
		}

		::logic::folderlock::storage::FLockObjectType fromThrift(::dgi::FLockObjectType::type _type)
		{
			if (_type == ::dgi::FLockObjectType::FLock_Directory)
			{
				return ::logic::folderlock::storage::FLockObjectType::FLock_Directory;
			}
			else if (_type == ::dgi::FLockObjectType::FLock_File)
			{
				return ::logic::folderlock::storage::FLockObjectType::FLock_File;
			}
			else if (_type == ::dgi::FLockObjectType::FLock_HardDisk)
			{
				return ::logic::folderlock::storage::FLockObjectType::FLock_HardDisk;
			}

			return ::logic::folderlock::storage::FLockObjectType::FLock_Unknown;
		}

		::dgi::FLockState::type toThrift(::logic::folderlock::storage::FLockState _state)
		{
			::dgi::FLockState::type thriftFlockState = ::dgi::FLockState::type::FLock_UnknownState;

			if (_state == logic::folderlock::storage::FLock_Locked)
			{
				thriftFlockState = ::dgi::FLockState::type::FLock_Locked;
			}
			else if (_state == logic::folderlock::storage::FLock_Unlocked)
			{
				thriftFlockState = ::dgi::FLockState::type::FLock_Unlocked;
			}
			if (_state == logic::folderlock::storage::FLock_Missed)
			{
				thriftFlockState = ::dgi::FLockState::type::FLock_Missed;
			}
			if (_state == logic::folderlock::storage::FLock_Hidden)
			{
				thriftFlockState = ::dgi::FLockState::type::FLock_Hidden;
			}
			if (_state == logic::folderlock::storage::FLock_HiddenAndLocked)
			{
				thriftFlockState = ::dgi::FLockState::type::FLock_HiddenAndLocked;
			}

			return thriftFlockState;
		}

		::logic::folderlock::storage::FLockState fromThriftFlockState(::dgi::FLockState::type _state)
		{
			::logic::folderlock::storage::FLockState internalFLockState = ::logic::folderlock::storage::FLockState::FLock_UnknownState;

			if (_state == ::dgi::FLockState::type::FLock_Locked)
			{
				internalFLockState = ::logic::folderlock::storage::FLockState::FLock_Locked;
			}
			else if (_state == ::dgi::FLockState::type::FLock_Unlocked)
			{
				internalFLockState = ::logic::folderlock::storage::FLockState::FLock_Unlocked;
			}
			if (_state == ::dgi::FLockState::type::FLock_Missed)
			{
				internalFLockState = ::logic::folderlock::storage::FLockState::FLock_Missed;
			}
			else if (_state == ::dgi::FLockState::type::FLock_Hidden)
			{
				internalFLockState = ::logic::folderlock::storage::FLockState::FLock_Hidden;
			}
			else if (_state == ::dgi::FLockState::type::FLock_HiddenAndLocked)
			{
				internalFLockState = ::logic::folderlock::storage::FLockState::FLock_HiddenAndLocked;
			}

			return internalFLockState;
		}

		bool toThrift(const ::logic::folderlock::storage::FLockObject& _from, ::dgi::FLockInfo& _to)
		{
			_to.obj.flockId = std::string( (const char*) _from.uniqueId, sizeof(_from.uniqueId));
			_to.obj.path = strings::ws_s(_from.path, CP_UTF8);
			_to.obj.type = toThrift(_from.type);
			_to.state = toThrift(_from.state);

			return true;
		}

		bool fromThrift(const ::dgi::FLockInfo& _from, ::logic::folderlock::storage::FLockObject& _to)
		{
			std::wstring path = strings::s_ws(_from.obj.path, CP_UTF8);
			long targetBufferMaxSize = COUNT_OF_CHARS(_to.path); // Include last zero symbol.

			if (path.size() >= targetBufferMaxSize)
			{
				//
				// This function fails when target buffer is smaller than required.
				//
				return false;
			}

			if (_from.obj.flockId.length() > sizeof(_to.uniqueId))
			{
				return false;
			}

			fill_wchars(_to.path, path.c_str());
			_to.type = fromThrift(_from.obj.type);
			_to.state = fromThriftFlockState(_from.state);
			memcpy(_to.uniqueId, _from.obj.flockId.data(), sizeof(_to.uniqueId));
			// _to.pathLength // not initialized!!

			return true;
		}
	}
}
