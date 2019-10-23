//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#include "TDgiEraseConvertor.h"
#include "../../../helpers/internal/helpers.h"
#include "TDgiThriftTypesConverter.h"

namespace service
{
	namespace thrift_impl
	{
		::dgi::EraseObjectType::type toThriftObjectType(::logic::common::EraseObjectType _from)
		{
			switch (_from)
			{
			case ::logic::common::EraseObjectType::EOT_File:
				return ::dgi::EraseObjectType::Erase_File;

			case ::logic::common::EraseObjectType::EOT_Directory:
				return ::dgi::EraseObjectType::Erase_Directory;

			case ::logic::common::EraseObjectType::EOT_Disk:
				return ::dgi::EraseObjectType::Erase_HardDisk;
			}

			return ::dgi::EraseObjectType::Erase_Unknown;
		}

		::logic::common::EraseObjectType fromThriftObjectType(::dgi::EraseObjectType::type _from)
		{
			switch (_from)
			{
			case  ::dgi::EraseObjectType::Erase_File:
				return ::logic::common::EraseObjectType::EOT_File;

			case ::dgi::EraseObjectType::Erase_Directory:
				return ::logic::common::EraseObjectType::EOT_Directory;

			case ::dgi::EraseObjectType::Erase_HardDisk:
				return ::logic::common::EraseObjectType::EOT_Disk;
			}

			return ::logic::common::EOT_Unknown;
		}

		bool fromThrift(const dgi::EraseList& _fromThriftList, std::vector<::logic::common::EraseObject>& _toEraseList)
		{
			std::vector<::logic::common::EraseObject> toEraseList;

			for (auto thriftErObj : _fromThriftList)
			{
				::logic::common::EraseObject intEraseObject;
				intEraseObject.objectType = fromThriftObjectType(thriftErObj.type);
				intEraseObject.path = strings::s_ws(thriftErObj.path, CP_UTF8);

				//
				// Object is converted into internal type.
				//
				toEraseList.push_back(intEraseObject);
			}

			// Swap if everything is ok.
			toEraseList.swap(_toEraseList);

			return true;
		}

		bool toThrift(const std::vector<::logic::common::EraseObjectResult>& _from, dgi::EraseErrorList& _to)
		{
			dgi::EraseErrorList convertedList;

			for (auto intEraseResult : _from)
			{
				dgi::EraseObjectError eraseResult;
				eraseResult.erObject.type = toThriftObjectType(intEraseResult.object.objectType);
				eraseResult.erObject.path = strings::ws_s(intEraseResult.object.path, CP_UTF8);

				// Convert internal status to external.
				eraseResult.status = toThrift(intEraseResult.result);

				convertedList.push_back(eraseResult);
			}

			// if everything was ok.
			_to.swap(convertedList);

			return true;
		}
	}
}
