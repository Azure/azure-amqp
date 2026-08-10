# Microsoft Azure AMQP for .Net

[![Build status](https://github.com/Azure/azure-amqp/actions/workflows/ci.yml/badge.svg)](https://github.com/Azure/azure-amqp/actions/workflows/ci.yml)    [![NuGet Version and Downloads count](https://buildstats.info/nuget/Microsoft.Azure.Amqp)](https://www.nuget.org/packages/Microsoft.Azure.Amqp/)

This repository contains the source code for the Microsoft Azure AMQP for C# implementation.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.


### Build

- In Visual Studio
  - Open the solution `amqp.sln` and build. Visual Studio 2019 is required.
- dotnet
  - The project targets netstandard2.0, so dotnet commands can be used to build the library cross platform.

```
dotnet build -p:Version=3.0.0 src\Microsoft.Azure.Amqp.csproj
```

### CFSClean test broker build

Azure SDK pipelines that run on CFSClean agents must restore `TestAmqpBroker` with the checked-in `nuget.cfsclean.config`. The config clears inherited package sources. It uses the public `azure-sdk-for-net` Azure Artifacts feed, which has a NuGet.org upstream.

Downstream repositories clone this repository at a pinned commit and follow this section. Keep the clone unchanged and pass `--configfile` on the restore. A consumer can override the pinned commit with the `TEST_BROKER_COMMIT` environment variable.

#### Package source names

The root `nuget.config` names the NuGet.org source `nuget.org`. Keep that name and its case, and do not add a second source that repeats the NuGet.org URL.

A machine policy can turn off NuGet.org with a `disabledPackageSources` entry, and that entry matches a source by exact name and not by URL. A source that repeats the same URL under another name is a different source, and the policy leaves it on. This file used the name `NuGet official package source` until [#318](https://github.com/Azure/azure-amqp/pull/318), so a restore on a machine with such a policy reached NuGet.org directly. A NuGet configuration cannot block NuGet.org on its own, so this rule lets the repository cooperate with a policy and is not a boundary.

#### .NET SDK requirement

`global.json` pins the SDK to version `10.0.100`, with `rollForward: latestFeature` and `allowPrerelease: false`. This policy accepts any released `10.0.x` SDK at version `10.0.100` or later. It does not roll forward to a different major or minor version. It does not accept a prerelease SDK. The agent must have a released 10.0 SDK.

Run both commands with the clone root as the working directory. The `dotnet` muxer searches for `global.json` from the current working directory upward, and not from the project directory. A command that starts in a different directory can select a different SDK, or find no SDK at all.

#### Restore and build

```
dotnet restore ./test/TestAmqpBroker/TestAmqpBroker.csproj --configfile ./nuget.cfsclean.config
dotnet build ./test/TestAmqpBroker/TestAmqpBroker.csproj --configuration Debug --framework net10.0 --no-restore
```

The paths use forward slashes. The dotnet CLI accepts forward slashes on Windows, Linux, and macOS.

#### Restore scope

`TestAmqpBroker` targets `net48;net10.0`. The restore resolves both frameworks, but the build makes only `net10.0`. Keep this difference.

Do not try to narrow the restore to one framework. `dotnet restore` has no option for a single target framework. A global property such as `-p:TargetFramework=net10.0` is not a substitute. The property flows into the `netstandard2.0` project reference `src/Microsoft.Azure.Amqp.csproj`. That project then writes an assets file with no `netstandard2.0` target, and the build fails with error `NETSDK1005`.

The `net48` half of the restore adds only reference-assembly packages. Those packages are platform independent, so this restore also succeeds on a Linux agent.

#### Build output

The broker project sets `OutputPath` to `bin/$(Configuration)/$(MSBuildProjectName)/`, relative to the clone root. The project is multi-targeted, and this repository has no `Directory.Build.props` or `Directory.Build.targets`, so MSBuild appends the target framework to the output path. The commands above write the broker to this path, relative to the clone root:

```
bin/Debug/TestAmqpBroker/net10.0/TestAmqpBroker.dll
```

A consumer that starts the broker must use that path. The same directory also holds a native `TestAmqpBroker` host executable.

#### Feed access and authentication

The `azure-sdk-for-net` feed answers anonymous reads. The feed already holds every package that this restore needs, so the restore succeeds on an agent with no credentials.

Authentication is necessary only for a cache miss. The feed fetches a package from its NuGet.org upstream only for an authenticated request. Azure DevOps pipelines must therefore run `NuGetAuthenticate@1` before the restore. A new or changed dependency then restores correctly, and the agent stays isolated from NuGet.org.

Normal developer builds do not use this config. They continue to use the root `nuget.config` and its NuGet.org source.

#### No coverage in this repository

No pipeline in this repository uses `nuget.cfsclean.config`. A change to the broker's dependencies can therefore break CFSClean restores with no signal here. The failure appears in the downstream Azure SDK repositories instead.
