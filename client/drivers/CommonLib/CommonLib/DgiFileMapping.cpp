#include "DgiFileMapping.h"

szi::SziFileMapping::SziFileMapping()
	:map_(NULL), address_(NULL)
{
}

szi::SziFileMapping::~SziFileMapping()
{
	UnMap();
}

bool szi::SziFileMapping::Map(HANDLE file, unsigned long offset, unsigned long length, unsigned long maxLength)
{
	UnMap();

	DWORD lprotect = PAGE_READWRITE;
	DWORD lMaxSizeLow = LOWORD(maxLength);
	DWORD lMaxSizeHigh = HIWORD(maxLength);

	map_ = CreateFileMapping(file, NULL, lprotect, lMaxSizeLow, lMaxSizeHigh, NULL);
	if (map_ == NULL)
	{
		return false;
	}

	DWORD	lDesiredAccess = FILE_MAP_WRITE | FILE_MAP_READ;
	DWORD	lFileOffsetHigh = HIWORD(offset);
	DWORD	lFileOffsetLow = LOWORD(offset);
	SIZE_T	lBytesToMap = length;
	
	address_ = MapViewOfFile(map_, lDesiredAccess, lFileOffsetHigh, lFileOffsetLow, lBytesToMap);

	return address_ != NULL;
}

void szi::SziFileMapping::UnMap()
{
	if(address_)
	{
		UnmapViewOfFile(address_);
		address_ = NULL;
	}

	if (map_ != NULL)
	{
		CloseHandle(map_);
		map_ = NULL;
	}	
}

void * szi::SziFileMapping::Address()
{
	return address_;
}

void szi::SziFileMapping::Flush(unsigned long size)
{
	void * lAddress = Address();
	if (lAddress != NULL)
	{
		FlushViewOfFile(lAddress, size);
	}	
}
