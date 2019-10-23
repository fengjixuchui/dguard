//
//	Author: 
//			burluckij@gmail.com
//			Burlutsky Stanislav
//


#include "DgiCryptoTest.h"
#include "../../helpers/encryption/cryptoHelpers.h"

namespace tests
{
	bool doGostTest()
	{
		return false;
	}

	bool testPssrRsaSignature()
	{
		printf("\n%s: This is a digital signature test.\n\n", __FUNCTION__);

		std::string message = "Stas";
		std::string messageCompromised = message + "1", recovered;

		crypto::KeyPair keys;
		crypto::rsa::genKeys(keys, 1536);

		auto signature = crypto::rsa::signPssrSha1(keys.privateKey, message, false);
		bool verified = crypto::rsa::verifyPssrSha1NoMsg(keys.publicKey, message, signature);
		printf("\n\n%s\n", (verified ? "Signature is valid!" : "Invalid signature."));

		// Uncomment line to break public key.
		//
		// keys.publicKey[2] = '@';
		
		verified = crypto::rsa::verifyPssrSha1(keys.publicKey, signature, recovered);

		printf("\n\n%s\n", (verified ? "Signature is valid!" : "Invalid signature."));

		return verified;
	}

	bool test_Aes128Encoder()
	{
		::logic::common::EncoderAES aes128;

		char buffer[] = "This is my secret text!";
		char key[] = "passw0rd";

		printf("\n%s: Input data is - %s", __FUNCTION__, buffer);

		bool res = aes128.encrypt(buffer, sizeof buffer, key, sizeof key);

		if (res)
		{
			printf("\n%s: Encoded data is - ", __FUNCTION__);
			for (auto i : buffer){
				printf("%c", i);
			}

			res = aes128.decrypt(buffer, sizeof buffer, key, sizeof key);

			if (res)
			{
				printf("\n%s: Success. Decoded data is - ", __FUNCTION__);
				for (auto i : buffer){
					printf("%c", i);
				}

				printf("\n%s: Success. Test finished.\n", __FUNCTION__);
				return true;
			}
			else
			{
				printf("\n%s: Error. Decoded data is - ", __FUNCTION__);
				for (auto i : buffer){
					printf("%c", i);
				}
			}
		}
		else
		{
			printf("\n%s: Error. Can.t encode date, input data is - ", __FUNCTION__);
			for (auto i : buffer){
				printf("%c", i);
			}
		}

		printf("\n%s: Test failed.\n", __FUNCTION__);
		return false;
	}

}
