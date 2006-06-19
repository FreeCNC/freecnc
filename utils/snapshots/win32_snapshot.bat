@echo off
setlocal
set SOURCEPATH="..\.."
set TMPDIR="FreeCNC"

mkdir %TMPDIR%

rem Copying root directory files
copy /Y %SOURCEPATH%\FreeCNC.exe %TMPDIR%
copy /Y %SOURCEPATH%\SDL.dll %TMPDIR%
copy /Y %SOURCEPATH%\SDL_mixer.dll %TMPDIR%
copy /Y %SOURCEPATH%\INSTALL.txt %TMPDIR%
copy /Y %SOURCEPATH%\README.txt %TMPDIR%
copy /Y %SOURCEPATH%\TODO.txt %TMPDIR%
copy /Y %SOURCEPATH%\ChangeLog.txt %TMPDIR%

rem documentation
mkdir %TMPDIR%\doc
copy /Y %SOURCEPATH%\doc\*.* %TMPDIR%\doc

rem Data stuffs
xcopy /Y /I /E %SOURCEPATH%\data\gfx %TMPDIR%\data\gfx
xcopy /Y /I /E %SOURCEPATH%\data\settings %TMPDIR%\data\settings
xcopy /Y /I /E %SOURCEPATH%\data\scripts %TMPDIR%\data\scripts
mkdir %TMPDIR%\data\mix

rem compress stuff
rem assumes the zip.exe free zip utility is there
rem date.exe +%Y-%m-%d
chdir %TMPBASE%
zip -q -r freecnc-INSERT_DATE-win32.zip %TMPDIR%

rem delete temporary files
rmdir /S /Q %TMPDIR%