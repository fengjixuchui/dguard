//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#pragma once

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>

#include "../../../../../../thrift/cpp/DgiFolderLock.h"
#include "../../../helpers/internal/log.h"
#include "../../../logic/common/DgiCommon.h"

namespace service
{
	namespace thrift_impl
	{
		using namespace ::logic::common;

		using namespace ::apache::thrift;
		using namespace ::apache::thrift::protocol;
		using namespace ::apache::thrift::transport;
		using namespace ::apache::thrift::server;

		//
		// Apache::thrift interface implementation is here.
		//
		class DgiFolderlockService : virtual public ::dgi::DgiFolderLockIf
		{
			//
			// That service is pretty simple because of absence asynchronous calls. 
			//

		public:
			DgiFolderlockService(std::string _logfile);
			~DgiFolderlockService();

			// Returns common state of hole FLock subsystem.
			//
			virtual void getSubsState(::dgi::SubSystemStateResponse& _return) override;

            //
            //  The method verifies an ability to work with EAs on target volume by trying to create 
            //  a new temporary file and write some Extended Attributes there.
            //
            virtual void isSupportedFs(::dgi::BoolResponse& _return, const std::string& _path) override;

			// Adds new flock in a protection area.
			//
			virtual  ::dgi::DgiStatus::type add(const  ::dgi::DgiSid& _sid, const  ::dgi::FLockInfo& _flock) override;
			
			// Lookups file in our storage by its file path.
			//
			virtual void present(::dgi::BoolResponse& _return, const  ::dgi::DgiSid& _sid, const  ::dgi::utf8string& _flockPath) override;

			// Verifies presence by unique flock id.
			//
			virtual void presentById(::dgi::BoolResponse& _return, const  ::dgi::DgiSid& _sid, const std::string& _flockId) override;
			
			// Returns list of all flocks.
			//
			virtual void getFlocks(::dgi::FLockListResponse& _return, const  ::dgi::DgiSid& _sid) override;

            //
            //  Changes flock's state by its id.
            //  I added that method because sometimes in GUI we have problems with strings encoding.
            //
            virtual void setStateById(::dgi::DgiResult& _return, const  ::dgi::DgiSid& _sid, const std::string& _flockId, const  ::dgi::FLockState::type _newState) override;
			
			// Returns state only of one flock.
			//
			virtual void getState(::dgi::FLockStateResponse& _return, const  ::dgi::DgiSid& _sid, const  ::dgi::utf8string& _flockPath) override;

			// Changes flock's protection state - locked, unlocked, hidden, locked and hidden.
			//
			virtual void setState(::dgi::DgiResult& _return, const  ::dgi::DgiSid& _sid, const  ::dgi::utf8string& _flockPath, const  ::dgi::FLockState::type _newState) override;

			// Removes an access policy for one special flock.
			//
			virtual void remove(::dgi::DgiResult& _return, const  ::dgi::DgiSid& _sid, const  ::dgi::utf8string& _flockPath) override;
			
			// Clears flock storage.
			//
			virtual void removeAll(::dgi::DgiResult& _return, const  ::dgi::DgiSid& _sid) override;

			// Returns all internal information about state of kernel-mode cache.
			//
			virtual void getCacheInfo(::dgi::FLockCacheInfo& _return) override;
			
		protected:

		private:

			logfile m_log;
		};
	}
}
