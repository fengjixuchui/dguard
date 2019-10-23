
#include "DgiFile.h"
#include "DgiString.h"
#include <algorithm>
#include <cwctype>


szi::SziFileCreateAttr::SziCreateAttr szi::SziFileCreateAttr::flags_ = {{ L"FILE_SHARE_DELETE", FILE_SHARE_DELETE },
																		{ L"FILE_SHARE_READ", FILE_SHARE_READ },
																		{ L"FILE_SHARE_WRITE", FILE_SHARE_WRITE },
																		{ L"CREATE_ALWAYS", CREATE_ALWAYS },
																		{ L"CREATE_NEW", CREATE_NEW },
																		{ L"OPEN_ALWAYS", OPEN_ALWAYS },
																		{ L"OPEN_EXISTING", OPEN_EXISTING },
																		{ L"TRUNCATE_EXISTING", TRUNCATE_EXISTING },
																		{ L"FILE_ATTRIBUTE_ARCHIVE", FILE_ATTRIBUTE_ARCHIVE },
																		{ L"FILE_ATTRIBUTE_ENCRYPTED", FILE_ATTRIBUTE_ENCRYPTED },
																		{ L"FILE_ATTRIBUTE_HIDDEN", FILE_ATTRIBUTE_HIDDEN },
																		{ L"FILE_ATTRIBUTE_NORMAL", FILE_ATTRIBUTE_NORMAL },																		
																		{ L"FILE_ATTRIBUTE_READONLY", FILE_ATTRIBUTE_READONLY },
																		{ L"FILE_ATTRIBUTE_SYSTEM", FILE_ATTRIBUTE_SYSTEM },
																		{ L"FILE_ATTRIBUTE_TEMPORARY", FILE_ATTRIBUTE_TEMPORARY },
																		{ L"FILE_FLAG_DELETE_ON_CLOSE", FILE_FLAG_DELETE_ON_CLOSE },
																		{ L"FILE_FLAG_NO_BUFFERING", FILE_FLAG_NO_BUFFERING },
																		{ L"FILE_FLAG_OPEN_REPARSE_POINT", FILE_FLAG_OPEN_REPARSE_POINT },
																		{ L"FILE_FLAG_RANDOM_ACCESS", FILE_FLAG_RANDOM_ACCESS },
																		{ L"FILE_FLAG_SEQUENTIAL_SCAN", FILE_FLAG_SEQUENTIAL_SCAN },
																		{ L"FILE_FLAG_WRITE_THROUGH", FILE_FLAG_WRITE_THROUGH },
																		{ L"GENERIC_READ", GENERIC_READ },
																		{ L"GENERIC_WRITE", GENERIC_WRITE }
																		};

DWORD szi::SziFileCreateAttr::operator ()(const std::wstring& flagName) const
{
	std::wstring	lFlagName;
	DWORD			lReturn = 0;
	
	std::transform(flagName.begin(), flagName.end(), std::back_inserter(lFlagName), std::towupper);
	
	SziUtilsString::SziSplitStrings lFlagStrings = SziUtilsString::Split(lFlagName, L"|");
	
	for (auto lFlag : lFlagStrings)
	{
		auto lIt = flags_.find(lFlag);
		lReturn |= lIt != flags_.end() ? lIt->second : 0;
	}

	return lReturn;
}
