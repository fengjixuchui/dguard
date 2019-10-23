//
// author:
//			Burlutsky Stas
//			burluckij@gmail.com
//

namespace cpp dgi

include "dgiCommonTypes.thrift"
include "dgiEncryptionTypes.thrift"

//
// Here is an interface of service for files encryption.
//

service DgiEncryption
{
	//
	// Returns true if file is encoded.
	//
	dgiCommonTypes.BoolResponse isFileEncoded(1: string _filepath)
	
	//
	// Returns information about file - encrypted or not. And if it is than it has meta information.
	//
	dgiEncryptionTypes.ResponseFileInfo getFileInfo(1: string _filepath)

	//
	// Request to encrypt file's data asynchronously.
	// To get final operation result you should call another method to read a state of this operation.
	// Async operation could be in following states: NotFound, InProcess, Completed.
	//
	dgiCommonTypes.AsyncResponse encryptFileAsync(1: dgiEncryptionTypes.RequestEncryptFile _file)
	
	//
	// Encrypts file synchronously.
	//
	dgiCommonTypes.DgiResult encryptFile(1: dgiEncryptionTypes.RequestEncryptFile _file)
	
	//
	// Returns state of previously called asynchronous encryptFileAsync(..) method.
	// That methods returns only state without additional information (any specific errors, results and so on).
	//
	dgiCommonTypes.DgiResult getState(1: dgiCommonTypes.AsyncId  _asyncId)
	
	//
	// Decodes file synchroniously.
	//
	dgiEncryptionTypes.ResponseDecodeFile decodeFile(1: dgiEncryptionTypes.RequestDecodeFile _file)
	
	dgiCommonTypes.AsyncResponse decodeFileAsync(1: dgiEncryptionTypes.RequestDecodeFile _file)
}
