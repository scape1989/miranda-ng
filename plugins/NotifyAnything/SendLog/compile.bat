@echo off

cl.exe /MD /O2 /Os /EHsc SendLog.cpp SLImp.cpp /link ws2_32.lib user32.lib
cl.exe /MD /O2 /Os /EHsc SendLogWin.cpp SLImp.cpp /link ws2_32.lib user32.lib

mkdir ..\..\..\%1\Debug 2>nul
copy /Y *.exe ..\..\..\%1\Debug >nul

mkdir ..\..\..\%1\Debug64 2>nul
copy /Y *.exe ..\..\..\%1\Debug64 >nul

mkdir ..\..\..\%1\Release 2>nul
copy /Y *.exe ..\..\..\%1\Release >nul

mkdir ..\..\..\%1\Release64 2>nul
copy /Y *.exe ..\..\..\%1\Release64 >nul

del *.obj;*.exe >nul
goto :eof
