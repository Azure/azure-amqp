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

Azure SDK pipelines running on CFSClean agents must restore `TestAmqpBroker` with the checked-in `nuget.cfsclean.config`. The config clears inherited package sources and uses the public `azure-sdk-for-net` Azure Artifacts feed, which has a NuGet.org upstream.

Azure DevOps pipelines must run `NuGetAuthenticate@1` before the restore. Authentication allows the feed to serve upstream cache misses while the agent remains isolated from NuGet.org.

Run these commands from the root of a pinned `azure-amqp` clone:

```powershell
dotnet restore .\test\TestAmqpBroker\TestAmqpBroker.csproj --configfile .\nuget.cfsclean.config
dotnet build .\test\TestAmqpBroker\TestAmqpBroker.csproj --configuration Debug --framework net10.0 --no-restore
```

SDK pipeline setup should use the same two commands after `NuGetAuthenticate@1`. Keep the clone unchanged and pass `--configfile` on the restore. Normal developer builds continue to use the root `nuget.config` and its NuGet.org source.
