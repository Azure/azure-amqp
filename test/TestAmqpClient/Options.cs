using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Security.Authentication;
using Microsoft.Azure.Amqp.Sasl;

namespace TestAmqpClient
{
    class Options
    {
        static readonly Dictionary<string, PropertyInfo> map;

        static Options()
        {
            map = new Dictionary<string, PropertyInfo>(StringComparer.OrdinalIgnoreCase);
            foreach (PropertyInfo pi in typeof(Options).GetProperties(BindingFlags.Public | BindingFlags.Instance))
            {
                var oa = pi.GetCustomAttribute<OptionAttribute>();
                if (oa != null)
                {
                    map[oa.Name ?? pi.Name] = pi;
                }
            }
        }

        public Options()
        {
            this.Address = "amqp://localhost:5672";
            this.Count = 1;
            this.Node = "q1";
            this.Requests = 1;
        }

        [Option(Name = "address", Description = "address URL of the remote peer")]
        public string Address
        {
            get;
            private set;
        }

        [Option(Name = "ssl", Description = "enabled Ssl protocols")]
        public SslProtocols EnabledSslProtocols
        {
            get;
            private set;
        }

        [Option(Name = "sasl", Description = "SASL mechanism to authenticate (anonymous, exteranl, plain(user,pwd))")]
        public SaslHandler Sasl
        {
            get;
            private set;
        }

        [Option(Name = "count", Description = "number of messages to send or receive (0: infinite)")]
        public long Count
        {
            get;
            private set;
        }

        [Option(Name = "node", Description = "node name of the link endpoint")]
        public string Node
        {
            get;
            private set;
        }

        [Option(Name = "body-size", Description = "body size of messages to be sent")]
        public int BodySize
        {
            get;
            private set;
        }

        [Option(Name = "requests", Description = "concurrent requests for sending/receiving messages")]
        public int Requests
        {
            get;
            private set;
        }

        [Option(Name = "progress", Description = "number of messages to report progress")]
        public int Progress
        {
            get;
            private set;
        }

        public static Options Parse(string[] args, int start)
        {
            Options options = new Options();
            for (int i = start; i < args.Length; i++)
            {
                string name = args[i].TrimStart('-');
                PropertyInfo pi;
                if (map.TryGetValue(name, out pi))
                {
                    if (pi.PropertyType == typeof(bool))
                    {
                        pi.SetValue(options, true);
                    }
                    else if (i < args.Length - 1)
                    {
                        pi.SetValue(options, GetValue(pi.PropertyType, args[i + 1]));
                    }
                    else
                    {
                        throw new ArgumentException($"Missing value for argument {pi.Name}");
                    }
                }
            }

            return options;
        }

        public static void Print(TextWriter writer)
        {
            writer.WriteLine("options:");
            foreach (var kvp in map)
            {
                var oa = kvp.Value.GetCustomAttribute<OptionAttribute>();
                writer.WriteLine($"  --{kvp.Key}\t{oa.Description}");
            }
        }
        static object GetValue(Type type, string str)
        {
            if (type == typeof(string))
            {
                return str;
            }
            else if (type == typeof(int))
            {
                return int.Parse(str);
            }
            else if (type == typeof(long))
            {
                return long.Parse(str);
            }
            else if (type == typeof(SslProtocols))
            {
                string[] parts = str.Split(',');
                SslProtocols protocols = SslProtocols.None;
                foreach (var p in parts)
                {
                    protocols |= (SslProtocols)Enum.Parse(typeof(SslProtocols), p, ignoreCase: true);
                }
                return protocols;
            }
            else if (type == typeof(SaslHandler))
            {
                if (str.Equals("anonymous", StringComparison.OrdinalIgnoreCase))
                {
                    return new SaslAnonymousHandler();
                }
                else if (str.Equals("external", StringComparison.OrdinalIgnoreCase))
                {
                    return new SaslExternalHandler();
                }
                else if (str.StartsWith("plain(", StringComparison.OrdinalIgnoreCase))
                {
                    // plain(user,pwd)
                    string[] creds = str.Substring(6, str.Length - 7).Split(',');
                    return new SaslPlainHandler() { AuthenticationIdentity = creds[0], Password = creds[1] };
                }
                else
                {
                    throw new ArgumentException($"Sasl handler {str}");
                }
            }
            else
            {
                throw new NotSupportedException(type.Name);
            }
        }

        class OptionAttribute : Attribute
        {
            public string Name { get; set; }

            public string Description { get; set; }
        }
    }
}