// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Serialization
{
    using System;
    using System.Collections.Concurrent;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Reflection;
    using System.Runtime.Serialization;
    using Microsoft.Azure.Amqp.Encoding;

    sealed class AmqpContractSerializer
    {
        static readonly Dictionary<Type, SerializableType> builtInTypes = new Dictionary<Type, SerializableType>()
        {
            { typeof(bool),     SerializableType.CreatePrimitiveType(typeof(bool)) },
            { typeof(byte),     SerializableType.CreatePrimitiveType(typeof(byte)) },
            { typeof(ushort),   SerializableType.CreatePrimitiveType(typeof(ushort)) },
            { typeof(uint),     SerializableType.CreatePrimitiveType(typeof(uint)) },
            { typeof(ulong),    SerializableType.CreatePrimitiveType(typeof(ulong)) },
            { typeof(sbyte),    SerializableType.CreatePrimitiveType(typeof(sbyte)) },
            { typeof(short),    SerializableType.CreatePrimitiveType(typeof(short)) },
            { typeof(int),      SerializableType.CreatePrimitiveType(typeof(int)) },
            { typeof(long),     SerializableType.CreatePrimitiveType(typeof(long)) },
            { typeof(float),    SerializableType.CreatePrimitiveType(typeof(float)) },
            { typeof(double),   SerializableType.CreatePrimitiveType(typeof(double)) },
            { typeof(decimal),  SerializableType.CreatePrimitiveType(typeof(decimal)) },
            { typeof(char),     SerializableType.CreatePrimitiveType(typeof(char)) },
            { typeof(DateTime), SerializableType.CreatePrimitiveType(typeof(DateTime)) },
            { typeof(Guid),     SerializableType.CreatePrimitiveType(typeof(Guid)) },
            { typeof(ArraySegment<byte>), SerializableType.CreatePrimitiveType(typeof(ArraySegment<byte>)) },
            { typeof(string),   SerializableType.CreatePrimitiveType(typeof(string)) },
            { typeof(AmqpSymbol), SerializableType.CreatePrimitiveType(typeof(AmqpSymbol)) },
            { typeof(object),   new SerializableType.Object(typeof(object)) },
        };

        static readonly AmqpContractSerializer Instance = new AmqpContractSerializer();
        readonly ConcurrentDictionary<Type, SerializableType> customTypeCache;

        static AmqpContractSerializer()
        {
            // register extended .NET types
            builtInTypes[typeof(TimeSpan)] = new SerializableType.Converted(
                AmqpType.Described,
                typeof(TimeSpan),
                typeof(DescribedType),
                o => new DescribedType(AmqpConstants.TimeSpanName, ((TimeSpan)o).Ticks),
                o => TimeSpan.FromTicks((long)((DescribedType)o).Value));
            builtInTypes[typeof(Uri)] = new SerializableType.Converted(
                AmqpType.Described,
                typeof(Uri),
                typeof(DescribedType),
                o => new DescribedType(AmqpConstants.UriName, ((Uri)o).AbsoluteUri),
                o => new Uri((string)((DescribedType)o).Value));
            builtInTypes[typeof(DateTimeOffset)] = new SerializableType.Converted(
                AmqpType.Described,
                typeof(DateTimeOffset),
                typeof(DescribedType),
                o => new DescribedType(AmqpConstants.DateTimeOffsetName, ((DateTimeOffset)o).UtcTicks),
                o => new DateTimeOffset(new DateTime((long)((DescribedType)o).Value, DateTimeKind.Utc)));
        }

        internal AmqpContractSerializer()
        {
            this.customTypeCache = new ConcurrentDictionary<Type, SerializableType>();
        }

        public static void WriteObject(Stream stream, object graph)
        {
            Instance.WriteObjectInternal(stream, graph);
        }

        public static T ReadObject<T>(Stream stream)
        {
            return Instance.ReadObjectInternal<T, T>(stream);
        }

        public static TAs ReadObject<T, TAs>(Stream stream)
        {
            return Instance.ReadObjectInternal<T, TAs>(stream);
        }

        internal void WriteObjectInternal(Stream stream, object graph)
        {
            if (graph == null)
            {
                stream.WriteByte(FormatCode.Null);
            }
            else
            {
                SerializableType type = this.GetType(graph.GetType());
                using (ByteBuffer buffer = new ByteBuffer(1024, true))
                {
                    type.WriteObject(buffer, graph);
                    stream.Write(buffer.Buffer, buffer.Offset, buffer.Length);
                }
            }
        }

        internal void WriteObjectInternal(ByteBuffer buffer, object graph)
        {
            if (graph == null)
            {
                AmqpEncoding.EncodeNull(buffer);
            }
            else
            {
                SerializableType type = this.GetType(graph.GetType());
                type.WriteObject(buffer, graph);
            }
        }

        internal T ReadObjectInternal<T>(Stream stream)
        {
            return this.ReadObjectInternal<T, T>(stream);
        }

        internal TAs ReadObjectInternal<T, TAs>(Stream stream)
        {
            if (!stream.CanSeek)
            {
                throw new AmqpException(AmqpErrorCode.DecodeError, "stream.CanSeek must be true.");
            }

            SerializableType type = this.GetType(typeof(T));
            ByteBuffer buffer = null;
            long position = stream.Position;
            BufferListStream listStream = stream as BufferListStream;
            if (listStream != null)
            {
                ArraySegment<byte> segment = listStream.ReadBytes(int.MaxValue);
                buffer = new ByteBuffer(segment.Array, segment.Offset, segment.Count);
            }
            else
            {
                buffer = new ByteBuffer((int)stream.Length, false);
                int bytes = stream.Read(buffer.Buffer, 0, buffer.Capacity);
                buffer.Append(bytes);
            }

            using (buffer)
            {
                TAs value = (TAs)type.ReadObject(buffer);
                if (buffer.Length > 0)
                {
                    stream.Position = position + buffer.Offset;
                }

                return value;
            }
        }

        internal TAs ReadObjectInternal<T, TAs>(ByteBuffer buffer)
        {
            SerializableType type = this.GetType(typeof(T));
            return (TAs)type.ReadObject(buffer);
        }

        internal SerializableType GetType(Type type)
        {
            return this.GetOrCompileType(type, false);
        }

        bool TryGetSerializableType(Type type, out SerializableType serializableType)
        {
            serializableType = null;
            if (builtInTypes.TryGetValue(type, out serializableType))
            {
                return true;
            }
            else if (this.customTypeCache.TryGetValue(type, out serializableType))
            {
                return true;
            }

            return false;
        }

        SerializableType GetOrCompileType(Type type, bool describedOnly)
        {
            SerializableType serialiableType = null;
            if (!this.TryGetSerializableType(type, out serialiableType))
            {
                serialiableType = this.CompileType(type, describedOnly);
                if (serialiableType != null)
                {
                    this.customTypeCache.TryAdd(type, serialiableType);
                }
            }

            if (serialiableType == null)
            {
                throw new NotSupportedException(type.FullName);
            }

            return serialiableType;
        }

        SerializableType CompileType(Type type, bool describedOnly)
        {
            var typeAttributes = type.GetTypeInfo().GetCustomAttributes(typeof(AmqpContractAttribute), false);
            if (!typeAttributes.Any())
            {
                if (describedOnly)
                {
                    return null;
                }
                else
                {
                    return CompileNonContractTypes(type);
                }
            }

            AmqpContractAttribute contractAttribute = (AmqpContractAttribute)typeAttributes.First();
            SerializableType.Composite baseType = null;
            if (type.GetTypeInfo().BaseType != typeof(object))
            {
                var baseSerializableType = this.CompileType(type.GetTypeInfo().BaseType, true);
                if (baseSerializableType != null)
                {
                    if (baseSerializableType.AmqpType != AmqpType.Composite)
                    {
                        throw new SerializationException(AmqpResources.GetString(AmqpResources.AmqpInvalidType, baseType.GetType().Name));
                    }

                    baseType = (SerializableType.Composite)baseSerializableType;
                    if (baseType.EncodingType != contractAttribute.Encoding)
                    {
                        throw new SerializationException(AmqpResources.GetString(AmqpResources.AmqpEncodingTypeMismatch, type.Name, contractAttribute.Encoding, type.GetTypeInfo().BaseType.Name, baseType.EncodingType));
                    }

                    this.customTypeCache.TryAdd(type.GetTypeInfo().BaseType, baseType);
                }
            }

            string descriptorName = contractAttribute.Name;
            ulong? descriptorCode = contractAttribute.InternalCode;
            if (descriptorName == null && descriptorCode == null)
            {
                descriptorName = type.FullName;
            }

            List<SerialiableMember> memberList = new List<SerialiableMember>();
            if (baseType != null)
            {
                memberList.AddRange(baseType.Members);
            }

            int lastOrder = memberList.Count + 1;
            MemberInfo[] memberInfos = type.GetMembers(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance);
            MethodAccessor onDeserialized = null;
            foreach (MemberInfo memberInfo in memberInfos)
            {
                if (memberInfo.DeclaringType != type)
                {
                    continue;
                }

                if (memberInfo is FieldInfo ||
                    memberInfo is PropertyInfo)
                {
                    var memberAttributes = memberInfo.GetCustomAttributes(typeof(AmqpMemberAttribute), true);
                    if (memberAttributes.Count() != 1)
                    {
                        continue;
                    }

                    AmqpMemberAttribute attribute = (AmqpMemberAttribute)memberAttributes.First();

                    SerialiableMember member = new SerialiableMember();
                    member.Name = attribute.Name ?? memberInfo.Name;
                    member.Order = attribute.InternalOrder ?? lastOrder++;
                    member.Mandatory = attribute.Mandatory;
                    member.Accessor = MemberAccessor.Create(memberInfo, true);

                    // This will recursively resolve member types
                    Type memberType = memberInfo is FieldInfo ? ((FieldInfo)memberInfo).FieldType : ((PropertyInfo)memberInfo).PropertyType;
                    member.Type = GetType(memberType);

                    memberList.Add(member);
                }
                else if (memberInfo is MethodInfo)
                {
                    var memberAttributes = memberInfo.GetCustomAttributes(typeof(OnDeserializedAttribute), false);
                    if (memberAttributes.Count() == 1)
                    {
                        onDeserialized = MethodAccessor.Create((MethodInfo)memberInfo);
                    }
                }
            }

            if (contractAttribute.Encoding == EncodingType.List)
            {
                memberList.Sort(MemberOrderComparer.Instance);
                int order = -1;
                foreach (SerialiableMember member in memberList)
                {
                    if (order > 0 && member.Order == order)
                    {
                        throw new SerializationException(AmqpResources.GetString(AmqpResources.AmqpDuplicateMemberOrder, order, type.Name));
                    }

                    order = member.Order;
                }
            }

            SerialiableMember[] members = memberList.ToArray();

            Dictionary<Type, SerializableType> knownTypes = null;
            foreach (object o in type.GetTypeInfo().GetCustomAttributes(typeof(KnownTypeAttribute), false))
            {
                KnownTypeAttribute knownAttribute = (KnownTypeAttribute)o;
                if (knownAttribute.Type.GetTypeInfo().GetCustomAttributes(typeof(AmqpContractAttribute), false).Any())
                {
                    if (knownTypes == null)
                    {
                        knownTypes = new Dictionary<Type, SerializableType>();
                    }

                    // KnownType compilation is delayed and non-recursive to avoid circular references
                    knownTypes.Add(knownAttribute.Type, null);
                }
            }

            if (contractAttribute.Encoding == EncodingType.List)
            {
                return new SerializableType.CompositeList(this, type, baseType, descriptorName,
                    descriptorCode, members, knownTypes, onDeserialized);
            }
            else if (contractAttribute.Encoding == EncodingType.Map)
            {
                return new SerializableType.CompositeMap(this, type, baseType, descriptorName,
                    descriptorCode, members, knownTypes, onDeserialized);
            }
            else
            {
                throw new NotSupportedException(contractAttribute.Encoding.ToString());
            }
        }

        SerializableType CompileNonContractTypes(Type type)
        {
            return this.CompileNullableTypes(type) ?? this.CompileInterfaceTypes(type);
        }

        SerializableType CompileNullableTypes(Type type)
        {
            if (type.GetTypeInfo().IsGenericType &&
                type.GetGenericTypeDefinition() == typeof(Nullable<>))
            {
                Type[] argTypes = type.GetGenericArguments();
                Fx.Assert(argTypes.Length == 1, "Nullable type must have one argument");
                return this.GetType(argTypes[0]);
            }

            return null;
        }

        SerializableType CompileInterfaceTypes(Type type)
        {
            if (type.IsArray)
            {
                // validate item type to be AMQP types only
                AmqpEncoding.GetEncoding(type.GetElementType());
                return SerializableType.CreatePrimitiveType(type);
            }

            bool isArray = type.IsArray;
            bool isMap = false;
            bool isList = false;
            MemberAccessor keyAccessor = null;
            MemberAccessor valueAccessor = null;
            MethodAccessor addAccess = null;
            Type itemType = null;

            if (type.GetInterfaces().FirstOrDefault(i => i == typeof(IAmqpSerializable)) != null)
            {
                return new SerializableType.Serializable(this, type);
            }

            foreach (Type it in type.GetInterfaces())
            {
                if (it.GetTypeInfo().IsGenericType)
                {
                    Type genericTypeDef = it.GetGenericTypeDefinition();
                    if (genericTypeDef == typeof(IDictionary<,>))
                    {
                        isMap = true;
                        Type[] argTypes = it.GetGenericArguments();
                        itemType = typeof(KeyValuePair<,>).MakeGenericType(argTypes);
                        keyAccessor = MemberAccessor.Create(itemType.GetProperty("Key"), false);
                        valueAccessor = MemberAccessor.Create(itemType.GetProperty("Value"), false);
                        addAccess = MethodAccessor.Create(type.GetMethod("Add", argTypes));
                        break;
                    }
                    else if (genericTypeDef == typeof(IList<>))
                    {
                        isList = true;
                        Type[] argTypes = it.GetGenericArguments();
                        itemType = argTypes[0];
                        addAccess = MethodAccessor.Create(type.GetMethod("Add", argTypes));
                        break;
                    }
                }
            }

            if (isMap)
            {
                return new SerializableType.Map(this, type, keyAccessor, valueAccessor, addAccess);
            }
            else if (isList)
            {
                return new SerializableType.List(this, type, itemType, addAccess);
            }

            return null;
        }

        sealed class MemberOrderComparer : IComparer<SerialiableMember>
        {
            public static readonly MemberOrderComparer Instance = new MemberOrderComparer();

            public int Compare(SerialiableMember m1, SerialiableMember m2)
            {
                return m1.Order == m2.Order ? 0 : (m1.Order > m2.Order ? 1 : -1);
            }
        }
    }
}
