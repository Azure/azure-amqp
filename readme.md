# Microsoft Azure AMQP for .Net

[![Build status](https://ci.appveyor.com/api/projects/status/the7eqq0ixf0hcx3?svg=true)](https://ci.appveyor.com/project/xinchen10/azure-amqp)    [![NuGet Version and Downloads count](https://buildstats.info/nuget/Microsoft.Azure.Amqp)](https://www.nuget.org/packages/Microsoft.Azure.Amqp/)

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
