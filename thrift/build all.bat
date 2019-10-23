set "DIRNAME=%~dp0%"

"%~dp0\..\libs\thrift-0.12.0\compiller\thrift-0.12.0.exe" --gen cpp -out  "%~dp0cpp" "%~dp0dgiBanking.thrift"
"%~dp0\..\libs\thrift-0.12.0\compiller\thrift-0.12.0.exe" --gen cpp -out  "%~dp0cpp" "%~dp0dgiBankingTypes.thrift"
"%~dp0\..\libs\thrift-0.12.0\compiller\thrift-0.12.0.exe" --gen cpp -out  "%~dp0cpp" "%~dp0dgiCommonTypes.thrift"
"%~dp0\..\libs\thrift-0.12.0\compiller\thrift-0.12.0.exe" --gen cpp -out  "%~dp0cpp" "%~dp0dgiServiceManager.thrift"
"%~dp0\..\libs\thrift-0.12.0\compiller\thrift-0.12.0.exe" --gen cpp -out  "%~dp0cpp" "%~dp0dgiSecureErase.thrift"
"%~dp0\..\libs\thrift-0.12.0\compiller\thrift-0.12.0.exe" --gen cpp -out  "%~dp0cpp" "%~dp0dgiSecureEraseTypes.thrift"

"%~dp0\..\libs\thrift-0.12.0\compiller\thrift-0.12.0.exe" --gen cpp -out  "%~dp0cpp" "%~dp0dgiFolderLock.thrift"
"%~dp0\..\libs\thrift-0.12.0\compiller\thrift-0.12.0.exe" --gen cpp -out  "%~dp0cpp" "%~dp0dgiFolderLockTypes.thrift"

"%~dp0\..\libs\thrift-0.12.0\compiller\thrift-0.12.0.exe" --gen cpp -out  "%~dp0cpp" "%~dp0dgiEncryption.thrift"
"%~dp0\..\libs\thrift-0.12.0\compiller\thrift-0.12.0.exe" --gen cpp -out  "%~dp0cpp" "%~dp0dgiEncryptionTypes.thrift"

pause