# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

function GetAssemblyVersionFromFile($filename) {
    $regex = 'AssemblyInformationalVersion\("(\d{1,3}\.\d{1,3}\.\d{1,3}(?:-[A-Za-z0-9-]+)?)"\)'
    $values = select-string -Path $filename -Pattern $regex | % { $_.Matches } | % { $_.Groups } | % { $_.Value }
    if( $values.Count -eq 2 ) {
        return $values[1]
    }
    Write-Host "Error: Unable to find AssemblyInformationalVersion in $filename" -foregroundcolor "red"
    exit
}

if (-Not (Test-Path 'NuGet.exe')) {
    Invoke-WebRequest 'https://dist.nuget.org/win-x86-commandline/latest/nuget.exe' -OutFile 'NuGet.exe'
}

$PSScriptRoot = Split-Path -Parent -Path $MyInvocation.MyCommand.Definition
$AssemblyInfoFile = "$PSScriptRoot\..\Properties\Version.cs"
$BuildConfig = "Release"
$OutputDirectory = [IO.Path]::GetFullPath("$PSScriptRoot\..\..\bin\$BuildConfig\")

# Delete existing packages to force rebuild
ls "$OutputDirectory\Microsoft.Azure.Amqp.*.nupkg" | % { del $_ }

$ver = GetAssemblyVersionFromFile($AssemblyInfoFile)
$id='Microsoft.Azure.Amqp'

echo "Creating NuGet package $id version $ver"
.\NuGet.exe pack "$PSScriptRoot\$id.nuspec" -Prop Configuration=$BuildConfig -Prop Version=$ver -OutputDirectory $OutputDirectory