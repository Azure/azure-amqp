# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

function GetAssemblyVersionFromFile($filename)
{
    $regex = 'AssemblyInformationalVersion\("(\d{1,3}\.\d{1,3}\.\d{1,3}(?:-[A-Za-z0-9-]+)?)"\)'
    $values = select-string -Path $filename -Pattern $regex | % { $_.Matches } | % { $_.Groups } | % { $_.Value }
    if( $values.Count -eq 2 ) {
        return $values[1]
    }
    Write-Host "Error: Unable to find AssemblyInformationalVersion in $filename" -foregroundcolor "red"
    exit
}

function Build-Solution
{
	Param($Configuration, $Platform)

	msbuild.exe amqp.sln /t:restore /p:Configuration=$Configuration /p:Platform="$Platform" /verbosity:minimal
	if (-Not $?)
	{
		throw "Restore failed."
	}

	msbuild.exe amqp.sln /p:Configuration=$Configuration /p:Platform="$Platform" /verbosity:minimal
	if (-Not $?)
	{
		throw "Build failed."
	}
}

function Run-Tests
{
	Param($Configuration)

	dotnet.exe test -c $Configuration --no-build .\test\Test.Microsoft.Amqp\Test.Microsoft.Amqp.csproj
	if (-Not $?)
	{
		throw "Test failed."
	}
}

function Create-Package
{
	Param($Configuration)

	$ver = GetAssemblyVersionFromFile(".\src\Properties\Version.cs")
	dotnet.exe pack -p:Version=$ver -c $Configuration --no-build .\src\Microsoft.Azure.Amqp.csproj
	if (-Not $?)
	{
		throw "Packaging failed for version $ver."
	}
}