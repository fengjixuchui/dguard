//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#pragma once

#include "../../../logic/common/DgiCommon.h"
#include "../../../logic/encryption/files/manager/FileEncoder.h"
#include "../../../../../../thrift/cpp/DgiEncryption.h"
#include "../../../helpers/internal/log.h"

namespace service
{
	namespace thrift_impl
	{
		bool toThrift(const ::logic::encryption::EncodingMetaInfo& _fromMetainfo, dgi::EncryptionFileInfo& _toFileInfo);

		bool fromThrift(const dgi::EncryptionAlgType::type& _algType, ::logic::common::CryptAlgorithm& _resultAlgType);

		bool toThrift(const ::logic::common::CryptAlgorithm& _fromAlgType, dgi::EncryptionAlgType::type& _toAlgType);
	}
}
