cd "%~dp0"

IF /i NOT "%PROCESSOR_ARCHITECTURE%" == "x86" (set ARC=x64) else (set ARC=x86)

"%~dp0%ARC%\DgiService.exe" -console_test

pause
