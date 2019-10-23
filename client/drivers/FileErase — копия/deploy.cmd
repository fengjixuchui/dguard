echo "Deploy file to Z:\"
echo "Dir:" %1
echo "File:" %2
copy /B /Y %1\%2 Z:\

rem copy /B /Y %1\SudisFltFileErase.inf Z:\
rem copy /B /Y %1\SudisFltFileErase.sys Z:\
rem copy /B /Y %1\sudisfltfileerase.cat Z:\
