// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Serialization
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.Reflection;

    delegate object MethodDelegate(object container, object[] parameters);

    /// <summary>
    /// Provides access to a method of a type.
    /// </summary>
    public abstract class MethodAccessor
    {
        bool isStatic;
        MethodDelegate methodDelegate;

        /// <summary>Creates a <see cref="MethodAccessor"/> for the specified method.</summary>
        /// <param name="methodInfo">The method info.</param>
        /// <returns>A <see cref="MethodAccessor"/> instance.</returns>
        [RequiresDynamicCode(AmqpContractSerializer.AotWarning)]
        public static MethodAccessor Create(MethodInfo methodInfo)
        {
            return new TypeMethodAccessor(methodInfo);
        }

        /// <summary>Creates a <see cref="MethodAccessor"/> for the specified constructor.</summary>
        /// <param name="constructorInfo">The constructor info.</param>
        /// <returns>A <see cref="MethodAccessor"/> instance.</returns>
        public static MethodAccessor Create(ConstructorInfo constructorInfo)
        {
            return new ConstructorAccessor(constructorInfo);
        }

        /// <summary>Invokes a static method with the specified parameters.</summary>
        /// <param name="parameters">The method parameters.</param>
        /// <returns>The return value.</returns>
        public object Invoke(object[] parameters)
        {
            if (!this.isStatic)
            {
                throw new InvalidOperationException("Instance required to call an instance method.");
            }

            return this.Invoke(null, parameters);
        }

        /// <summary>Invokes the method on the specified instance with the given parameters.</summary>
        /// <param name="container">The object instance, or null for static methods.</param>
        /// <param name="parameters">The method parameters.</param>
        /// <returns>The return value.</returns>
        public object Invoke(object container, object[] parameters)
        {
            if (this.isStatic && container != null)
            {
                throw new InvalidOperationException("Static method must be called with null instance.");
            }

            return this.methodDelegate(container, parameters);
        }

        sealed class ConstructorAccessor : MethodAccessor
        {
            public ConstructorAccessor(ConstructorInfo constructorInfo)
            {
                this.isStatic = true;
                this.methodDelegate = constructorInfo.CreateMethod();
            }
        }

        sealed class TypeMethodAccessor : MethodAccessor
        {
            [RequiresDynamicCode(AmqpContractSerializer.AotWarning)]
            public TypeMethodAccessor(MethodInfo methodInfo)
            {
                this.isStatic = methodInfo.IsStatic;
                this.methodDelegate = methodInfo.CreateMethod(this.isStatic);
            }
        }
    }
}
