@echo off

set dwarn=-wd4706 -wd4127 -wd4996 -wd4100 -wd4201 -wd4204
set cflags=-Od -Oi -fp:fast -Z7 -MD -GS- -FC -W4 -nologo %dwarn%
set lflags=-dynamicbase:no -opt:ref -map -nologo
set libs=kernel32.lib user32.lib gdi32.lib winmm.lib

if not exist out mkdir out
pushd out
cl %cflags% ..\demo1.c -link -subsystem:console %lflags% %libs%
popd
