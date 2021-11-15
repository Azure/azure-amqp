@setlocal EnableExtensions EnableDelayedExpansion
@echo off

set current-path=%~dp0
set build-root=%current-path%..
rem // resolve to fully qualified path
for %%i in ("%build-root%") do set build-root=%%~fi

rem ensure nuget.exe exists
if not exist %current-path%\NuGet.exe (
  @Echo Downloading NuGet.exe from https://www.nuget.org
  Powershell.exe wget -outf %current-path%\NuGet.exe https://dist.nuget.org/win-x86-commandline/latest/nuget.exe
)
if not exist %current-path%\NuGet.exe (
	echo Failed to download NuGet.exe
	exit /b 1
)

rem -----------------------------------------------------------------------------
rem -- parse script arguments
rem -----------------------------------------------------------------------------

rem // default build options
set build-clean=0
set build-config=Debug
set build-platform=Any CPU

:args-loop
if "%1" equ "" goto args-done
if "%1" equ "-c" goto arg-build-clean
if "%1" equ "--clean" goto arg-build-clean
if "%1" equ "--config" goto arg-build-config
if "%1" equ "--platform" goto arg-build-platform
call :usage && exit /b 1

:arg-build-clean
set build-clean=1
goto args-continue

:arg-build-config
shift
if "%1" equ "" call :usage && exit /b 1
set build-config=%1
goto args-continue

:arg-build-platform
shift
if "%1" equ "" call :usage && exit /b 1
set build-platform=%1
goto args-continue

:args-continue
shift
goto args-loop

:args-done

rem -----------------------------------------------------------------------------
rem -- build solution
rem -----------------------------------------------------------------------------

call "%current-path%\nuget.exe" restore "%build-root%\microsoft_azure_amqp.sln"
if %build-clean%==1 (
    call :clean-a-solution "%build-root%\microsoft_azure_amqp.sln" "%build-config%" "%build-platform%"
    if not !errorlevel!==0 exit /b !errorlevel!
)

call :build-a-solution "%build-root%\microsoft_azure_amqp.sln" "%build-config%" "%build-platform%"
if not !errorlevel!==0 exit /b !errorlevel!

if /I "%build-config%" == "Release" call :package %build-root%\bin\Payload
if /I "%build-config%" == "Signed" call :package %build-root%\bin\Payload

rem -----------------------------------------------------------------------------
rem -- done
rem -----------------------------------------------------------------------------

goto :eof


rem -----------------------------------------------------------------------------
rem -- subroutines
rem -----------------------------------------------------------------------------

:clean-a-solution
call :_run-msbuild Clean %1 %2 %3
echo %errorlevel%
goto :eof

:build-a-solution
call :_run-msbuild Build %1 %2 %3
goto :eof

:usage
echo build.cmd [options]
echo options:
echo  -c, --clean           delete artifacts from previous build before building
echo  --config ^<value^>      [Debug] build configuration (e.g. Debug, Release, Signed)
echo  --platform ^<value^>    [Win32] build platform (e.g. Win32, x64, ...)
goto :eof

:package
set dest-path=%1
rd /s /q "%dest-path%"
mkdir "%dest-path%"
xcopy "%build-root%\icon.png" "%dest-path%\images\" /F
xcopy "%build-root%\Microsoft.Azure.Amqp\bin\%build-config%\net45\Microsoft.Azure.Amqp.dll" "%dest-path%\lib\net45\" /F
xcopy "%build-root%\Microsoft.Azure.Amqp\bin\%build-config%\net45\Microsoft.Azure.Amqp.xml" "%dest-path%\lib\net45\" /F
xcopy "%build-root%\Microsoft.Azure.Amqp\bin\%build-config%\netstandard1.3\Microsoft.Azure.Amqp.dll" "%dest-path%\lib\netstandard1.3\" /F
xcopy "%build-root%\Microsoft.Azure.Amqp\bin\%build-config%\netstandard1.3\Microsoft.Azure.Amqp.xml" "%dest-path%\lib\netstandard1.3\" /F
xcopy "%build-root%\Microsoft.Azure.Amqp\bin\%build-config%\netstandard2.0\Microsoft.Azure.Amqp.dll" "%dest-path%\lib\netstandard2.0\" /F
xcopy "%build-root%\Microsoft.Azure.Amqp\bin\%build-config%\netstandard2.0\Microsoft.Azure.Amqp.xml" "%dest-path%\lib\netstandard2.0\" /F
xcopy "%build-root%\Microsoft.Azure.Amqp.Uwp\bin\%build-config%\Microsoft.Azure.Amqp.dll" "%dest-path%\lib\uap10.0\" /F
xcopy "%build-root%\Microsoft.Azure.Amqp.Uwp\bin\%build-config%\Microsoft.Azure.Amqp.pri" "%dest-path%\lib\uap10.0\" /F
xcopy "%build-root%\Microsoft.Azure.Amqp.Android\bin\%build-config%\Microsoft.Azure.Amqp.dll" "%dest-path%\lib\monoandroid\" /F
xcopy "%build-root%\Microsoft.Azure.Amqp.Pcl\bin\%build-config%\Microsoft.Azure.Amqp.dll" "%dest-path%\lib\portable-net45+wp8+wpa81+win8+MonoAndroid10+MonoTouch10+Xamarin.iOS10+UAP10\" /F
powershell %current-path%\make_nuget_package.ps1
goto :eof

rem -----------------------------------------------------------------------------
rem -- helper subroutines
rem -----------------------------------------------------------------------------

:_run-msbuild
echo msbuild /t:%1 /v:m "/p:Configuration=%~3;Platform=%~4" %2
msbuild /t:%1 /v:m "/p:Configuration=%~3;Platform=%~4" %2
if not %errorlevel%==0 exit /b %errorlevel%
goto :eof
