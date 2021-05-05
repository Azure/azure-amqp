// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Test.Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;
    using System.Security.Cryptography.X509Certificates;
    using System.Threading;
    using global::Microsoft.Azure.Amqp;
    using global::Microsoft.Azure.Amqp.Encoding;
    using global::Microsoft.Azure.Amqp.Framing;
    using global::Microsoft.Azure.Amqp.Sasl;
    using global::Microsoft.Azure.Amqp.Transport;

    static class AmqpUtils
    {
        public static bool IsSecureTransport(this Uri uri)
        {
            return uri.Scheme.Equals("amqps", StringComparison.OrdinalIgnoreCase)
                || uri.Scheme.Equals("wss", StringComparison.OrdinalIgnoreCase);
        }

        public static AmqpConnection CreateConnection(Uri uri, string sslHost, bool doSslUpgrade, SaslHandler saslHandler, int maxFrameSize, uint? idleTimeoutMs = null)
        {
            if (uri.IsSecureTransport() && sslHost == null)
            {
                sslHost = uri.Host;
            }

            return AmqpUtils.CreateConnection(uri.Host, uri.Port, sslHost, doSslUpgrade, saslHandler, maxFrameSize, idleTimeoutMs);
        }

        public static TcpTransportSettings GetTcpSettings(string host, int port, bool listen)
        {
            TcpTransportSettings tcpSettings = new TcpTransportSettings() { Host = host, Port = port };
            tcpSettings.TcpBacklog = 20;
            return tcpSettings;
        }

        public static AmqpSettings GetAmqpSettings(bool client, string sslValue, bool doSslUpgrade, params SaslHandler[] saslHandlers)
        {
            AmqpSettings settings = new AmqpSettings();
            if ((client && doSslUpgrade) || (!client && sslValue != null))
            {
                TlsTransportSettings tlsSettings = new TlsTransportSettings();
                if (client)
                {
                    tlsSettings.TargetHost = sslValue;
                    tlsSettings.CertificateValidationCallback = (s, c, h, e) => { return true; };
                }
                else
                {
                    tlsSettings.IsInitiator = false;
                    tlsSettings.Certificate = GetCertificate(sslValue); ;
                }

                TlsTransportProvider tlsProvider = new TlsTransportProvider(tlsSettings, AmqpVersion.V100);
                settings.TransportProviders.Add(tlsProvider);
            }

            if (saslHandlers != null && saslHandlers.Length >= 1 && saslHandlers[0] != null)
            {
                SaslTransportProvider saslProvider = new SaslTransportProvider(AmqpVersion.V100);
                foreach (SaslHandler handler in saslHandlers)
                {
                    saslProvider.AddHandler(handler);
                }

                settings.TransportProviders.Add(saslProvider);
            }

            AmqpTransportProvider amqpProvider = new AmqpTransportProvider(AmqpVersion.V100);
            settings.TransportProviders.Add(amqpProvider);

            return settings;
        }

        public static AmqpConnectionSettings GetConnectionSettings(int maxFrameSize)
        {
            return new AmqpConnectionSettings()
            {
                ContainerId = "AMQP-" + Guid.NewGuid().ToString().Substring(0, 8),
                MaxFrameSize = (uint)maxFrameSize,
            };
        }

        public static AmqpLinkSettings GetLinkSettings(bool forSender, string address, SettleMode settleType, int credit = 0, bool dynamic = false)
        {
            AmqpLinkSettings settings = null;

            // create a setting for sender
            settings = new AmqpLinkSettings();
            settings.LinkName = string.Format("link-{0}", Guid.NewGuid().ToString("N"));
            settings.Role = !forSender;

            Target target = new Target();
            target.Address = address;

            Source source = new Source();
            source.Address = address;
            source.DistributionMode = "move";

            settings.Source = source;
            settings.Target = target;
            settings.SettleType = settleType;

            if (!forSender)
            {
                settings.TotalLinkCredit = (uint)credit;
                settings.AutoSendFlow = credit > 0;
                if (dynamic)
                {
                    source.Address = null;
                    source.Dynamic = true;
                }
            }
            else
            {
                settings.InitialDeliveryCount = 0;
                if (dynamic)
                {
                    target.Address = null;
                    target.Dynamic = true;
                }
            }

            settings.AddProperty("x-opt-just-testing", "ignore me");

            return settings;
        }

        public static AmqpMessage CreateMessage(byte[] body)
        {
            return CreateMessage(new ArraySegment<byte>(body));
        }

        public static AmqpMessage CreateMessage(ArraySegment<byte> binaryData)
        {
            return AmqpMessage.Create(new Data[] { new Data() { Segment = binaryData } });
        }

        public static ByteBuffer GetBuffer(this AmqpMessage message)
        {
            return message.GetPayload(int.MaxValue, out bool more);
        }

        public static AmqpTransportListener CreateListener(string host, int port, string certFindValue, bool doSslUpgrade, SaslHandler saslHandler)
        {
            AmqpSettings settings = GetAmqpSettings(false, certFindValue, doSslUpgrade, saslHandler);

            TransportSettings transportSettings = GetTcpSettings(host, port, true);
            if (!doSslUpgrade && certFindValue != null)
            {
                TlsTransportSettings tlsSettings = new TlsTransportSettings(transportSettings, false);
                tlsSettings.Certificate = GetCertificate(certFindValue);
                transportSettings = tlsSettings;
            }

            TransportListener listener = transportSettings.CreateListener();
            return new AmqpTransportListener(new TransportListener[] { listener }, settings);
        }

        public static TransportBase CreateTransport(string host, int port, string sslHost, bool doSslUpgrade, SaslHandler saslHandler)
        {
            AmqpSettings settings = GetAmqpSettings(true, sslHost, doSslUpgrade, saslHandler);

            TransportSettings transportSettings = GetTcpSettings(host, port, false);
            if (!doSslUpgrade && sslHost != null)
            {
                TlsTransportSettings tlsSettings = new TlsTransportSettings(transportSettings);
                tlsSettings.TargetHost = sslHost;
                tlsSettings.CertificateValidationCallback = (s, c, h, e) => { return true; };
                transportSettings = tlsSettings;
            }

            ManualResetEvent complete = new ManualResetEvent(false);
            AmqpTransportInitiator initiator = new AmqpTransportInitiator(settings, transportSettings);
            TransportAsyncCallbackArgs args = new TransportAsyncCallbackArgs();
            args.CompletedCallback = (a) => { complete.Set(); };
            initiator.ConnectAsync(TimeSpan.FromSeconds(120), args);

            complete.WaitOne();
            complete.Dispose();

            if (args.Exception != null)
            {
                throw args.Exception;
            }

            return args.Transport;
        }

        public static AmqpConnection CreateConnection(string host, int port, string sslHost, bool doSslUpgrade, SaslHandler saslHandler, int maxFrameSize,
            uint? idleTimeoutMs = null)
        {
            AmqpSettings settings = GetAmqpSettings(true, sslHost, doSslUpgrade, saslHandler);
            TransportBase transport = CreateTransport(host, port, sslHost, doSslUpgrade, saslHandler);
            AmqpConnectionSettings connSettings = GetConnectionSettings(maxFrameSize);
            if (idleTimeoutMs != null)
            {
                connSettings.IdleTimeOut = idleTimeoutMs;
            }
            connSettings.HostName = host;
            return new AmqpConnection(transport, settings, connSettings);
        }

        public static void DumpBinaryArray(IEnumerable<ArraySegment<byte>> array)
        {
            foreach (var item in array)
            {
                System.Text.StringBuilder sb = new System.Text.StringBuilder();
                for (int i = 0; i < item.Count; ++i)
                {
                    sb.AppendFormat("{0:X2}", item.Array[i + item.Offset]);
                }

                System.Diagnostics.Debug.WriteLine(sb.ToString());
            }
        }

        public static void DumpAmqpData(byte[] data, int offset, int count)
        {
            ByteBuffer buffer = new ByteBuffer(data, offset, count);
            try
            {
                DumpAmqpData(buffer, 0);
            }
            catch (Exception exception)
            {
                System.Diagnostics.Debug.WriteLine(string.Format("Exception occurred at offset {0}", buffer.Offset));
                System.Diagnostics.Debug.WriteLine(exception.ToString());
            }
        }

        static X509Certificate2 GetCertificate(string certFindValue)
        {
            StoreLocation[] locations = new StoreLocation[] { StoreLocation.LocalMachine, StoreLocation.CurrentUser };
            foreach (StoreLocation location in locations)
            {
                X509Store store = new X509Store(StoreName.My, location);
                store.Open(OpenFlags.OpenExistingOnly);

                X509Certificate2Collection collection = store.Certificates.Find(
                    X509FindType.FindBySubjectName,
                    certFindValue,
                    false);

                if (collection.Count == 0)
                {
                    collection = store.Certificates.Find(
                        X509FindType.FindByThumbprint,
                        certFindValue,
                        false);
                }

                store.Close();
                if (collection.Count > 0)
                {
                    return collection[0];
                }
            }

            throw new ArgumentException("No certificate can be found using the find value.");
        }

        static void DumpAmqpData(ByteBuffer buffer, int indent)
        {
            int offset = buffer.Offset;
            byte formatCode = buffer.Buffer[buffer.Offset];
            if (formatCode == 0x40)
            {
                WriteAmqpValue(offset, indent, "Null");
            }
            else if (formatCode == 0x41)
            {
                WriteAmqpValue(offset, indent, "Bool:True");
            }
            else if (formatCode == 0x42)
            {
                WriteAmqpValue(offset, indent, "Bool:False");
            }
            else if (formatCode == 0x50)
            {
                WriteAmqpValue(offset, indent, string.Format("UByte:{0}", (byte)AmqpEncoding.DecodeObject(buffer)));
            }
            else if (formatCode == 0x60)
            {
                WriteAmqpValue(offset, indent, string.Format("UShort:{0}", (ushort)AmqpEncoding.DecodeObject(buffer)));
            }
            else if (formatCode == 0x70)
            {
                WriteAmqpValue(offset, indent, string.Format("UInt:{0}", (uint)AmqpEncoding.DecodeObject(buffer)));
            }
            else if (formatCode == 0x52)
            {
                WriteAmqpValue(offset, indent, string.Format("SmallUInt:{0}", (uint)AmqpEncoding.DecodeObject(buffer)));
            }
            else if (formatCode == 0x80)
            {
                WriteAmqpValue(offset, indent, string.Format("ULong:{0}", (ulong)AmqpEncoding.DecodeObject(buffer)));
            }
            else if (formatCode == 0x53)
            {
                WriteAmqpValue(offset, indent, string.Format("SmallULong:{0}", (ulong)AmqpEncoding.DecodeObject(buffer)));
            }
            else if (formatCode == 0x51)
            {
                WriteAmqpValue(offset, indent, string.Format("Byte:{0}", (sbyte)AmqpEncoding.DecodeObject(buffer)));
            }
            else if (formatCode == 0x61)
            {
                WriteAmqpValue(offset, indent, string.Format("Short:{0}", (short)AmqpEncoding.DecodeObject(buffer)));
            }
            else if (formatCode == 0x71)
            {
                WriteAmqpValue(offset, indent, string.Format("Int:{0}", (int)AmqpEncoding.DecodeObject(buffer)));
            }
            else if (formatCode == 0x54)
            {
                WriteAmqpValue(offset, indent, string.Format("SmallInt:{0}", (int)AmqpEncoding.DecodeObject(buffer)));
            }
            else if (formatCode == 0x81)
            {
                WriteAmqpValue(offset, indent, string.Format("Long:{0}", (long)AmqpEncoding.DecodeObject(buffer)));
            }
            else if (formatCode == 0x54)
            {
                WriteAmqpValue(offset, indent, string.Format("SmallLong:{0}", (long)AmqpEncoding.DecodeObject(buffer)));
            }
            else if (formatCode == 0x72)
            {
                WriteAmqpValue(offset, indent, string.Format("Float:{0}", (float)AmqpEncoding.DecodeObject(buffer)));
            }
            else if (formatCode == 0x82)
            {
                WriteAmqpValue(offset, indent, string.Format("Double:{0}", (double)AmqpEncoding.DecodeObject(buffer)));
            }
            else if (formatCode == 0x73)
            {
                WriteAmqpValue(offset, indent, string.Format("Char:{0}", (char)AmqpEncoding.DecodeObject(buffer)));
            }
            else if (formatCode == 0x83)
            {
                WriteAmqpValue(offset, indent, string.Format("TimeStamp:{0}", (DateTime)AmqpEncoding.DecodeObject(buffer)));
            }
            else if (formatCode == 0x98)
            {
                WriteAmqpValue(offset, indent, string.Format("Uuid:{0}", (Guid)AmqpEncoding.DecodeObject(buffer)));
            }
            else if (formatCode == 0xa0 || formatCode == 0xb0)
            {
                ArraySegment<byte> bin = (ArraySegment<byte>)AmqpEncoding.DecodeObject(buffer);
                WriteAmqpValue(offset, indent, string.Format("Binary{0}:{1}", formatCode == 0xa0 ? 8 : 32, bin.Array == null ? "Null" : bin.Count.ToString()));
            }
            else if (formatCode == 0xa1 || formatCode == 0xb1 || formatCode == 0xa2 || formatCode == 0xb2)
            {
                string str = (string)AmqpEncoding.DecodeObject(buffer);
                string toDisplay = "Null";
                if (str != null)
                {
                    toDisplay = str.Length > 20 ? str.Substring(0, 8) + "..." + str.Substring(str.Length - 8) : str;
                }

                WriteAmqpValue(offset, indent, string.Format("Utf{0}String{1}:{2}", (formatCode & 0x0F) == 0x01 ? 8 : 16, (formatCode & 0xF0) == 0xa0 ? 8 : 32, toDisplay));
            }
            else if (formatCode == 0xa3 || formatCode == 0xb3)
            {
                AmqpSymbol sym = (AmqpSymbol)AmqpEncoding.DecodeObject(buffer);
                string toDisplay = "Null";
                if (sym.Value != null)
                {
                    toDisplay = sym.Value.Length > 16 ? sym.Value.Substring(0, 6) + "..." + sym.Value.Substring(sym.Value.Length - 6) : sym.Value;
                }

                WriteAmqpValue(offset, indent, string.Format("Symbol{0}:{1}", formatCode == 0xa3 ? 8 : 32, toDisplay));
            }
            else if (formatCode == 0xc0 || formatCode == 0xd0)
            {
                IList<object> list = AmqpEncoding.DecodeObject(buffer) as List<object>;
                WriteAmqpValue(offset, indent, string.Format("List{0}:{1}", formatCode == 0xc0 ? 8 : 32, list.Count));
                for (int i = 0; i < list.Count; ++i)
                {
                    WriteAmqpValue(-1, indent + 1, list[i].ToString());
                }
            }
            else if (formatCode == 0xe0 || formatCode == 0xf0)
            {
                Array array = (Array)AmqpEncoding.DecodeObject(buffer);
                WriteAmqpValue(offset, indent, string.Format("Array{0}:{1}", formatCode == 0xe0 ? 8 : 32, array.Length));
                for (int i = 0; i < array.Length; ++i)
                {
                    WriteAmqpValue(-1, indent + 1, array.GetValue(i).ToString());
                }
            }
            else if (formatCode == 0xc1 || formatCode == 0xd1)
            {
                AmqpMap map = (AmqpMap)AmqpEncoding.DecodeObject(buffer);
                WriteAmqpValue(offset, indent, string.Format("Map{0}:{1}", formatCode == 0xc0 ? 8 : 32, map.Count));
                foreach (var kvp in map)
                {
                    WriteAmqpValue(-1, indent + 1, kvp.Key.ToString());
                    WriteAmqpValue(-1, indent + 1, kvp.Value?.ToString() ?? "Null");
                }
            }
            else if (formatCode == 0x00)
            {
                WriteAmqpValue(offset, indent, "Described");
                DumpAmqpData(buffer, indent + 1);
                DumpAmqpData(buffer, indent + 1);
            }
        }

        static void WriteAmqpValue(int offset, int indent, string value)
        {
            System.Text.StringBuilder sb = new System.Text.StringBuilder();
            if (offset >= 0)
            {
                sb.AppendFormat("0x{0:X4}  ", offset);
            }
            else
            {
                sb.Append("        ");
            }

            if (indent > 0)
            {
                sb.Append(new string(' ', indent * 4));
            }

            sb.Append(value);

            System.Diagnostics.Debug.WriteLine(sb.ToString());
        }
    }
}
