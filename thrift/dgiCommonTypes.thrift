//
// author:
//			Burlutsky Stas
//			burluckij@gmail.com
//

namespace cpp dgi

typedef string DgiSid

enum DgiStatus
{
	Success = 0,
	UnknownError = 1,
	NotFound = 2,
	CriticalError = 3,
	AccessDenied = 4,
	InvalidFormat = 5,
	InProcess = 6,
	Completed = 7,
	LimitAchieved = 8,
	PresentAlready = 9,
	DriverNotConnected = 10,
	UnknownType = 11,
	HaveNoResponse = 12,
	NoMasterPassword = 13,
	LicenseExpired = 14,
	LicenseExpireSoon = 15,
	NotEncoded = 16,
	EncodedAlready = 17,
	WrongEncryptionKey = 18,
	DecodedButIntegrityCompromised = 19,
	TypeConvertionError = 20,
	NotLoaded = 21,
	FLockWasNotSigned = 22
}

struct DgiResult
{
	1: DgiStatus status,
	2: string description
}

typedef i32 AsyncId

struct AsyncResponse
{
	1: DgiResult result,
	2: AsyncId asyncId
}

typedef string utf8string

struct BoolResponse
{
	1: DgiResult errorResult, // If an error happened it is marked here.
	2: bool bool_result // If no errors - that filed has boolean result.
}

struct AuthResponse
{
	1: DgiResult result,
	2: DgiSid sessionId
}

struct SubSystemStateResponse
{
	1: DgiResult result,
	2: bool hasProblems, // The value is true when subsystem has some problems.
	3: list< DgiResult > report
}
