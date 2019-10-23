//
// author:
//			Burlutsky Stas
//			burluckij@gmail.com
//

namespace cpp dgi

include "dgiCommonTypes.thrift"

enum EraseObjectType
{
	Erase_Unknown,
	Erase_File,
	Erase_Directory,
	Erase_HardDisk
}

struct EraseObject
{
	1: EraseObjectType type,
	2: string path
}

typedef list<EraseObject> EraseList

struct EraseObjectError
{
	1: EraseObject erObject, // file system object itself
	2: dgiCommonTypes.DgiStatus status, // error code is here
	3: string description // why we could not erase the object
}

typedef list<EraseObjectError> EraseErrorList

//
// That structure will contain a list of files, which were not erased.
//
struct EraseResponse
{
	1: dgiCommonTypes.DgiResult result, // Common state - success if 'notErased' list is not empty
	2: EraseErrorList notErased // Here will be the objects which were not erased with error codes
}
