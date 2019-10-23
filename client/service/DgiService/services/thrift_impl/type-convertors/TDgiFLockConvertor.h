//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#pragma once

#include "../../../logic/common/DgiCommon.h"
#include "../../../../../../thrift/cpp/DgiFolderLock.h"
#include "../../../helpers/internal/log.h"
#include "../../../logic/folderlock/storage/DgiFolderLockStorage.h"

namespace service
{
	namespace thrift_impl
	{
		::dgi::FLockObjectType::type toThrift(::logic::folderlock::storage::FLockObjectType _type);

		::dgi::FLockState::type toThrift(::logic::folderlock::storage::FLockState _state);

		::logic::folderlock::storage::FLockState fromThriftFlockState(::dgi::FLockState::type _state);

		::logic::folderlock::storage::FLockObjectType fromThrift(::dgi::FLockObjectType::type _type);

		bool toThrift(const ::logic::folderlock::storage::FLockObject& _from, ::dgi::FLockInfo& _to);

		bool fromThrift(const ::dgi::FLockInfo& _from, ::logic::folderlock::storage::FLockObject& _to);

	}
}
