::cd /d "%~dp0"
IF /i NOT "%PROCESSOR_ARCHITECTURE%" == "x86" (set ARC=x64) else (set ARC=x86)

set dest_dir=%windir%\system32
set drv_dir=%windir%\system32\drivers

sc stop DgiService
timeout 2
rem sc delete DgiService

sc stop "FLock"
sc stop "DgiFErase"

timeout 1
TASKKILL /F /IM DgiService.exe

del /Q "%drv_dir%\FLock.sys"
del /Q "%drv_dir%\FErase.sys"
del /Q "%dest_dir%\DgiService.exe"
del /Q "%dest_dir%\cards.storage"
del /Q "%dest_dir%\flock.storage"
del /Q "%dest_dir%\settings\mpr.conf"
rmdir /S /Q "%dest_dir%\techreport"

copy /y "%~dp0%ARC%\DgiService.exe" "%dest_dir%"
copy /y "%~dp0%ARC%\FLock\FLock.sys" "%drv_dir%"
copy /y "%~dp0%ARC%\FErase\FErase.sys" "%drv_dir%"

pnputil -i -a "%~dp0%ARC%\FLock\FLock.inf"
pnputil -i -a "%~dp0%ARC%\FErase\FErase.inf"

timeout 2

sc start "FLock"
sc start "DgiFErase"

sc create DgiService binpath= "%dest_dir%\DgiService.exe" type= own start= auto displayname= "Data Guard"
sc start DgiService

pause
