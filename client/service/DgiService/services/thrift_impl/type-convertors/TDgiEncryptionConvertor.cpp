//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#include "TDgiEncryptionConvertor.h"

namespace service
{
	namespace thrift_impl
	{
		bool toThrift(const ::logic::encryption::EncodingMetaInfo& _fromMetainfo, dgi::EncryptionFileInfo& _toFileInfo)
		{
			if (!toThrift(_fromMetainfo.encodingAlgorithm, _toFileInfo.encodingAlgorithm) )
			{
				printf("\n%s: an error - could not convert crypto algorithm from internal to thrift-defined.\n", __FUNCTION__);
				return false;
			}

			_toFileInfo.usedMasterPassword = _fromMetainfo.usedMasterPassword;
			_toFileInfo.originalFileSize = _fromMetainfo.originalFileSize;

			//
			// Copying strings here.
			//

			_toFileInfo.keyChecksum = _fromMetainfo.keyChecksum;
			_toFileInfo.originalChecksum = _fromMetainfo.originalChecksum;

			return true;
		}

		bool toThrift(const ::logic::common::CryptAlgorithm& _fromAlgType, dgi::EncryptionAlgType::type& _toAlgType)
		{
			switch (_fromAlgType)
			{
			case ::logic::common::CryptAlgorithm::CA_Grader:
				_toAlgType = dgi::EncryptionAlgType::EAlg_Grader;
				break;

			case ::logic::common::CryptAlgorithm::CA_Aes:
				_toAlgType = dgi::EncryptionAlgType::EAlg_Aes;
				break;

			case ::logic::common::CryptAlgorithm::CA_Aes256:
				_toAlgType = dgi::EncryptionAlgType::EAlg_Aes256;
				break;

			default:
				_toAlgType = dgi::EncryptionAlgType::EAlg_Unknown;
				// return false;
				break;
			}

			return true;
		}

		bool fromThrift(const dgi::EncryptionAlgType::type& _algType, ::logic::common::CryptAlgorithm& _resultAlgType)
		{
			switch (_algType)
			{
			case dgi::EncryptionAlgType::EAlg_Grader:
				_resultAlgType = ::logic::common::CryptAlgorithm::CA_Grader;
				break;

			case dgi::EncryptionAlgType::EAlg_Aes:
				_resultAlgType = ::logic::common::CryptAlgorithm::CA_Aes;
				break;

			case dgi::EncryptionAlgType::EAlg_Aes256:
				_resultAlgType = ::logic::common::CryptAlgorithm::CA_Aes256;
				break;

			default:
				_resultAlgType = ::logic::common::CryptAlgorithm::CA_Unknown;
				break;
			}

			return true;
		}

	}
}
