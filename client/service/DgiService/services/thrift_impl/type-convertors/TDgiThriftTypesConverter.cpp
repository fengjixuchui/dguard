//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#include "TDgiThriftTypesConverter.h"

namespace service
{
	namespace thrift_impl
	{
		//
		// I did a tricky thing =) All thrift enum values converting directly into internal types.
		//
		// Do not thank me, please xD

		dgi::DgiStatus::type toThrift(::logic::common::InternalStatus _intStatusCode)
		{
			return (dgi::DgiStatus::type) _intStatusCode;
		}

		::logic::common::InternalStatus fromThrift(dgi::DgiStatus::type _thriftStatus)
		{
			return (::logic::common::InternalStatus)_thriftStatus;
		}

	}
}
