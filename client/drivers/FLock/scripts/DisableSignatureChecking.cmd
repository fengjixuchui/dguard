@echo off
REM http://www.thewindowsclub.com/disable-driver-signature-enforcement-windows
openfiles > NUL 2>&1 

if NOT %ERRORLEVEL% EQU 0 goto NotAdmin 



bcdedit /set nointegritychecks on
bcdedit /set testsigning on






echo Push a key to reboot
pause
shutdown.exe /r /t 00




goto End 


















REM error messages

:NotAdmin 
echo The installation script must be run as administrator
echo If you are running it from a network share, you must also map that network share as administator
echo To do so, run the command prompt as administrator and use a command like
echo ---- net use e: "\\COMPUTER\Users\You\git\client\SyncDrive_FileSystemDriver\x64\Debug"
echo Then, run a command prompt normally and run the same command again

pause

goto End 


:End
