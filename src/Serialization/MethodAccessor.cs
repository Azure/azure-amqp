// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Serialization
{
    using System;
    using System.Reflection;

    delegate object MethodDelegate(object container, object[] parameters);

    abstract class MethodAccessor
    {
        bool isStatic;
        MethodDelegate methodDelegate;

        public static MethodAccessor Create(MethodInfo methodInfo)
        {
            return new TypeMethodAccessor(methodInfo);
        }

        public static MethodAccessor Create(ConstructorInfo constructorInfo)
        {
            return new ConstructorAccessor(constructorInfo);
        }

        public object Invoke(object[] parameters)
        {
            if (!this.isStatic)
            {
                throw new InvalidOperationException("Instance required to call an instance method.");
            }

            return this.Invoke(null, parameters);
        }

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
            public TypeMethodAccessor(MethodInfo methodInfo)
            {
                this.isStatic = methodInfo.IsStatic;
                this.methodDelegate = methodInfo.CreateMethod(this.isStatic);
            }
        }
    }
}
