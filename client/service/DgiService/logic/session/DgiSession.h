//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#pragma once

#include <map>
#include <set>
#include <string>
#include <thread>
#include <vector>
#include "../../helpers/containers/ths_map.h"


namespace logic
{
	namespace session
	{
		typedef std::string SessionId;
		typedef std::set<std::string> AccessRules;

		enum AccessLevel
		{
			NoAccess,
			FullAccess,
			SoftAccess
		};

		struct Info
		{
			SessionId sid; // unique identificator
			std::wstring holder; // @internal for root access.
			std::string creationTime; // time when this session was created.
			AccessRules rules; // which operations holder can do. This is user-defined operations.
		};


		class Manager
		{
		public:
			typedef ThsMap<SessionId, Info> SessionTable;
			typedef std::vector<Info> UserSessions;

			// Создаёт сессию, для внутреннего - технического использования.
			// Она не доступна из вне, используется только внутри сервиса.
			SessionId getInternalSession();

			SessionId create();
			void saveSession(SessionId _id, Info _info);
			void close(SessionId _id);
			void closeAll();

			// Verifies an existence of the session.
			bool present(SessionId _session);

			bool getSessionInfo(SessionId _sid, Info& _info);

			bool getForHolder(const std::wstring& _holder, SessionId& _sid);

			SessionTable::ThsMapType getSessions();

		private:
			SessionTable m_sessions;
		};


		//
		// Единственно существующий объект хранения пользовательских сессий.
		//
		// Существует только в памяти, на диске ничего не хранит.
		//
		class Storage
		{
		private:
			static Manager* g_sziUserSessions;
			static std::mutex g_lock;

		public:
			static Manager& get();
			static void destroy();
		};
	}
}
