#pragma once

#include <windows.h>

namespace szi
{
	class SziFileMapping
	{
	public:
		SziFileMapping();
		~SziFileMapping();

		bool Map(HANDLE file, unsigned long offset, unsigned long length, unsigned long maxLength);
		void UnMap();
		void * Address();
		void Flush(unsigned long size);
	protected:
	private:
		HANDLE	map_;
		VOID*	address_;
	};
}
