// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Runtime.Serialization;
    using Microsoft.Azure.Amqp.Encoding;
    using Microsoft.Azure.Amqp.Framing;

    struct Section
    {
        public SectionFlag Flag;
        public int Offset;
        public int Length;
        public AmqpDescribed Value;
    }

    sealed class AmqpMessageReader
    {
        struct SectionInfo
        {
            public SectionFlag Flag;
            public ulong Code;
            public string Name;
            public Func<AmqpDescribed> Ctor;
            public Func<byte, ByteBuffer, Error> Scanner;
        }

        static readonly SectionInfo[] MessageSections;

        static AmqpMessageReader()
        {
            MessageSections = new SectionInfo[]
            {
                new SectionInfo()
                {
                    Flag = SectionFlag.Header,
                    Code = Header.Code,
                    Name = Header.Name,
                    Ctor = () => new Header(),
                    Scanner = (a, b) => ScanListSection(a, b)
                },
                new SectionInfo()
                {
                    Flag = SectionFlag.DeliveryAnnotations,
                    Code = DeliveryAnnotations.Code,
                    Name = DeliveryAnnotations.Name,
                    Ctor = () => new DeliveryAnnotations(),
                    Scanner = (a, b) => ScanMapSection(a, b)
                },
                new SectionInfo()
                {
                    Flag = SectionFlag.MessageAnnotations,
                    Code = MessageAnnotations.Code,
                    Name = MessageAnnotations.Name,
                    Ctor = () => new MessageAnnotations(),
                    Scanner = (a, b) => ScanMapSection(a, b)
                },
                new SectionInfo()
                {
                    Flag = SectionFlag.Properties,
                    Code = Properties.Code,
                    Name = Properties.Name,
                    Ctor = () => new Properties(),
                    Scanner = (a, b) => ScanListSection(a, b)
                },
                new SectionInfo()
                {
                    Flag = SectionFlag.ApplicationProperties,
                    Code = ApplicationProperties.Code,
                    Name = ApplicationProperties.Name,
                    Ctor = () => new ApplicationProperties(),
                    Scanner = (a, b) => ScanMapSection(a, b)
                },
                new SectionInfo()
                {
                    Flag = SectionFlag.Data,
                    Code = Data.Code,
                    Name = Data.Name,
                    Ctor = () => new Data(),
                    Scanner = (a, b) => ScanDataSection(a, b)
                },
                new SectionInfo()
                {
                    Flag = SectionFlag.AmqpSequence,
                    Code = AmqpSequence.Code,
                    Name = AmqpSequence.Name,
                    Ctor = () => new AmqpSequence(),
                    Scanner = (a, b) => ScanListSection(a, b)
                },
                new SectionInfo()
                {
                    Flag = SectionFlag.AmqpValue,
                    Code = AmqpValue.Code,
                    Name = AmqpValue.Name,
                    Ctor = () => new AmqpValue(),
                    Scanner = (a, b) => ScanValueSection(a, b)
                },
                new SectionInfo()
                {
                    Flag = SectionFlag.Footer,
                    Code = Footer.Code,
                    Name = Footer.Name,
                    Ctor = () => new Footer(),
                    Scanner = (a, b) => ScanMapSection(a, b)
                },
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
                                section.Value = BinaryEncoding.Decode(buffer, 0, false);
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

                        Section s = new Section() { Flag = info.Flag, Offset = offset, Length = length, Value = section };
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
