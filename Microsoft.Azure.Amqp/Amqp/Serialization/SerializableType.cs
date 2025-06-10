// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Serialization
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Reflection;
    using System.Runtime.Serialization;
    using Microsoft.Azure.Amqp.Encoding;
    using AmqpDescribedType = Microsoft.Azure.Amqp.Encoding.DescribedType;

    public enum AmqpType
    {
        Primitive,
        Described,
        Composite,
        Serializable,
        List,
        Map,
        Converted,
    }

    public abstract class SerializableType
    {
        readonly AmqpContractSerializer serializer;
#if NET8_0_OR_GREATER
        [System.Diagnostics.CodeAnalysis.DynamicallyAccessedMembers(System.Diagnostics.CodeAnalysis.DynamicallyAccessedMemberTypes.PublicParameterlessConstructor)]
#endif
        readonly Type type;
        readonly bool hasDefaultCtor;

        protected SerializableType(AmqpContractSerializer serializer,
#if NET8_0_OR_GREATER
        [System.Diagnostics.CodeAnalysis.DynamicallyAccessedMembers(System.Diagnostics.CodeAnalysis.DynamicallyAccessedMemberTypes.PublicParameterlessConstructor)]
#endif
            Type type)
        {
            this.serializer = serializer;
            this.type = type;
            this.hasDefaultCtor = type.GetConstructor(Type.EmptyTypes) != null;
        }

        public AmqpType AmqpType
        {
            get;
            protected set;
        }

        public Type UnderlyingType
        {
            get { return this.type; }
        }

        internal bool Final
        {
            get;
            set;
        }

        public static SerializableType CreatePrimitiveType(
#if NET8_0_OR_GREATER
        [System.Diagnostics.CodeAnalysis.DynamicallyAccessedMembers(System.Diagnostics.CodeAnalysis.DynamicallyAccessedMemberTypes.PublicParameterlessConstructor)]
#endif
            Type type)
        {
            // encoder is pre-determined
            EncodingBase encoder = AmqpEncoding.GetEncoding(type);
            return new Primitive(type, encoder);
        }

        public object CreateInstance()
        {
#if NETSTANDARD || NET8_0_OR_GREATER
            return Activator.CreateInstance(this.type);
#else
            return this.hasDefaultCtor ?
                Activator.CreateInstance(this.type) :
                FormatterServices.GetUninitializedObject(this.type);
#endif
        }

        public abstract void WriteObject(ByteBuffer buffer, object graph);

        public abstract object ReadObject(ByteBuffer buffer);

        public sealed class Primitive : SerializableType
        {
            readonly EncodingBase encoder;

            public Primitive(
#if NET8_0_OR_GREATER
        [System.Diagnostics.CodeAnalysis.DynamicallyAccessedMembers(System.Diagnostics.CodeAnalysis.DynamicallyAccessedMemberTypes.PublicParameterlessConstructor)]
#endif
                Type type,
                EncodingBase encoder)
                : base(null, type)
            {
                this.AmqpType = AmqpType.Primitive;
                this.encoder = encoder;
            }

            public override void WriteObject(ByteBuffer buffer, object value)
            {
                this.encoder.EncodeObject(value, false, buffer);
            }

            public override object ReadObject(ByteBuffer buffer)
            {
                return this.encoder.DecodeObject(buffer, 0);
            }
        }

        public sealed class Serializable : SerializableType
        {
            public Serializable(AmqpContractSerializer serializer,
#if NET8_0_OR_GREATER
        [System.Diagnostics.CodeAnalysis.DynamicallyAccessedMembers(System.Diagnostics.CodeAnalysis.DynamicallyAccessedMemberTypes.PublicParameterlessConstructor)]
#endif
                Type type)
                : base(serializer, type)
            {
                this.AmqpType = AmqpType.Serializable;
            }

            public override void WriteObject(ByteBuffer buffer, object value)
            {
                if (value == null)
                {
                    AmqpEncoding.EncodeNull(buffer);
                }
                else
                {
                    ((IAmqpSerializable)value).Encode(buffer);
                }
            }

            public override object ReadObject(ByteBuffer buffer)
            {
                buffer.Validate(false, FixedWidth.FormatCode);
                FormatCode formatCode = buffer.Buffer[buffer.Offset];
                if (formatCode == FormatCode.Null)
                {
                    buffer.Complete(FixedWidth.FormatCode);
                    return null;
                }
                else
                {
                    object container = this.CreateInstance();
                    ((IAmqpSerializable)container).Decode(buffer);
                    return container;
                }
            }
        }

        public sealed class Converted : SerializableType
        {
            readonly EncodingBase encoder;
            readonly Type source;
            readonly Type target;
            readonly Func<object, Type, object> getTarget;
            readonly Func<object, Type, object> getSource;

            public Converted(AmqpType amqpType,
                Type source,
#if NET8_0_OR_GREATER
        [System.Diagnostics.CodeAnalysis.DynamicallyAccessedMembers(System.Diagnostics.CodeAnalysis.DynamicallyAccessedMemberTypes.PublicParameterlessConstructor)]
#endif
                Type target,
                Func<object, Type, object> getTarget, Func<object, Type, object> getSource)
                : base(null, target)
            {
                this.AmqpType = amqpType;
                this.source = source;
                this.target = target;
                this.getTarget = getTarget;
                this.getSource = getSource;
                this.encoder = AmqpEncoding.GetEncoding(target);
            }

            public object GetTarget(object value)
            {
                return this.getTarget(value, this.target);
            }

            public object GetSource(object value)
            {
                return this.getSource(value, this.source);
            }

            public override void WriteObject(ByteBuffer buffer, object value)
            {
                if (value == null)
                {
                    AmqpEncoding.EncodeNull(buffer);
                }
                else
                {
                    this.encoder.EncodeObject(this.getTarget(value, this.target), false, buffer);
                }
            }

            public override object ReadObject(ByteBuffer buffer)
            {
                object value = AmqpEncoding.DecodeObject(buffer);
                if (value == null)
                {
                    return null;
                }
                else
                {
                    return this.GetSource(value);
                }
            }
        }

        public sealed class Object : SerializableType
        {
            public Object(
#if NET8_0_OR_GREATER
        [System.Diagnostics.CodeAnalysis.DynamicallyAccessedMembers(System.Diagnostics.CodeAnalysis.DynamicallyAccessedMemberTypes.PublicParameterlessConstructor)]
#endif
                Type type)
                : base(null, type)
            {
                this.AmqpType = AmqpType.Primitive;
            }

            public override void WriteObject(ByteBuffer buffer, object value)
            {
                AmqpEncoding.EncodeObject(value, buffer);
            }

            public override object ReadObject(ByteBuffer buffer)
            {
                return AmqpEncoding.DecodeObject(buffer);
            }
        }

#if NET8_0_OR_GREATER
        [System.Diagnostics.CodeAnalysis.RequiresUnreferencedCode(AmqpContractSerializer.TrimWarning)]
        [System.Diagnostics.CodeAnalysis.RequiresDynamicCode(AmqpContractSerializer.AotWarning)]
#endif
        public abstract class Collection : SerializableType
        {
            protected Collection(AmqpContractSerializer serializer,
#if NET8_0_OR_GREATER
        [System.Diagnostics.CodeAnalysis.DynamicallyAccessedMembers(System.Diagnostics.CodeAnalysis.DynamicallyAccessedMemberTypes.PublicParameterlessConstructor)]
#endif
                Type type)
                : base(serializer, type)
            {
            }

            public abstract int WriteMembers(ByteBuffer buffer, object container);

            public abstract void ReadMembers(ByteBuffer buffer, object container, ref int count);

            protected abstract bool WriteFormatCode(ByteBuffer buffer);

            protected abstract void Initialize(ByteBuffer buffer, FormatCode formatCode,
                out int size, out int count, out int encodeWidth, out Collection effectiveType);

            public override void WriteObject(ByteBuffer buffer, object graph)
            {
                if (graph == null)
                {
                    AmqpEncoding.EncodeNull(buffer);
                    return;
                }

                if (!this.WriteFormatCode(buffer))
                {
                    return;
                }

                int pos = buffer.WritePos;                    // remember the current position
                AmqpBitConverter.WriteULong(buffer, 0);  // reserve space for size and count

                int count = this.WriteMembers(buffer, graph);

                AmqpBitConverter.WriteUInt(buffer.Buffer, pos, (uint)(buffer.WritePos - pos - FixedWidth.UInt));
                AmqpBitConverter.WriteUInt(buffer.Buffer, pos + FixedWidth.UInt, (uint)count);
            }

            public override object ReadObject(ByteBuffer buffer)
            {
                FormatCode formatCode = AmqpEncoding.ReadFormatCode(buffer);
                if (formatCode == FormatCode.Null)
                {
                    return null;
                }

                int size;
                int count;
                int encodeWidth;
                Collection effectiveType;
                this.Initialize(buffer, formatCode, out size, out count, out encodeWidth, out effectiveType);
                int offset = buffer.Offset;

                object container = effectiveType.CreateInstance();
                if (count > 0)
                {
                    effectiveType.ReadMembers(buffer, container, ref count);

                    if (count > 0)
                    {
                        // skip unknow members: todo: support IExtensibleDataObject
                        buffer.Complete(size - (buffer.Offset - offset) - encodeWidth);
                    }
                }

                return container;
            }
        }

#if NET8_0_OR_GREATER
        [System.Diagnostics.CodeAnalysis.RequiresUnreferencedCode(AmqpContractSerializer.TrimWarning)]
        [System.Diagnostics.CodeAnalysis.RequiresDynamicCode(AmqpContractSerializer.AotWarning)]
#endif
        public sealed class List : Collection
        {
            readonly SerializableType itemType;
            readonly MethodAccessor addMethodAccessor;

            public List(AmqpContractSerializer serializer,
#if NET8_0_OR_GREATER
        [System.Diagnostics.CodeAnalysis.DynamicallyAccessedMembers(System.Diagnostics.CodeAnalysis.DynamicallyAccessedMemberTypes.PublicParameterlessConstructor)]
#endif
                Type type,
#if NET8_0_OR_GREATER
        [System.Diagnostics.CodeAnalysis.DynamicallyAccessedMembers(System.Diagnostics.CodeAnalysis.DynamicallyAccessedMemberTypes.PublicParameterlessConstructor)]
#endif
                Type itemType,
                MethodAccessor addAccessor)
                : base(serializer, type)
            {
                this.AmqpType = AmqpType.List;
                this.itemType = serializer.GetType(itemType);
                this.addMethodAccessor = addAccessor;
            }

            public SerializableType ItemType
            {
                get { return this.itemType; }
            }

            public MethodAccessor Add
            {
                get { return this.addMethodAccessor; }
            }

            protected override bool WriteFormatCode(ByteBuffer buffer)
            {
                AmqpBitConverter.WriteUByte(buffer, FormatCode.List32);
                return true;
            }

            public override int WriteMembers(ByteBuffer buffer, object container)
            {
                int count = 0;
                foreach (object item in (IEnumerable)container)
                {
                    if (item == null)
                    {
                        AmqpEncoding.EncodeNull(buffer);
                    }
                    else
                    {
                        SerializableType effectiveType = this.itemType;
                        if (item.GetType() != effectiveType.type)
                        {
                            effectiveType = this.serializer.GetType(item.GetType());
                        }
                        effectiveType.WriteObject(buffer, item);
                    }
                    ++count;
                }
                return count;
            }

            protected override void Initialize(ByteBuffer buffer, FormatCode formatCode,
                out int size, out int count, out int encodeWidth, out Collection effectiveType)
            {
                if (formatCode == FormatCode.List0)
                {
                    size = count = encodeWidth = 0;
                    effectiveType = this;
                    return;
                }

                if (formatCode != FormatCode.List32 && formatCode != FormatCode.List8)
                {
                    throw new AmqpException(AmqpErrorCode.InvalidField, AmqpResources.GetString(AmqpResources.AmqpInvalidFormatCode, formatCode, buffer.Offset));
                }

                encodeWidth = formatCode == FormatCode.List8 ? FixedWidth.UByte : FixedWidth.UInt;
                AmqpEncoding.ReadSizeAndCount(buffer, formatCode, FormatCode.List8, FormatCode.List32, out size, out count);
                effectiveType = this;
            }

            public override void ReadMembers(ByteBuffer buffer, object container, ref int count)
            {
                for (; count > 0; count--)
                {
                    object value = this.itemType.ReadObject(buffer);
                    this.addMethodAccessor.Invoke(container, new object[] { value });
                }
            }
        }

#if NET8_0_OR_GREATER
        [System.Diagnostics.CodeAnalysis.RequiresUnreferencedCode(AmqpContractSerializer.TrimWarning)]
        [System.Diagnostics.CodeAnalysis.RequiresDynamicCode(AmqpContractSerializer.AotWarning)]
#endif
        public sealed class Map : Collection
        {
            readonly SerializableType keyType;
            readonly SerializableType valueType;
            readonly MemberAccessor keyAccessor;
            readonly MemberAccessor valueAccessor;
            readonly MethodAccessor addMethodAccessor;

            public Map(AmqpContractSerializer serializer, Type type, MemberAccessor keyAccessor,
                MemberAccessor valueAccessor, MethodAccessor addAccessor)
                : base(serializer, type)
            {
                this.AmqpType = AmqpType.Map;
                this.keyType = this.serializer.GetType(keyAccessor.Type);
                this.valueType = this.serializer.GetType(valueAccessor.Type);
                this.keyAccessor = keyAccessor;
                this.valueAccessor = valueAccessor;
                this.addMethodAccessor = addAccessor;
            }

            public MemberAccessor Key { get { return this.keyAccessor; } }

            public MemberAccessor Value { get { return this.valueAccessor; } }

            public MethodAccessor Add { get { return this.addMethodAccessor; } }

            protected override bool WriteFormatCode(ByteBuffer buffer)
            {
                AmqpBitConverter.WriteUByte(buffer, FormatCode.Map32);
                return true;
            }

            public override int WriteMembers(ByteBuffer buffer, object container)
            {
                int count = 0;
                foreach (object item in (IEnumerable)container)
                {
                    object key = this.keyAccessor.Get(item);
                    object value = this.valueAccessor.Get(item);
                    if (value != null)
                    {
                        this.keyType.WriteObject(buffer, key);

                        SerializableType effectiveType = this.valueType;
                        if (value.GetType() != effectiveType.type)
                        {
                            effectiveType = this.serializer.GetType(value.GetType());
                        }
                        effectiveType.WriteObject(buffer, value);

                        count += 2;
                    }
                }
                return count;
            }

            protected override void Initialize(ByteBuffer buffer, FormatCode formatCode,
                out int size, out int count, out int encodeWidth, out Collection effectiveType)
            {
                if (formatCode != FormatCode.Map32 && formatCode != FormatCode.Map8)
                {
                    throw new AmqpException(AmqpErrorCode.InvalidField, AmqpResources.GetString(AmqpResources.AmqpInvalidFormatCode, formatCode, buffer.Offset));
                }

                encodeWidth = formatCode == FormatCode.Map8 ? FixedWidth.UByte : FixedWidth.UInt;
                AmqpEncoding.ReadSizeAndCount(buffer, formatCode, FormatCode.Map8, FormatCode.Map32, out size, out count);
                effectiveType = this;
            }

            public override void ReadMembers(ByteBuffer buffer, object container, ref int count)
            {
                for (; count > 0; count -= 2)
                {
                    object key = this.keyType.ReadObject(buffer);
                    object value = this.valueType.ReadObject(buffer);
                    this.addMethodAccessor.Invoke(container, new object[] { key, value });
                }
            }
        }

#if NET8_0_OR_GREATER
        [System.Diagnostics.CodeAnalysis.RequiresUnreferencedCode(AmqpContractSerializer.TrimWarning)]
        [System.Diagnostics.CodeAnalysis.RequiresDynamicCode(AmqpContractSerializer.AotWarning)]
#endif
        public abstract class Composite : Collection
        {
            readonly Composite baseType;
            readonly AmqpSymbol descriptorName;
            readonly ulong? descriptorCode;
            readonly SerialiableMember[] members;
            readonly MethodAccessor onDeserialized;
            IList<Type> knownTypes;

            protected Composite(
                AmqpContractSerializer serializer,
                Type type,
                SerializableType baseType,
                string descriptorName,
                ulong? descriptorCode,
                SerialiableMember[] members,
                IList<Type> knownTypes,
                MethodAccessor onDesrialized)
                : base(serializer, type)
            {
                this.AmqpType = AmqpType.Composite;
                this.baseType = (Composite)baseType;
                this.descriptorName = descriptorName;
                this.descriptorCode = descriptorCode;
                this.members = members;
                this.onDeserialized = onDesrialized;
                this.knownTypes = knownTypes;
            }

            public AmqpSymbol Name
            {
                get { return this.descriptorName; }
            }

            public SerialiableMember[] Members
            {
                get { return this.members; }
            }

            public EncodingType EncodingType
            {
                get;
                protected set;
            }

            public IList<Type> KnownTypes
            {
                get { return this.knownTypes; }
            }

            protected abstract byte Code
            {
                get;
            }

            protected Composite BaseType
            {
                get { return this.baseType; }
            }

            protected override bool WriteFormatCode(ByteBuffer buffer)
            {
                AmqpBitConverter.WriteUByte(buffer, (byte)FormatCode.Described);
                if (this.descriptorCode != null)
                {
                    ULongEncoding.Encode(this.descriptorCode, buffer);
                }
                else
                {
                    SymbolEncoding.Encode(this.descriptorName, buffer);
                }

                AmqpBitConverter.WriteUByte(buffer, this.Code);
                return true;
            }

            protected override void Initialize(ByteBuffer buffer, FormatCode formatCode,
                out int size, out int count, out int encodeWidth, out Collection effectiveType)
            {
                if (formatCode != FormatCode.Described)
                {
                    throw new AmqpException(AmqpErrorCode.InvalidField, AmqpResources.GetString(AmqpResources.AmqpInvalidFormatCode, formatCode, buffer.Offset));
                }

                effectiveType = null;
                formatCode = AmqpEncoding.ReadFormatCode(buffer);
                ulong? code = null;
                AmqpSymbol symbol = default(AmqpSymbol);
                if (formatCode == FormatCode.ULong0)
                {
                    code = 0;
                }
                else if (formatCode == FormatCode.ULong || formatCode == FormatCode.SmallULong)
                {
                    code = ULongEncoding.Decode(buffer, formatCode);
                }
                else if (formatCode == FormatCode.Symbol8 || formatCode == FormatCode.Symbol32)
                {
                    symbol = SymbolEncoding.Decode(buffer, formatCode);
                }

                if (this.AreEqual(this.descriptorCode, this.descriptorName, code, symbol))
                {
                    effectiveType = this;
                }
                else if (this.knownTypes != null)
                {
                    for (int i = 0; i < this.knownTypes.Count; ++i)
                    {
                        Composite knownType = (Composite)this.serializer.GetType(this.knownTypes[i]);
                        if (this.AreEqual(knownType.descriptorCode, knownType.descriptorName, code, symbol))
                        {
                            effectiveType = knownType;
                            break;
                        }
                    }
                }

                if (effectiveType == null)
                {
                    throw new SerializationException(AmqpResources.GetString(AmqpResources.AmqpUnknownDescriptor, code ?? (object)symbol.Value, this.type.Name));
                }

                formatCode = AmqpEncoding.ReadFormatCode(buffer);
                if (this.Code == FormatCode.List32)
                {
                    if (formatCode == FormatCode.List0)
                    {
                        size = count = encodeWidth = 0;
                    }
                    else
                    {
                        encodeWidth = formatCode == FormatCode.List8 ? FixedWidth.UByte : FixedWidth.UInt;
                        AmqpEncoding.ReadSizeAndCount(buffer, formatCode, FormatCode.List8,
                            FormatCode.List32, out size, out count);
                    }
                }
                else
                {
                    encodeWidth = formatCode == FormatCode.Map8 ? FixedWidth.UByte : FixedWidth.UInt;
                    AmqpEncoding.ReadSizeAndCount(buffer, formatCode, FormatCode.Map8,
                        FormatCode.Map32, out size, out count);
                }
            }

            protected void InvokeDeserialized(object container)
            {
                if (this.baseType != null)
                {
                    this.baseType.InvokeDeserialized(container);
                }

                if (this.onDeserialized != null)
                {
                    this.onDeserialized.Invoke(container, new object[] { default(StreamingContext) });
                }
            }

            bool AreEqual(ulong? code1, AmqpSymbol symbol1, ulong? code2, AmqpSymbol symbol2)
            {
                if (code1 != null && code2 != null)
                {
                    return code1.Value == code2.Value;
                }

                if (symbol1.Value != null && symbol2.Value != null)
                {
                    return symbol1.Value == symbol2.Value;
                }

                return false;
            }
        }

#if NET8_0_OR_GREATER
        [System.Diagnostics.CodeAnalysis.RequiresUnreferencedCode(AmqpContractSerializer.TrimWarning)]
        [System.Diagnostics.CodeAnalysis.RequiresDynamicCode(AmqpContractSerializer.AotWarning)]
#endif
        public sealed class CompositeList : Composite
        {
            public CompositeList(
                AmqpContractSerializer serializer,
                Type type,
                SerializableType baseType,
                string descriptorName,
                ulong? descriptorCode,
                SerialiableMember[] members,
                IList<Type> knownTypes,
                MethodAccessor onDesrialized)
                : base(serializer, type, baseType, descriptorName, descriptorCode, members, knownTypes, onDesrialized)
            {
                this.EncodingType = EncodingType.List;
            }

            protected override byte Code
            {
                get { return FormatCode.List32; }
            }

            public override int WriteMembers(ByteBuffer buffer, object container)
            {
                foreach (SerialiableMember member in this.Members)
                {
                    object memberValue = member.Accessor.Get(container);
                    if (memberValue == null)
                    {
                        AmqpEncoding.EncodeNull(buffer);
                    }
                    else
                    {
                        SerializableType effectiveType = member.Type;
                        if (!effectiveType.Final && memberValue.GetType() != effectiveType.type)
                        {
                            effectiveType = this.serializer.GetType(memberValue.GetType());
                        }

                        effectiveType.WriteObject(buffer, memberValue);
                    }
                }

                return this.Members.Length;
            }

            public override void ReadMembers(ByteBuffer buffer, object container, ref int count)
            {
                for (int i = 0; i < this.Members.Length && count > 0; ++i, --count)
                {
                    object value = this.Members[i].Type.ReadObject(buffer);
                    this.Members[i].Accessor.Set(container, value);
                }

                this.InvokeDeserialized(container);
            }
        }

#if NET8_0_OR_GREATER
        [System.Diagnostics.CodeAnalysis.RequiresUnreferencedCode(AmqpContractSerializer.TrimWarning)]
        [System.Diagnostics.CodeAnalysis.RequiresDynamicCode(AmqpContractSerializer.AotWarning)]
#endif
        public sealed class CompositeMap : Composite
        {
            public CompositeMap(
                AmqpContractSerializer serializer,
                Type type,
                SerializableType baseType,
                string descriptorName,
                ulong? descriptorCode,
                SerialiableMember[] members,
                IList<Type> knownTypes,
                MethodAccessor onDesrialized)
                : base(serializer, type, baseType, descriptorName, descriptorCode, members, knownTypes, onDesrialized)
            {
                this.EncodingType = EncodingType.Map;
            }

            protected override byte Code
            {
                get { return FormatCode.Map32; }
            }

            public override int WriteMembers(ByteBuffer buffer, object container)
            {
                int count = 0;
                foreach (SerialiableMember member in this.Members)
                {
                    AmqpCodec.EncodeSymbol(member.Name, buffer);

                    object memberValue = member.Accessor.Get(container);
                    SerializableType effectiveType = member.Type;
                    if (!effectiveType.Final && memberValue.GetType() != effectiveType.type)
                    {
                        effectiveType = this.serializer.GetType(memberValue.GetType());
                    }

                    effectiveType.WriteObject(buffer, memberValue);
                    count += 2;
                }

                return count;
            }

            public override void ReadMembers(ByteBuffer buffer, object container, ref int count)
            {
                for (int i = 0; i < this.Members.Length && count > 0; ++i, count -= 2)
                {
                    AmqpSymbol key = AmqpCodec.DecodeSymbol(buffer);
                    object value = this.Members[i].Type.ReadObject(buffer);
                    this.Members[i].Accessor.Set(container, value);
                }

                this.InvokeDeserialized(container);
            }
        }
    }
}
