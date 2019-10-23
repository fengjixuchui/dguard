//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//

#pragma once

#include <string>
#include <map>
#include <vector>


#define WINDOWS_SERVICE_NAME		L"DgiService"

#define THRIFT_SERVICE_ADDRESS      "127.0.0.1"
//#define THRIFT_SERVICE_ADDRESS      "192.168.1.72"
//#define THRIFT_SERVICE_ADDRESS      "10.8.211.115"



#define THRIFT_SERVICE_BASE_PORT	2991
#define THRIFT_MAIN_PORT			(THRIFT_SERVICE_BASE_PORT + 0)
#define THRIFT_ENCRYPTION_PORT		(THRIFT_SERVICE_BASE_PORT + 1)
#define THRIFT_FLOCK_PORT			(THRIFT_SERVICE_BASE_PORT + 2)
#define THRIFT_ERASING_PORT			(THRIFT_SERVICE_BASE_PORT + 3)
#define THRIFT_BANKING_PORT			(THRIFT_SERVICE_BASE_PORT + 4)

#define DGI_FOLDER_LOCK_ONLY    1


namespace dguard
{
    // ... 
}
