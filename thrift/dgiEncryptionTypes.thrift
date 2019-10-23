//
// author:
//			Burlutsky Stas
//			burluckij@gmail.com
//

namespace cpp dgi

include "dgiCommonTypes.thrift"

enum HashAlgType
{
	HAlg_Unknown = 0,
	HAlg_Crc32,
	HAlg_Md5,
	HAlg_Sha1,
	HAlg_Sha256,
	HAlg_Sha512,
	HAlg_Whirepool
}

//
// (*) Should converts direct to tagert internal C++ service types.
//
enum EncryptionAlgType
{
	EAlg_Unknown = 0,
	EAlg_Grader = 1, // 64 bit encoding
	EAlg_Aes = 2, // Default AES with 128 bits length key.
	EAlg_Aes256 = 3 // The difference only in key length which is a 256 bits.
}

struct RequestEncryptFile
{
	1: string filePath, // Full path to file which we're going to encrypt.
	2: bool useMasterPassword, // Use master password as encryption key.
	3: string encryptionKey // In case we use custom encryption key, not master-password.
	4: EncryptionAlgType encodingAlgorithm,
	// 5: HashAlgType hashAlgorithm // sha256 by default.
}

struct EncryptionFileInfo
{
	//1: string dateAndTime, // Time when file was encrypted in format "13:45:22 12.06.2019"
	1: bool usedMasterPassword, // Did we use a master-password for file encryption? 
	2: EncryptionAlgType encodingAlgorithm, // Algorithm which was used encoding file's body
	3: i64 originalFileSize, // Size of original file without data-guard meta-information
	4: string originalChecksum, // Hash of original file (not encoded)
	5: string keyChecksum // Checksum of encryption key
	//
	// By default we use sha256 algorithm.
	// 7: HashAlgType hashingAlgorithm, // Type of hashing algorithm which was used for calculating hashes
}

struct ResponseFileInfo
{
	1: bool encryptedAlready, // Equals to true if file was encrypted earlier.
	2: EncryptionFileInfo info, // Has info only if the file was encrypted.
	3: dgiCommonTypes.DgiResult result // Has 'Success' status code in a good case. 
}

struct RequestDecodeFile
{
	1: string filePath, // Full path to file which we're going to decrypt.
	2: bool useMasterPassword, // Use current master password applying this operation.
	3: string encryptionKey // Use it when we do not want to use master password as encryption key.
}

struct ResponseDecodeFile
{
	1: dgiCommonTypes.DgiResult result // Has 'Success' status code when file was decoded.
	2: bool integrityCompromised, // 'true' when decoded data not equal to original (before ancoding) data.
}



