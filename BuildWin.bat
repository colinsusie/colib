@echo off

@REM call "c:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
@REM  set LUA_SRC="..\lua\lua-5.4.2\src"

If NOT Defined LUA_SRC (
    set /p LUA_SRC=input lua src path:
)
cl /MD /O2 /c /DLUA_BUILD_AS_DLL /utf-8 /I"%LUA_SRC%" src/*.c
link /DLL /LIBPATH:"%LUA_SRC%" lua.lib /OUT:colib/colibc.dll *.obj

pause