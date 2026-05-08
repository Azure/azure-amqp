// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Serialization
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.Reflection;

    /// <summary>
    /// Provides access to a member of a type.
    /// </summary>
    public abstract class MemberAccessor
    {
        readonly Type type;
        Func<object, object> getter;
        Action<object, object> setter;

        /// <summary>
        /// Initializes a new instance with the specified type.
        /// </summary>
        /// <param name="type">The member type.</param>
        protected MemberAccessor(Type type)
        {
            this.type = type;
        }

        /// <summary>Gets the member type.</summary>
        public Type Type
        {
            get { return this.type; }
        }

        /// <summary>
        /// Creates a <see cref="MemberAccessor"/> for the specified member.
        /// </summary>
        /// <param name="memberInfo">The field or property info.</param>
        /// <param name="requiresSetter">true if a setter is required.</param>
        /// <returns>A <see cref="MemberAccessor"/> instance.</returns>
        [RequiresUnreferencedCode(AmqpContractSerializer.TrimWarning)]
        [RequiresDynamicCode(AmqpContractSerializer.AotWarning)]
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

        /// <summary>Gets the member value from the container object.</summary>
        /// <param name="container">The object containing the member.</param>
        /// <returns>The member value.</returns>
        public object Get(object container)
        {
            return this.getter(container);
        }

        /// <summary>Sets the member value on the container object.</summary>
        /// <param name="container">The object containing the member.</param>
        /// <param name="value">The value to set.</param>
        public void Set(object container, object value)
        {
            this.setter(container, value);
        }

        [RequiresUnreferencedCode(AmqpContractSerializer.TrimWarning)]
        sealed class FieldMemberAccessor : MemberAccessor
        {
            public FieldMemberAccessor(FieldInfo fieldInfo)
                : base(fieldInfo.FieldType)
            {
                this.getter = fieldInfo.CreateGetter();
                this.setter = fieldInfo.CreateSetter();
            }
        }

        [RequiresUnreferencedCode(AmqpContractSerializer.TrimWarning)]
        [RequiresDynamicCode(AmqpContractSerializer.AotWarning)]
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
