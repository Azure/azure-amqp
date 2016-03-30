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

    abstract class SerializableType
    {
        readonly AmqpContractSerializer serializer;
        readonly Type type;
        readonly bool hasDefaultCtor;

        protected SerializableType(AmqpContractSerializer serializer, Type type)
        {
            this.serializer = serializer;
            this.type = type;
            this.hasDefaultCtor = type.GetConstructor(Type.EmptyTypes) != null;
#if DNXCORE
            Fx.AssertAndThrow(this.hasDefaultCtor, "CoreCLR support is only implemented for types with default .ctors.");
#endif
        }

        public virtual EncodingType Encoding
        {
            get
            {
                throw new InvalidOperationException();
            }
        }

        public virtual SerialiableMember[] Members
        {
            get
            {
                throw new InvalidOperationException();
            }
        }

        public static SerializableType CreateSingleValueType(Type type)
        {
            // encoder is pre-determined
            EncodingBase encoder = AmqpEncoding.GetEncoding(type);
            return new SingleValueType(type, encoder);
        }

        public static SerializableType CreateDescribedValueType<TValue, TAs>(
            string symbol, Func<TValue, TAs> getter, Func<TAs, TValue> setter)
        {
            // extended .net types, for example:
            //   TimeSpan -> described-long("com.microsoft:timespan", TimeSpan.Ticks)
            return new DescribedValueType<TValue, TAs>(symbol, getter, setter);
        }

        public static SerializableType CreateObjectType(Type type)
        {
            // encoder must be looked up at runtime
            return new AmqpObjectType(type);
        }

        public static SerializableType CreateListType(
            AmqpContractSerializer serializer,
            Type type,
            Type itemType,
            MethodAccessor addAccessor)
        {
            return new ListType(serializer, type, itemType, addAccessor);
        }

        public static SerializableType CreateMapType(
            AmqpContractSerializer serializer,
            Type type,
            MemberAccessor keyAccessor,
            MemberAccessor valueAccessor,
            MethodAccessor addAccessor)
        {
            return new MapType(serializer, type, keyAccessor, valueAccessor, addAccessor);
        }

        public static SerializableType CreateDescribedListType(
            AmqpContractSerializer serializer,
            Type type,
            SerializableType baseType,
            string descriptorName,
            ulong? descriptorCode,
            SerialiableMember[] members,
            Dictionary<Type, SerializableType> knownTypes,
            MethodAccessor onDesrialized)
        {
            return new DescribedListType(serializer, type, baseType, descriptorName,
                descriptorCode, members, knownTypes, onDesrialized);
        }

        public static SerializableType CreateDescribedMapType(
            AmqpContractSerializer serializer,
            Type type,
            SerializableType baseType,
            string descriptorName,
            ulong? descriptorCode,
            SerialiableMember[] members,
            Dictionary<Type, SerializableType> knownTypes,
            MethodAccessor onDesrialized)
        {
            return new DescribedMapType(serializer, type, baseType, descriptorName, descriptorCode,
                members, knownTypes, onDesrialized);
        }

        public static SerializableType CreateAmqpSerializableType(AmqpContractSerializer serializer, Type type)
        {
            return new AmqpSerializableType(serializer, type);
        }

        public abstract void WriteObject(ByteBuffer buffer, object graph);

        public abstract object ReadObject(ByteBuffer buffer);

        sealed class SingleValueType : SerializableType
        {
            readonly EncodingBase encoder;

            public SingleValueType(Type type, EncodingBase encoder)
                : base(null, type)
            {
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

        sealed class AmqpSerializableType : SerializableType
        {
            public AmqpSerializableType(AmqpContractSerializer serializer, Type type)
                : base(serializer, type)
            {
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
#if DNXCORE
                    object container = Activator.CreateInstance(this.type);
#else
                    object container = this.hasDefaultCtor ?
                        Activator.CreateInstance(this.type) :
                        FormatterServices.GetUninitializedObject(this.type);
#endif
                    ((IAmqpSerializable)container).Decode(buffer);
                    return container;
                }
            }
        }

        sealed class DescribedValueType<TValue, TAs> : SerializableType
        {
            readonly AmqpSymbol symbol;
            readonly EncodingBase encoder;
            readonly Func<TValue, TAs> getter;
            readonly Func<TAs, TValue> setter;

            public DescribedValueType(string symbol, Func<TValue, TAs> getter, Func<TAs, TValue> setter)
                : base(null, typeof(TValue))
            {
                this.symbol = symbol;
                this.encoder = AmqpEncoding.GetEncoding(typeof(TAs));
                this.getter = getter;
                this.setter = setter;
            }

            public override void WriteObject(ByteBuffer buffer, object value)
            {
                if (value == null)
                {
                    AmqpEncoding.EncodeNull(buffer);
                }
                else
                {
                    AmqpBitConverter.WriteUByte(buffer, FormatCode.Described);
                    SymbolEncoding.Encode(this.symbol, buffer);
                    this.encoder.EncodeObject(this.getter((TValue)value), false, buffer);
                }
            }

            public override object ReadObject(ByteBuffer buffer)
            {
                AmqpDescribedType describedType = DescribedEncoding.Decode(buffer);
                if (describedType == null)
                {
                    return null;
                }
                else
                {
                    if (!this.symbol.Equals(describedType.Descriptor))
                    {
                        throw new SerializationException(describedType.Descriptor.ToString());
                    }

                    return this.setter((TAs)describedType.Value);
                }
            }
        }

        sealed class AmqpObjectType : SerializableType
        {
            public AmqpObjectType(Type type)
                : base(null, type)
            {
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

        abstract class CollectionType : SerializableType
        {
            protected CollectionType(AmqpContractSerializer serializer, Type type)
                : base(serializer, type)
            {
            }

            public abstract int WriteMembers(ByteBuffer buffer, object container);

            public abstract void ReadMembers(ByteBuffer buffer, object container, ref int count);

            protected abstract bool WriteFormatCode(ByteBuffer buffer);

            protected abstract void Initialize(ByteBuffer buffer, FormatCode formatCode,
                out int size, out int count, out int encodeWidth, out CollectionType effectiveType);

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
                CollectionType effectiveType;
                this.Initialize(buffer, formatCode, out size, out count, out encodeWidth, out effectiveType);
                int offset = buffer.Offset;

#if DNXCORE
                object container = Activator.CreateInstance(effectiveType.type);
#else
                object container = effectiveType.hasDefaultCtor ?
                Activator.CreateInstance(effectiveType.type) :
                    FormatterServices.GetUninitializedObject(effectiveType.type);
#endif

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

        sealed class ListType : CollectionType
        {
            readonly SerializableType itemType;
            readonly MethodAccessor addMethodAccessor;

            public ListType(AmqpContractSerializer serializer, Type type, Type itemType, MethodAccessor addAccessor)
                : base(serializer, type)
            {
                this.itemType = serializer.GetType(itemType);
                this.addMethodAccessor = addAccessor;
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
                out int size, out int count, out int encodeWidth, out CollectionType effectiveType)
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

        sealed class MapType : CollectionType
        {
            readonly SerializableType keyType;
            readonly SerializableType valueType;
            readonly MemberAccessor keyAccessor;
            readonly MemberAccessor valueAccessor;
            readonly MethodAccessor addMethodAccessor;

            public MapType(AmqpContractSerializer serializer, Type type, MemberAccessor keyAccessor,
                MemberAccessor valueAccessor, MethodAccessor addAccessor)
                : base(serializer, type)
            {
                this.keyType = this.serializer.GetType(keyAccessor.Type);
                this.valueType = this.serializer.GetType(valueAccessor.Type);
                this.keyAccessor = keyAccessor;
                this.valueAccessor = valueAccessor;
                this.addMethodAccessor = addAccessor;
            }

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
                out int size, out int count, out int encodeWidth, out CollectionType effectiveType)
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

        abstract class DescribedType : CollectionType
        {
            readonly DescribedType baseType;
            readonly AmqpSymbol descriptorName;
            readonly ulong? descriptorCode;
            readonly SerialiableMember[] members;
            readonly MethodAccessor onDeserialized;
            readonly KeyValuePair<Type, SerializableType>[] knownTypes;

            protected DescribedType(
                AmqpContractSerializer serializer,
                Type type,
                SerializableType baseType,
                string descriptorName,
                ulong? descriptorCode,
                SerialiableMember[] members,
                Dictionary<Type, SerializableType> knownTypes,
                MethodAccessor onDesrialized)
                : base(serializer, type)
            {
                this.baseType = (DescribedType)baseType;
                this.descriptorName = descriptorName;
                this.descriptorCode = descriptorCode;
                this.members = members;
                this.onDeserialized = onDesrialized;
                this.knownTypes = GetKnownTypes(knownTypes);
            }

            public override SerialiableMember[] Members
            {
                get { return this.members; }
            }

            protected abstract byte Code
            {
                get;
            }

            protected DescribedType BaseType
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
                out int size, out int count, out int encodeWidth, out CollectionType effectiveType)
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
                    for (int i = 0; i < this.knownTypes.Length; ++i)
                    {
                        var kvp = this.knownTypes[i];
                        if (kvp.Value == null)
                        {
                            SerializableType knownType = this.serializer.GetType(kvp.Key);
                            this.knownTypes[i] = kvp = new KeyValuePair<Type, SerializableType>(kvp.Key, knownType);
                        }

                        DescribedType describedKnownType = (DescribedType)kvp.Value;
                        if (this.AreEqual(describedKnownType.descriptorCode, describedKnownType.descriptorName, code, symbol))
                        {
                            effectiveType = describedKnownType;
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
            
            static KeyValuePair<Type, SerializableType>[] GetKnownTypes(Dictionary<Type, SerializableType> types)
            {
                if (types == null || types.Count == 0)
                {
                    return null;
                }

                var kt = new KeyValuePair<Type, SerializableType>[types.Count];
                int i = 0;
                foreach (var kvp in types)
                {
                    kt[i++] = kvp;
                }

                return kt;
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

        sealed class DescribedListType : DescribedType
        {
            public DescribedListType(
                AmqpContractSerializer serializer,
                Type type,
                SerializableType baseType,
                string descriptorName,
                ulong? descriptorCode,
                SerialiableMember[] members,
                Dictionary<Type, SerializableType> knownTypes,
                MethodAccessor onDesrialized)
                : base(serializer, type, baseType, descriptorName, descriptorCode, members, knownTypes, onDesrialized)
            {
            }

            public override EncodingType Encoding
            {
                get
                {
                    return EncodingType.List;
                }
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
                        if (memberValue.GetType() != effectiveType.type)
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

        sealed class DescribedMapType : DescribedType
        {
            readonly Dictionary<string, SerialiableMember> membersMap;

            public DescribedMapType(
                AmqpContractSerializer serializer,
                Type type,
                SerializableType baseType,
                string descriptorName,
                ulong? descriptorCode,
                SerialiableMember[] members,
                Dictionary<Type, SerializableType> knownTypes,
                MethodAccessor onDesrialized)
                : base(serializer, type, baseType, descriptorName, descriptorCode, members, knownTypes, onDesrialized)
            {
                this.membersMap = new Dictionary<string, SerialiableMember>();
                foreach(SerialiableMember member in members)
                {
                    this.membersMap.Add(member.Name, member);
                }
            }

            public override EncodingType Encoding
            {
                get
                {
                    return EncodingType.Map;
                }
            }

            protected override byte Code
            {
                get { return FormatCode.Map32; }
            }

            public override int WriteMembers(ByteBuffer buffer, object container)
            {
                int count = 0;
                if (this.BaseType != null)
                {
                    this.BaseType.WriteMembers(buffer, container);
                }

                foreach (SerialiableMember member in this.Members)
                {
                    object memberValue = member.Accessor.Get(container);
                    if (memberValue != null)
                    {
                        AmqpCodec.EncodeSymbol(member.Name, buffer);

                        SerializableType effectiveType = member.Type;
                        if (memberValue.GetType() != effectiveType.type)
                        {
                            effectiveType = this.serializer.GetType(memberValue.GetType());
                        }

                        effectiveType.WriteObject(buffer, memberValue);
                        count += 2;
                    }
                }

                return count;
            }

            public override void ReadMembers(ByteBuffer buffer, object container, ref int count)
            {
                if (this.BaseType != null)
                {
                    this.BaseType.ReadMembers(buffer, container, ref count);
                }

                for (int i = 0; i < this.membersMap.Count && count > 0; ++i, count -= 2)
                {
                    AmqpSymbol key = AmqpCodec.DecodeSymbol(buffer);
                    SerialiableMember member;
                    if (this.membersMap.TryGetValue(key.Value, out member))
                    {
                        object value = member.Type.ReadObject(buffer);
                        member.Accessor.Set(container, value);
                    }
                }

                this.InvokeDeserialized(container);
            }
        }
    }
}
