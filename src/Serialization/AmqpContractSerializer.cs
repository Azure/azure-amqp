// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Serialization
{
    using System;
    using System.Collections;
    using System.Collections.Concurrent;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Reflection;
    using System.Runtime.Serialization;
    using Microsoft.Azure.Amqp.Encoding;

    /// <summary>
    /// An serializer that uses AMQP type system to serialize .Net objects.
    /// </summary>
    public sealed class AmqpContractSerializer
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
        readonly List<Func<Type, SerializableType>> externalCompilers;

        static AmqpContractSerializer()
        {
            // register extended .NET types
            builtInTypes[typeof(TimeSpan)] = new SerializableType.Converted(
                AmqpType.Converted,
                typeof(TimeSpan),
                typeof(DescribedType),
                (o, t) => new DescribedType(AmqpConstants.TimeSpanName, ((TimeSpan)o).Ticks),
                (o, t) => TimeSpan.FromTicks((long)((DescribedType)o).Value));
            builtInTypes[typeof(Uri)] = new SerializableType.Converted(
                AmqpType.Converted,
                typeof(Uri),
                typeof(DescribedType),
                (o, t) => new DescribedType(AmqpConstants.UriName, ((Uri)o).AbsoluteUri),
                (o, t) => new Uri((string)((DescribedType)o).Value));
            builtInTypes[typeof(DateTimeOffset)] = new SerializableType.Converted(
                AmqpType.Converted,
                typeof(DateTimeOffset),
                typeof(DescribedType),
                (o, t) => new DescribedType(AmqpConstants.DateTimeOffsetName, ((DateTimeOffset)o).UtcTicks),
                (o, t) => new DateTimeOffset(new DateTime((long)((DescribedType)o).Value, DateTimeKind.Utc)));
        }

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public AmqpContractSerializer()
        {
            this.customTypeCache = new ConcurrentDictionary<Type, SerializableType>();
        }

        /// <summary>
        /// Initializes the object with external type resolvers.
        /// </summary>
        /// <param name="compiler"></param>
        public AmqpContractSerializer(Func<Type, SerializableType> compiler)
            : this()
        {
            this.externalCompilers = new List<Func<Type, SerializableType>>() { compiler };
        }

        /// <summary>
        /// Writes an object to a stream.
        /// </summary>
        /// <param name="stream">The destination stream.</param>
        /// <param name="graph">The object to serialize.</param>
        public static void WriteObject(Stream stream, object graph)
        {
            if (graph == null)
            {
                stream.WriteByte(FormatCode.Null);
            }
            else
            {
                SerializableType type = Instance.GetType(graph.GetType());
                using (ByteBuffer buffer = new ByteBuffer(1024, true))
                {
                    type.WriteObject(buffer, graph);
                    stream.Write(buffer.Buffer, buffer.Offset, buffer.Length);
                }
            }
        }

        /// <summary>
        /// Reads an object from a stream.
        /// </summary>
        /// <typeparam name="T">The expected type.</typeparam>
        /// <param name="stream">The source stream.</param>
        /// <returns>An object of T.</returns>
        public static T ReadObject<T>(Stream stream)
        {
            return ReadObject<T, T>(stream);
        }

        /// <summary>
        /// Reads an object from a stream.
        /// </summary>
        /// <typeparam name="T">The expected type.</typeparam>
        /// <typeparam name="TAs">The returned type.</typeparam>
        /// <param name="stream">The source stream.</param>
        /// <returns>An object of TAs.</returns>
        /// <remarks>The serializer uses T to resolve decoding
        /// types and returns the decoded object as TAs.</remarks>
        public static TAs ReadObject<T, TAs>(Stream stream)
        {
            if (!stream.CanSeek)
            {
                throw new AmqpException(AmqpErrorCode.DecodeError, "stream.CanSeek must be true.");
            }

            SerializableType type = Instance.GetType(typeof(T));
            ByteBuffer buffer = null;
            long position = stream.Position;
            BufferListStream listStream = stream as BufferListStream;
            if (listStream != null)
            {
                ArraySegment<byte> segment = listStream.ReadBytes(int.MaxValue);
                buffer = new ByteBuffer(segment);
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

        /// <summary>
        /// Writes an object to a buffer.
        /// </summary>
        /// <param name="buffer"></param>
        /// <param name="graph"></param>
        public void WriteObjectToBuffer(ByteBuffer buffer, object graph)
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

        /// <summary>
        /// Reads an object from a buffer.
        /// </summary>
        /// <typeparam name="T">The expected type.</typeparam>
        /// <param name="buffer">The source buffer.</param>
        /// <returns>An object of T.</returns>
        public T ReadObjectFromBuffer<T>(ByteBuffer buffer)
        {
            return this.ReadObjectFromBuffer<T, T>(buffer);
        }

        /// <summary>
        /// Reads an object from a buffer.
        /// </summary>
        /// <typeparam name="T">The expected type.</typeparam>
        /// <typeparam name="TAs">The returned type.</typeparam>
        /// <param name="buffer">The source buffer.</param>
        /// <returns>An object of TAs.</returns>
        /// <remarks>The serializer uses T to resolve decoding
        /// types and returns the decoded object as TAs.</remarks>
        public TAs ReadObjectFromBuffer<T, TAs>(ByteBuffer buffer)
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
            if (this.externalCompilers != null)
            {
                foreach (var compiler in this.externalCompilers)
                {
                    SerializableType serializable = compiler(type);
                    if (serializable != null)
                    {
                        return serializable;
                    }
                }
            }

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

                if (memberInfo is FieldInfo || memberInfo is PropertyInfo)
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

            List<Type> knownTypes = new List<Type>();
            foreach (object o in type.GetTypeInfo().GetCustomAttributes(typeof(KnownTypeAttribute), false))
            {
                KnownTypeAttribute knownAttribute = (KnownTypeAttribute)o;
                if (knownAttribute.Type.GetTypeInfo().GetCustomAttributes(typeof(AmqpContractAttribute), false).Any())
                {
                    // KnownType compilation is delayed and non-recursive to avoid circular references
                    knownTypes.Add(knownAttribute.Type);
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
            if (type.GetTypeInfo().IsGenericType &&
                type.GetGenericTypeDefinition() == typeof(Nullable<>))
            {
                Type[] argTypes = type.GetGenericArguments();
                Fx.Assert(argTypes.Length == 1, "Nullable type must have one argument");
                return this.GetType(argTypes[0]);
            }

            if (type.GetTypeInfo().IsInterface)
            {
                if (typeof(IEnumerable).GetTypeInfo().IsAssignableFrom(type.GetTypeInfo()))
                {
                    // if a member is defined as enumerable interface, we have to change it
                    // to list, otherwise the decoder cannot initialize an object of an interface
                    Type itemType = typeof(object);
                    Type listType = typeof(List<object>);
                    if (type.GetTypeInfo().IsGenericType)
                    {
                        Type[] argTypes = type.GetGenericArguments();
                        Fx.Assert(argTypes.Length == 1, "IEnumerable type must have one argument");
                        itemType = argTypes[0];
                        listType = typeof(List<>).MakeGenericType(argTypes);
                    }

                    MethodAccessor addAccess = MethodAccessor.Create(listType.GetMethod("Add", new Type[] { itemType }));
                    return new SerializableType.List(this, listType, itemType, addAccess) { Final = true };
                }

                return null;
            }

            if (type.GetTypeInfo().IsEnum)
            {
                Type underlyingType = Enum.GetUnderlyingType(type);
                return new SerializableType.Converted(
                    AmqpType.Converted,
                    type,
                    underlyingType,
                    (o, t) => Convert.ChangeType(o, t),
                    (o, t) => Enum.ToObject(t, o));
            }

            if (type.GetInterfaces().Any(it => it == typeof(IAmqpSerializable)))
            {
                return new SerializableType.Serializable(this, type);
            }

            if (type.IsArray)
            {
                // validate item type to be AMQP types only
                AmqpEncoding.GetEncoding(type.GetElementType());
                return SerializableType.CreatePrimitiveType(type);
            }

            foreach (Type it in type.GetInterfaces())
            {
                if (it.GetTypeInfo().IsGenericType)
                {
                    Type genericTypeDef = it.GetGenericTypeDefinition();
                    if (genericTypeDef == typeof(IDictionary<,>))
                    {
                        Type[] argTypes = it.GetGenericArguments();
                        Type itemType = typeof(KeyValuePair<,>).MakeGenericType(argTypes);
                        MemberAccessor keyAccessor = MemberAccessor.Create(itemType.GetProperty("Key"), false);
                        MemberAccessor valueAccessor = MemberAccessor.Create(itemType.GetProperty("Value"), false);
                        MethodAccessor addAccess = MethodAccessor.Create(type.GetMethod("Add", argTypes));

                        return new SerializableType.Map(this, type, keyAccessor, valueAccessor, addAccess);
                    }

                    if (genericTypeDef == typeof(ICollection<>))
                    {
                        Type[] argTypes = it.GetGenericArguments();
                        Type itemType = argTypes[0];
                        MethodAccessor addAccess = MethodAccessor.Create(type.GetMethod("Add", argTypes));

                        return new SerializableType.List(this, type, itemType, addAccess);
                    }
                }
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
