echo "Make Release"
echo "Param 1:" %1
echo "Param 2:" %2
echo "Param 3:" %3
echo "copy Param1\Param2 Param3"

copy /B /Y %1\%2 %3
