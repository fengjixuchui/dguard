#include "SziNamedPipe.h"
#include "SziDecodeError.h"
#include <iostream>

#define BUFFER_SIZE		(255 * sizeof(wchar_t))

static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

szi::SziNamedPipe::SziNamedPipe()  :
	pipe_(INVALID_HANDLE_VALUE),
	error_(0)
{
}


szi::SziNamedPipe::~SziNamedPipe() 
{
	Close();
}

bool szi::SziNamedPipe::Create(const std::wstring& name)
{	
	pipe_ = CreateNamedPipe(name.c_str(), PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE, 
										PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_REJECT_REMOTE_CLIENTS | PIPE_WAIT,
										1, BUFFER_SIZE, BUFFER_SIZE, NMPWAIT_USE_DEFAULT_WAIT, NULL);
	
	if (INVALID_HANDLE_VALUE == pipe_)
	{
		error_ = ::GetLastError();
	}

	return pipe_ != INVALID_HANDLE_VALUE ? true : false;
}

bool szi::SziNamedPipe::Open(const std::wstring& name, unsigned int timeout)
{
	if (INVALID_HANDLE_VALUE == pipe_)
	{
		if (!WaitNamedPipe(name.c_str(), timeout))
		{
			error_ = ::GetLastError();
			return false;
		}

		pipe_ = CreateFile(name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		if(INVALID_HANDLE_VALUE != pipe_)
		{			
			DWORD lmode = PIPE_READMODE_MESSAGE | PIPE_WAIT;
			if(SetNamedPipeHandleState(pipe_, &lmode, NULL, NULL) == FALSE) 
			{
				error_ = ::GetLastError();
				CloseHandle(pipe_);
				pipe_ = INVALID_HANDLE_VALUE;
			}
		} else
		{
			error_ = ::GetLastError();
		}
	}
	return pipe_ != INVALID_HANDLE_VALUE ? true : false;
}

void szi::SziNamedPipe::Close()
{
	if(INVALID_HANDLE_VALUE != pipe_) 
	{
		CloseHandle(pipe_);
		pipe_ = INVALID_HANDLE_VALUE;
	}	
}

bool szi::SziNamedPipe::Write(const std::wstring& message)
{
	BOOL	lisWrite = FALSE;
	if (INVALID_HANDLE_VALUE != pipe_)
	{
		DWORD lbWrite = 0;
		const std::string lbuffer = Base64Encode(message);
		lisWrite = WriteFile(pipe_, lbuffer.c_str(), (DWORD)lbuffer.length(), &lbWrite, NULL);
		if(!lisWrite)
		{
			error_ = ::GetLastError();
		}
	}
	return lisWrite == TRUE;
}

bool szi::SziNamedPipe::Read(std::wstring& messages, unsigned int timeout)
{	
	BOOL lisReady = FALSE;

	if (INVALID_HANDLE_VALUE != pipe_)
	{
		OVERLAPPED	loConnect;
		
		ZeroMemory(&loConnect, sizeof(loConnect));
		loConnect.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

		if (INVALID_HANDLE_VALUE == loConnect.hEvent)
		{
			error_ = ::GetLastError();
			return false;
		}

		if (ConnectNamedPipe(pipe_, &loConnect) == FALSE)
		{
			DWORD lerror = ::GetLastError();
			if(ERROR_PIPE_CONNECTED == lerror)
			{
				lisReady = TRUE;
			} else if(ERROR_IO_PENDING == lerror)
			{
				DWORD lwaitObject = WaitForSingleObjectEx(loConnect.hEvent, timeout, TRUE);
				if(WAIT_OBJECT_0 == lwaitObject)
				{
					DWORD lbyteTrans = 0;
					lisReady = GetOverlappedResult(pipe_, &loConnect, &lbyteTrans, FALSE);
					if (!lisReady)
					{
						error_ = ::GetLastError();
					}
				} else if(WAIT_TIMEOUT == lwaitObject)
				{
					lisReady = FALSE;
				} else
				{
					error_ = ::GetLastError();
				}
			} else 
			{
				error_ = ::GetLastError();
			}
		} else
		{
			error_ = ::GetLastError();
		}		

		if (lisReady)
		{
			CHAR lbuffer[BUFFER_SIZE] = "\0";
			DWORD lbRead = 0;
			if(ReadFile(pipe_, lbuffer, sizeof(lbuffer), &lbRead, NULL) == TRUE) 
			{
				messages = Base64Decode(std::string(lbuffer, lbRead));
			} else
			{
				error_ = ::GetLastError();
			}
		}

		CloseHandle(loConnect.hEvent);
		DisconnectNamedPipe(pipe_);
	}
	return lisReady == TRUE;
}

std::wstring szi::SziNamedPipe::GetLastError() const
{
	return szi::SziDecodeError::DecodeError(error_, L"");
}

unsigned int szi::SziNamedPipe::GetCodeLastError() const
{
	return error_;
}

bool szi::SziNamedPipe::IsBase64(unsigned char c) const
{
	return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string szi::SziNamedPipe::Base64Encode(const std::wstring& toEncode) const 
{
	size_t in_len = toEncode.size() * sizeof(wchar_t);
	std::string ret;
	int i = 0;
	int j = 0;
	unsigned char char_array_3[3];
	unsigned char char_array_4[4];
	const char * bytes_to_encode = (char*)toEncode.data();

	while(in_len--) {
		char_array_3[i++] = *(bytes_to_encode++);
		if(i == 3) {
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;

			for(i = 0; (i < 4); i++)
				ret += base64_chars[char_array_4[i]];
			i = 0;
		}
	}

	if(i) {
		for(j = i; j < 3; j++)
			char_array_3[j] = '\0';

		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3f;

		for(j = 0; (j < i + 1); j++)
			ret += base64_chars[char_array_4[j]];

		while((i++ < 3))
			ret += '=';
	}

	return ret;
}

std::wstring szi::SziNamedPipe::Base64Decode(const std::string& toDecode) const
{
	size_t in_len = toDecode.size();
	int i = 0;
	int j = 0;
	int in_ = 0;
	unsigned char char_array_4[4], char_array_3[3];
	std::vector<unsigned char> ret;

	while(in_len-- && (toDecode[in_] != '=') && IsBase64(toDecode[in_])) {
		char_array_4[i++] = toDecode[in_];
		in_++;
		if(i == 4) {
			for(i = 0; i < 4; i++)
				char_array_4[i] = (unsigned char)base64_chars.find(char_array_4[i]);

			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
			char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

			for(i = 0; (i < 3); i++)
				ret.push_back(char_array_3[i]);
			i = 0;
		}
	}

	if(i) {
		for(j = i; j < 4; j++)
			char_array_4[j] = 0;

		for(j = 0; j < 4; j++)
			char_array_4[j] = (unsigned char)base64_chars.find(char_array_4[j]);

		char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
		char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
		char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

		for(j = 0; (j < i - 1); j++) 
			ret.push_back(char_array_3[j]);
	}

	return std::wstring((wchar_t*)&ret[0], ret.size()/sizeof(wchar_t));
}
