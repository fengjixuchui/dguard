//
// author:
//			Burlutsky Stas
//			burluckij@gmail.com
//

namespace cpp dgi

include "dgiCommonTypes.thrift"
include "dgiSecureEraseTypes.thrift"

//
// Here is an interface of service for secure data erasing (file shredder at first).
//

service DgiSecureErase
{
	//
	// Request to remove files asynchronous.
	// To get final operation result you should call another method to read a state of this operation.
	// Async operation could be in following states: NotFound, InProcess, Completed.
	//
	dgiCommonTypes.AsyncResponse eraseFiles(1: dgiSecureEraseTypes.EraseList _toErase)
	
	//
	// Returns state of the some asynchronous operation.
	//
	dgiCommonTypes.DgiResult getState( 1: dgiCommonTypes.AsyncId  _asyncId)
	
	//
	// Returns state of previously called asynchronous eraseFiles(..) method and a list of files
	// which were not removed from the system because of something.
	//
	dgiSecureEraseTypes.EraseResponse getEraseState(1: dgiCommonTypes.AsyncId  _asyncId)
	
	//
	// Deletes files from disk and erase its data secure.
	//
	dgiCommonTypes.DgiResult eraseFile(1: string _filepath)
}
