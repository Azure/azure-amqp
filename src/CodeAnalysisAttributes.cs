// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#nullable enable

namespace System.Diagnostics.CodeAnalysis;

#if NETSTANDARD2_0
/// <summary>
/// Indicates that a method does not return.
/// </summary>
[AttributeUsage(AttributeTargets.Method, Inherited = false)]
internal sealed class DoesNotReturnAttribute : Attribute
{
}
#endif
