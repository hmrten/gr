@echo off

set dwarn=-wd4706 -wd4127 -wd4996 -wd4100 -wd4201 -wd4204
set cflags=-Od -Oi -fp:fast -Z7 -MD -GS- -FC -W4 -nologo %dwarn%
set lflags=-dynamicbase:no -opt:ref -map -nologo
set lflags_dll=-pdb:dll_%random%.pdb -export:gr_setup -export:gr_frame %lflags%
set libs=kernel32.lib user32.lib gdi32.lib winmm.lib

if not exist out mkdir out
pushd out
del dll_*.pdb >nul 2>&1
del *.tmp >nul 2>&1
echo 1 > pdb.lock
cl %cflags% ..\src\demo1.c -LD -Fedemo1.dll -link %lflags_dll%
del pdb.lock
cl %cflags% ..\src\gr_w32.c -Fegr.exe -link -subsystem:console %lflags% %libs%
popd
