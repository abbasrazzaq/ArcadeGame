@echo off

REM ===================
REM === Win32 Build ===
REM ===================

REM CALL "D:\Programs\Microsoft Visual Studio 10.0\VC\vcvarsall.bat"
REM If you need to have cl.exe work this ^^^

REM Possible values: debug,release
set build=debug
echo %build% build

set internal_libs_path=..\proj\internal_libs

IF "%build%" == "debug" (
set build_compiler_flags=-MTd -Od
) ELSE IF "%build%" == "release" (
set build_compiler_flags=-MT -O2
)

set common_compiler_flags=%build_compiler_flags% -nologo -Gm- -GR- -EHa- -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4127 -wd4239 -wd4505 -wd4345 -wd4996 -wd4146 -FC -Z7 -DTWOSOME_INTERNAL=1 -DTWOSOME_SLOW=1

set exe_cpp_files=..\proj\code\win32_twosome.cpp
set internal_build=false

REM Check if we're doing an internal build
echo %common_compiler_flags% | findstr /C:"TWOSOME_INTERNAL=1">nul && (
set common_compiler_flags=%common_compiler_flags% /I%internal_libs_path%
echo internal build
set internal_build=true

) || (
REM Game links directly with game translation unit when not internal build otherwise uses dll
set exe_cpp_files=%exe_cpp_files% ..\proj\code\twosome.cpp
)

set common_linker_flags=-incremental:no -opt:ref -subsystem:windows,5.1 user32.lib gdi32.lib winmm.lib OpenGL32.lib Shell32.lib

IF NOT EXIST ..\..\build mkdir ..\..\build
pushd ..\..\build

del *.pdb > NUL 2> NUL

REM Asset file builder build
cl %common_compiler_flags% -DTWOSOME_INTERNAL=1 -DTWOSOME_SLOW=1 ..\proj\code\asset_builder.cpp /I%internal_libs_path% /link %common_linker_flag%

REM Game code dll
IF "%internal_build%" == "true" (
echo lock > pdb.lock
cl %common_compiler_flags% ..\proj\code\twosome.cpp -Fmtwosome.map /LD /link -incremental:no -opt:ref -PDB:twosome_%random%.pdb /EXPORT:game_update_and_render /EXPORT:game_get_sound_samples /EXPORT:DEBUG_game_frame_end
del pdb.lock
)

REM Main executable
REM exe icon resource file
rc /nologo /fo"twosome.res" ..\proj\code\win32\icon_resource.rc

cl /Fetwosome %common_compiler_flags% %exe_cpp_files% -Fmtwosome.map twosome.res /link %common_linker_flags%

@echo build finished

popd