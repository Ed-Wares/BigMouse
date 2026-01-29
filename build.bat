REM This is optional bat file to rebuild both DLL and EXE
REM You can run it from command line or integrate into VSCode tasks

g++ -shared -o knmhooker.dll src\knmhooker.cpp -DBUILDING_DLL
windres src\resource.rc -o resource.o
g++ -o BigMouse.exe src\BigMouse.cpp resource.o -L. -lknmhooker -lgdi32 -mwindows

REM Start-Process powershell -Verb RunAs -ArgumentList "-Command & { BigMouse.exe }"
REM taskkill /IM "BigMouse.exe" /F