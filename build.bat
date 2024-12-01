@echo off

set COMP_FLAGS=/W4 /WX

set SRC_DIR=src\base
set BUILD_DIR=base
set LIB_DIR=%BUILD_DIR%\lib
set INCLUDE_DIR=%BUILD_DIR%\include\base
set LIB_FILE=base.lib

if not exist "%LIB_DIR%\" mkdir "%LIB_DIR%"
if not exist "%INCLUDE_DIR%\" mkdir "%INCLUDE_DIR%"

del %LIB_DIR%\%LIB_FILE% %INCLUDE_DIR%\*.h

for %%f in (%SRC_DIR%\*.c) do (
  cl %COMP_FLAGS% /O2 /I src /c %%f 
)

lib /OUT:%LIB_DIR%\%LIB_FILE% *.obj
del *.obj

xcopy /s %SRC_DIR%\*.h %INCLUDE_DIR%
