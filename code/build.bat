@echo off

mkdir ..\build
pushd ..\build
gcc -std=c99 -Wall -c -g ..\code\win32_handmade.c  -o handmade.obj 
gcc handmade.obj -o handmade.exe -lgdi32
popd