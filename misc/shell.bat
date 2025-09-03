@echo off
CALL "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" x86
set path=w:\twosome\proj\misc;%path%
set _NO_DEBUG_HEAP=1