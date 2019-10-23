//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#pragma once

#include <boost/noncopyable.hpp>
#include "../../../../../../thrift/cpp/dgiBankingTypes_constants.h"
#include "../../../../../../thrift/cpp/dgiCommonTypes_types.h"
#include "../../../helpers/containers/ths_map.h"


namespace service
{
	namespace thrift_impl
	{
		//
		// Содержит историю с информацией о состоянии выполнения асинхронных процедур.
		//
		typedef ThsMap<dgi::AsyncId, dgi::DgiStatus::type> AsyncOperationTable;

		// Хранитель информации о статусе выполнения асинхронных процедур.
		class AsyncRequestsKeeper : private boost::noncopyable
		{
		public:

			AsyncRequestsKeeper();

			//
			// Возвращает истину в том случае, если такой запрос с указанным номером существует (существовал).
			//
			bool isPresent(const dgi::AsyncId _requestId) const;

			//
			// Создает запрос на выполнение новой асинхронной процедуры.
			//
			dgi::AsyncId createNew(dgi::DgiStatus::type _requestState = dgi::DgiStatus::InProcess);

			//
			// Удаляет все завершенные запросы.
			//
			void removeFinished();

			// Returns true if request is finished and have status unlike 'dgi::DgiStatus::InProcess'.
			//
			bool isFinished(dgi::AsyncId _requestId);

			//
			// Возвращает статус выполнения асинхронной операции. Если такого запрорса не существует, метод вернёт false.
			//
			bool getState(const dgi::AsyncId _requestId, dgi::DgiStatus::type& _outRequestState) /* const */;

			//
			// Обновляет статус выполнения асинхронной процедуры.
			//
			void setState(const dgi::AsyncId _requestId, const dgi::DgiStatus::type _newRequestState);

			//
			// Очищает очередь запросов, но не обнуляет счётчик запросов, что позволяет отсеивать (игнорировать) старые запросы и создавать
			// новые-оригинальные запросы, с уникальными идентификаторами.
			//
			void clear();

		private:
			AsyncOperationTable m_operationStates;
			dgi::AsyncId m_counter;
			std::mutex m_counterLock;
		};

	}
}
