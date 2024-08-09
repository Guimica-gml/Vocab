@echo off
if not exist build (mkdir build)

cl.exe /std:clatest /c /Fo:build/vocab.obj main.c /I include/raylib/src && link.exe /out:build/vocab.exe build/vocab.obj lib/raylib.lib
