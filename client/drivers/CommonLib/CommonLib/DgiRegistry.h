#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <list>
#include <locale>
#include <codecvt>
#include <algorithm>


namespace szi 
{
	class SziRegistry 
	{
	public:
		typedef std::vector<BYTE> RegData;
		enum RegBranch { Hklm, Hkcu };

		SziRegistry();
		~SziRegistry();
		
		template<typename R>
		static R ConvertValue(const RegData&, DWORD)
		{
			static_assert(false, "Type not support");
			return R();
		}
		
		template<>
		static int ConvertValue<int>(const RegData& Data, DWORD) 
		{
			return *(int *)(&Data[0]);
		}

		template<>
		static std::time_t ConvertValue<std::time_t>(const RegData& Data, DWORD)
		{
			return *(std::time_t *)(&Data[0]);
		}

		template<>
		static unsigned long ConvertValue<unsigned long>(const RegData& Data, DWORD)
		{
			return *(unsigned long *)(&Data[0]);
		}

		template<>
		static std::wstring ConvertValue<std::wstring>(const RegData& Data, DWORD Size) 
		{
			std::wstring lResult((wchar_t *)(&Data[0]), Size / sizeof(wchar_t));
			return lResult;
		}

		template<>
		static std::string ConvertValue<std::string>(const RegData& Data, DWORD Size)
		{
			std::string lResult((char*)(&Data[0]), Size / sizeof(char));
			return lResult;
		}

		template<>
		static std::list<std::wstring> ConvertValue<std::list<std::wstring> >(const RegData& Data, DWORD Size)
		{
			std::list<std::wstring> lResult;
			RegData::const_iterator lItBeginPos = Data.begin();
			while (lItBeginPos != Data.end())
			{
				RegData::const_iterator lItPos = std::adjacent_find(lItBeginPos, Data.end());
				if (lItPos == Data.end())
				{
					std::wstring lValue((wchar_t *)(&*lItBeginPos), Data.size() / sizeof(wchar_t));
					lResult.push_back(lValue);
					break;
				} else if(*lItPos != 0x0)
				{
					continue;
				} else
				{
					size_t			lLength = std::distance(lItBeginPos, ++lItPos);
					std::wstring	lValue((wchar_t *)(&*lItBeginPos), lLength / sizeof(wchar_t));
					lResult.push_back(lValue);
					
					lItBeginPos = lItPos + sizeof(wchar_t);

					if (lItBeginPos != Data.end())
					{
						while(*lItBeginPos == 0x0)
						{
							++lItBeginPos;
							if(lItBeginPos == Data.end())
							{
								break;
							}
						}
					} else {
						break;
					}					
				}
			}
			return lResult;
		}

		template<typename T>
		static DWORD ConvertType() 
		{
			static_assert(false, "Type not support");						
			return REG_NONE;
		}

		template<>
		static DWORD ConvertType<DWORD>() 
		{		
			return REG_DWORD;
		}

		template<>
		static DWORD ConvertType<unsigned int>()
		{
			return REG_DWORD;
		}

		template<>
		static DWORD ConvertType<bool>()
		{
			return REG_DWORD;
		}

		template<>
		static DWORD ConvertType<std::time_t>()
		{
			return REG_QWORD;
		}

		template<typename R>
		static bool GetValue(RegBranch Reg, const std::wstring& Key, const std::wstring& Value, R & RetValue)
		{
			HKEY	lHkey = NULL;
			HKEY	lRkey = Reg == RegBranch::Hklm ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
			bool	lResult = false;
			DWORD	lTypeData = 0;
			DWORD	lSizeData = 512;
			RegData lData(lSizeData);

			LONG lRet = RegCreateKeyExW(lRkey, Key.c_str(), 0, NULL, 0, KEY_READ, NULL, &lHkey, NULL);
			if(lRet == ERROR_SUCCESS)
			{
				lRet = RegQueryValueExW(lHkey, Value.c_str(), NULL, &lTypeData, &lData[0], &lSizeData);
				if(lRet == ERROR_SUCCESS)
				{
					//TODO:: check lTypeData and convert to result data
					RetValue = ConvertValue<R>(lData, lSizeData);
					lResult = true;
				} else {
					//TODO:get error
				}
				RegCloseKey(lHkey);
			} 
			return lResult;
		}

		template<typename T>
		static bool SetValue(RegBranch Reg, const std::wstring& Key, const std::wstring& ValueName, T & Value)
		{
			HKEY	lHkey = NULL;
			HKEY	lRkey = Reg == RegBranch::Hklm ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
			bool	lResult = false;
			DWORD	lSizeData = sizeof(T);
			const BYTE * lData = reinterpret_cast<const BYTE *>(&Value);
			DWORD	lTypeData = ConvertType<T>();
			
			LONG lRet = RegCreateKeyExW(lRkey, Key.c_str(), 0, NULL, 0, KEY_WRITE, NULL, &lHkey, NULL);
			if(lRet == ERROR_SUCCESS)
			{
				lRet = RegSetValueExW(lHkey, ValueName.c_str(), 0, lTypeData, lData, lSizeData);
				if(lRet == ERROR_SUCCESS) 
				{
					lResult = true;
				} else 
				{
					//TODO:get error
				}
				RegCloseKey(lHkey);
			}
			return lResult;
		}
	};
}
