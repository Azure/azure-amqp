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

$PSScriptRoot = Split-Path -Parent -Path $MyInvocation.MyCommand.Definition
$NugetPath = "$PSScriptRoot\NuGet.exe"
$AssemblyInfoFile = "$PSScriptRoot\..\Microsoft.Azure.Amqp\Properties\Version.cs"
$InputDirectory = [IO.Path]::GetFullPath("$PSScriptRoot\..\bin\Payload\")
$OutputDirectory = [IO.Path]::GetFullPath("$PSScriptRoot\..\bin\Package\")

echo "$InputDirectory -> $OutputDirectory"

if (-Not (Test-Path $NugetPath)) {
    Invoke-WebRequest 'https://dist.nuget.org/win-x86-commandline/latest/nuget.exe' -OutFile $NugetPath
}

if (-Not (Test-Path $InputDirectory)) {
    echo "Input directory does not exist. Make sure build is complete and successful."
	exit 2
}

if (-Not (Test-Path $OutputDirectory)) {
    New-Item -ItemType Directory -Force -Path $OutputDirectory
}

# Delete existing packages to force rebuild
ls "$OutputDirectory\Microsoft.Azure.Amqp.*.nupkg" | % { del $_ }

$ver = GetAssemblyVersionFromFile($AssemblyInfoFile)
$id='Microsoft.Azure.Amqp'

echo "Creating NuGet package $id version $ver"
& "$NugetPath" pack "$PSScriptRoot\$id.nuspec" -Prop InputDirectory=$InputDirectory -Prop Version=$ver -OutputDirectory $OutputDirectory