//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//


//
// Converter for all base (common) thrift types.
//

#pragma once

#include "../../../logic/common/DgiCommon.h"
#include "../../../../../../thrift/cpp/DgiSecureErase.h"
#include "../../../helpers/internal/log.h"

namespace service
{
	namespace thrift_impl
	{
		dgi::DgiStatus::type toThrift(::logic::common::InternalStatus _intStatusCode);

		::logic::common::InternalStatus fromThrift(dgi::DgiStatus::type _thriftStatus);
	}
}
