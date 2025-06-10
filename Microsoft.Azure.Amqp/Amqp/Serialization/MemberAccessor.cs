// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Serialization
{
    using System;
    using System.Reflection;

    public abstract class MemberAccessor
    {
        readonly Type type;
        Func<object, object> getter;
        Action<object, object> setter;

        protected MemberAccessor(Type type)
        {
            this.type = type;
        }

        public Type Type
        {
            get { return this.type; }
        }

#if NET8_0_OR_GREATER
        [System.Diagnostics.CodeAnalysis.RequiresUnreferencedCode(AmqpContractSerializer.TrimWarning)]
        [System.Diagnostics.CodeAnalysis.RequiresDynamicCode(AmqpContractSerializer.AotWarning)]
#endif
        public static MemberAccessor Create(MemberInfo memberInfo, bool requiresSetter)
        {
            FieldInfo fieldInfo;
            PropertyInfo propertyInfo;
            if ((fieldInfo = memberInfo as FieldInfo) != null)
            {
                return new FieldMemberAccessor(fieldInfo);
            }
            else if ((propertyInfo = memberInfo as PropertyInfo) != null)
            {
                return new PropertyMemberAccessor(propertyInfo, requiresSetter);
            }

            throw new NotSupportedException(memberInfo.GetType().ToString());
        }

        public object Get(object container)
        {
            return this.getter(container);
        }

        public void Set(object container, object value)
        {
            this.setter(container, value);
        }

#if NET8_0_OR_GREATER
        [System.Diagnostics.CodeAnalysis.RequiresUnreferencedCode(AmqpContractSerializer.TrimWarning)]
#endif
        sealed class FieldMemberAccessor : MemberAccessor
        {
            public FieldMemberAccessor(FieldInfo fieldInfo)
                : base(fieldInfo.FieldType)
            {
                this.getter = fieldInfo.CreateGetter();
                this.setter = fieldInfo.CreateSetter();
            }
        }

#if NET8_0_OR_GREATER
        [System.Diagnostics.CodeAnalysis.RequiresUnreferencedCode(AmqpContractSerializer.TrimWarning)]
        [System.Diagnostics.CodeAnalysis.RequiresDynamicCode(AmqpContractSerializer.AotWarning)]
#endif
        sealed class PropertyMemberAccessor : MemberAccessor
        {
            public PropertyMemberAccessor(PropertyInfo propertyInfo, bool requiresSetter)
                : base(propertyInfo.PropertyType)
            {
                this.getter = propertyInfo.CreateGetter();
                this.setter = propertyInfo.CreateSetter(requiresSetter);
            }
        }
    }
}