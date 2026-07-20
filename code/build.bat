@echo off

mkdir ..\build
pushd ..\build
gcc -std=c99 -Wall -c -g ..\code\win32_handmade.c  -o handmade.obj 

REM -Wl,--stack,4194304 pour agrandir la stack a 4Mbytes
gcc handmade.obj -o handmade.exe -lgdi32 
popd