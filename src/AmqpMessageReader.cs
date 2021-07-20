// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Runtime.Serialization;
    using Microsoft.Azure.Amqp.Encoding;
    using Microsoft.Azure.Amqp.Framing;

    readonly struct Section
    {
        public readonly SectionFlag Flag;
        public readonly int Offset;
        public readonly int Length;
        public readonly AmqpDescribed Value;

        public Section(SectionFlag flag, int offset, int length, AmqpDescribed value)
        {
            Flag = flag;
            Offset = offset;
            Length = length;
            Value = value;
        }
    }

    sealed class AmqpMessageReader
    {
        readonly struct SectionInfo
        {
            public readonly SectionFlag Flag;
            public readonly ulong Code;
            public readonly string Name;
            public readonly Func<AmqpDescribed> Ctor;
            public readonly Func<byte, ByteBuffer, Error> Scanner;

            public SectionInfo(SectionFlag flag, ulong code, string name, Func<AmqpDescribed> ctor, Func<byte, ByteBuffer, Error> scanner)
            {
                Flag = flag;
                Code = code;
                Name = name;
                Ctor = ctor;
                Scanner = scanner;
            }
        }

        static readonly SectionInfo[] MessageSections;

        static AmqpMessageReader()
        {
            MessageSections = new SectionInfo[]
            {
                new SectionInfo(
                    flag: SectionFlag.Header,
                    code: Header.Code,
                    name: Header.Name,
                    ctor: () => new Header(),
                    scanner: (a, b) => ScanListSection(a, b)),
                new SectionInfo(
                    flag: SectionFlag.DeliveryAnnotations,
                    code: DeliveryAnnotations.Code,
                    name: DeliveryAnnotations.Name,
                    ctor: () => new DeliveryAnnotations(),
                    scanner: (a, b) => ScanMapSection(a, b)),
                new SectionInfo(
                    flag: SectionFlag.MessageAnnotations,
                    code: MessageAnnotations.Code,
                    name: MessageAnnotations.Name,
                    ctor: () => new MessageAnnotations(),
                    scanner: (a, b) => ScanMapSection(a, b)),
                new SectionInfo(flag: SectionFlag.Properties,
                    code: Properties.Code,
                    name: Properties.Name,
                    ctor: () => new Properties(),
                    scanner: (a,  b) => ScanListSection(a, b)),
                new SectionInfo(
                    flag: SectionFlag.ApplicationProperties,
                    code: ApplicationProperties.Code,
                    name: ApplicationProperties.Name,
                    ctor: () => new ApplicationProperties(),
                    scanner: (a, b) => ScanMapSection(a, b)),
                new SectionInfo(
                    flag: SectionFlag.Data,
                    code: Data.Code,
                    name: Data.Name,
                    ctor: () => new Data(),
                    scanner: (a, b) => ScanDataSection(a, b)),
                new SectionInfo(
                    flag: SectionFlag.AmqpSequence,
                    code: AmqpSequence.Code,
                    name: AmqpSequence.Name,
                    ctor: () => new AmqpSequence(),
                    scanner: (a, b) => ScanListSection(a, b)),
                new SectionInfo(
                    flag: SectionFlag.AmqpValue,
                    code: AmqpValue.Code,
                    name: AmqpValue.Name,
                    ctor: () => new AmqpValue(),
                    scanner: (a, b) => ScanValueSection(a, b)),
                new SectionInfo(
                    flag: SectionFlag.Footer,
                    code: Footer.Code,
                    name: Footer.Name,
                    ctor: () => new Footer(),
                    scanner: (a, b) => ScanMapSection(a, b)),
            };

            for (int i = 0; i < MessageSections.Length; i++)
            {
                SectionInfo info = MessageSections[i];
                AmqpCodec.RegisterKnownTypes(info.Name, info.Code, info.Ctor);
            }
        }

        // Parse the buffer for message sections. If a section is specified in the flags, fully
        // decode it; otherwise attempt to fast advance the buffer. The handler is invoked at
        // least once with the first section found. The section could be null if it is not in
        // the flags. The parser stops if handler returns false.
        public static bool TryRead<T1, T2>(T1 t1, T2 t2, ByteBuffer buffer, SectionFlag sections,
            Func<T1, T2, Section, bool> sectionHandler, string context, out Error error)
        {
            error = null;

            if (buffer.TryAddReference())
            {
                int pos = buffer.Offset;

                try
                {
                    while (buffer.Length > 0)
                    {
                        int offset = buffer.Offset;

                        SectionInfo info;
                        if (!TryReadSectionInfo(buffer, out info, out error))
                        {
                            return false;
                        }

                        AmqpDescribed section;
                        int length;
                        if ((info.Flag & sections) > 0)
                        {
                            section = info.Ctor();
                            if (section.DescriptorCode == Data.Code)
                            {
                                section.Value = AmqpCodec.DecodeBinary(buffer, false);
                            }
                            else
                            {
                                section.DecodeValue(buffer);
                            }

                            section.Offset = offset;
                            length = buffer.Offset - offset;
                            section.Length = length;
                        }
                        else
                        {
                            // fast forward to next section
                            FormatCode formatCode = AmqpEncoding.ReadFormatCode(buffer);
                            if (formatCode != FormatCode.Null)
                            {
                                error = info.Scanner(formatCode, buffer);
                                if (error != null)
                                {
                                    return false;
                                }
                            }

                            section = null;
                            length = buffer.Offset - offset;
                        }

                        Section s = new Section(info.Flag, offset, length, section);
                        bool shouldContinue = sectionHandler(t1, t2, s);

                        if (!shouldContinue)
                        {
                            break;
                        }
                    }

                    return true;
                }
                catch (SerializationException se)
                {
                    error = GetDecodeError(se.Message);
                }
                catch (AmqpException ae)
                {
                    error = ae.Error;
                }
                catch (Exception exception)
                {
                    if (Fx.IsFatal(exception))
                    {
                        throw;
                    }

                    error = GetDecodeError(exception.Message);
                }
                finally
                {
                    buffer.Seek(pos);
                    buffer.RemoveReference();
                }
            }
            else
            {
                // The delivery is already disposed. Treat it as decode error.
                error = GetDecodeError(Resources.AmqpBufferAlreadyReclaimed);
            }

            return false;
        }

        static bool TryReadSectionInfo(ByteBuffer buffer, out SectionInfo info, out Error error)
        {
            info = default(SectionInfo);

            FormatCode formatCode = AmqpEncoding.ReadFormatCode(buffer);
            if (formatCode != FormatCode.Described)
            {
                error = GetDecodeError(AmqpResources.GetString(Resources.AmqpInvalidFormatCode, formatCode, buffer.Offset - FixedWidth.FormatCode));
                return false;
            }

            ulong code = ulong.MaxValue;
            formatCode = AmqpEncoding.ReadFormatCode(buffer);
            switch (formatCode)
            {
                case FormatCode.SmallULong:
                    code = AmqpBitConverter.ReadUByte(buffer);
                    break;
                case FormatCode.ULong:
                    code = AmqpBitConverter.ReadULong(buffer);
                    break;
                case FormatCode.Symbol32:
                case FormatCode.Symbol8:
                    // symbol name should be seldom used so do not optimize for it
                    AmqpSymbol name = SymbolEncoding.Decode(buffer, formatCode);
                    for (int i = 0; i < MessageSections.Length; i++)
                    {
                        if (MessageSections[i].Name.Equals(name.Value))
                        {
                            code = MessageSections[i].Code;
                            break;
                        }
                    }
                    if (code == ulong.MaxValue)
                    {
                        error = GetDecodeError(AmqpResources.GetString(Resources.AmqpInvalidMessageSectionCode, name));
                        return false;
                    }
                    break;
                default:
                    error = GetDecodeError(AmqpResources.GetString(Resources.AmqpInvalidFormatCode, formatCode, buffer.Offset - FixedWidth.FormatCode));
                    return false;
            }

            int index = (int)(code - MessageSections[0].Code);
            if (index < 0 || index >= MessageSections.Length)
            {
                error = GetDecodeError(AmqpResources.GetString(Resources.AmqpInvalidMessageSectionCode, code));
                return false;
            }

            info = MessageSections[index];
            error = null;

            return true;
        }

        static Error ScanListSection(byte formatCode, ByteBuffer buffer)
        {
            switch (formatCode)
            {
                case FormatCode.List0:
                    return null;
                case FormatCode.List8:
                    return AdvanceBuffer(buffer, AmqpBitConverter.ReadUByte(buffer));
                case FormatCode.List32:
                    return AdvanceBuffer(buffer, AmqpBitConverter.ReadUInt(buffer));
                default:
                    return GetDecodeError(AmqpResources.GetString(Resources.AmqpInvalidFormatCode, formatCode, buffer.Offset));
            }
        }

        static Error ScanMapSection(byte formatCode, ByteBuffer buffer)
        {
            switch (formatCode)
            {
                case FormatCode.Map8:
                    return AdvanceBuffer(buffer, AmqpBitConverter.ReadUByte(buffer));
                case FormatCode.Map32:
                    return AdvanceBuffer(buffer, AmqpBitConverter.ReadUInt(buffer));
                default:
                    return GetDecodeError(AmqpResources.GetString(Resources.AmqpInvalidFormatCode, formatCode, buffer.Offset));
            }
        }

        static Error ScanDataSection(byte formatCode, ByteBuffer buffer)
        {
            switch (formatCode)
            {
                case FormatCode.Binary8:
                    return AdvanceBuffer(buffer, AmqpBitConverter.ReadUByte(buffer));
                case FormatCode.Binary32:
                    return AdvanceBuffer(buffer, AmqpBitConverter.ReadUInt(buffer));
                default:
                    return GetDecodeError(AmqpResources.GetString(Resources.AmqpInvalidFormatCode, formatCode, buffer.Offset));
            }
        }

        static Error ScanValueSection(byte formatCode, ByteBuffer buffer, int depth = 0)
        {
            if (formatCode == FormatCode.Described)
            {
                if (depth > 10)
                {
                    // protection for stack overflow
                    return GetDecodeError(AmqpResources.GetString(Resources.AmqpInvalidFormatCode, formatCode, buffer.Offset));
                }

                Error error = ScanValueSection(AmqpEncoding.ReadFormatCode(buffer), buffer, depth + 1);
                if (error != null)
                {
                    return error;
                }

                formatCode = AmqpEncoding.ReadFormatCode(buffer);
            }

            uint size;
            if (formatCode >= FormatCode.Binary8)
            {
                // variable width
                size = (formatCode & 0x10) == 0 ?
                    AmqpBitConverter.ReadUByte(buffer) :
                    AmqpBitConverter.ReadUInt(buffer);
            }
            else
            {
                // fixed width
                size = (uint)((1 << ((formatCode >> 4) - 4)) >> 1);
            }

            return AdvanceBuffer(buffer, size);
        }

        static Error AdvanceBuffer(ByteBuffer buffer, uint size)
        {
            if (size > buffer.Length)
            {
                return GetDecodeError(AmqpResources.GetString(Resources.AmqpInsufficientBufferSize, size, buffer.Length));
            }

            buffer.Complete((int)size);
            return null;
        }

        static Error GetDecodeError(string description)
        {
            return new Error() { Condition = AmqpErrorCode.DecodeError, Description = description };
        }
    }
}
