@echo off
SET prj_name=BigMouse
SET "current_dir=%~dp0"
SET "build_dir=%current_dir%build\"
SET "distrib_dir=%current_dir%distrib\%prj_name%\"

REM This is optional bat file to rebuild both DLL and EXE
REM You can run it from command line or integrate into VSCode tasks

cd /d "%current_dir%"
FOR /F "tokens=*" %%i IN ('where g++.exe') DO pushd "%%~dpi..\.." && (call set "MSYS_ROOT=%%CD%%") && popd
echo MSYS_ROOT: %MSYS_ROOT%

echo removing old build
rmdir /s /q "%build_dir%"
REM rmdir /s /q "%distrib_dir%"

echo creating build directory "%build_dir%"
mkdir "%build_dir%"
mkdir "%distrib_dir%"
pushd "%build_dir%"

echo dependencies can be installed in MinGW with: pacman -S --needed git zip mingw-w64-ucrt-x86_64-toolchain base-devel make automake autoconf libtool pkg-config

echo building knmhooker.dll from %current_dir%src\knmhooker.cpp...
g++ -shared -o knmhooker.dll %current_dir%src\knmhooker.cpp -DBUILDING_DLL -static-libgcc -static-libstdc++

echo building BigMouse.exe from %current_dir%src\BigMouse.cpp...
windres %current_dir%src\resource.rc -o resource.o
g++ -o BigMouse.exe %current_dir%src\BigMouse.cpp resource.o -L. -lknmhooker -lgdi32 -mwindows -static-libgcc -static-libstdc++

echo copying binaries to the distrib folder...
copy /Y *.exe "%distrib_dir%"
copy /Y "%current_dir%LICENSE" "%distrib_dir%"
popd

pushd "%distrib_dir%.." && "%MSYS_ROOT%\usr\bin\zip.exe" -r %prj_name%.zip %prj_name% && popd
echo Created distribution file at distrib\%prj_name%.zip

REM taskkill /IM "BigMouse.exe" /F