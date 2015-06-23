using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;
using System.Xml.Serialization;

namespace amqplib_generator
{
    class Program
    {
        public static amqp LoadAMQPTypes()
        {
            XmlSerializer serializer = new XmlSerializer(typeof(amqp));
            StreamReader reader = new StreamReader("../../amqp_definitions.xml");
            amqp amqp = (amqp)serializer.Deserialize(reader);
            reader.Close();
            return amqp;
        }

        public static string GetCType(string amqp_type)
        {
            string result;
            switch (amqp_type)
            {
                default:
                    result = "AMQP_VALUE";
                    break;

                case "binary":
                    result = "amqp_binary";
                    break;

                case "boolean":
                    result = "bool";
                    break;

                case "uint":
                    result = "uint32_t";
                    break;

                case "ubyte":
                    result = "uint8_t";
                    break;

                case "ushort":
                    result = "uint16_t";
                    break;

                case "ulong":
                    result = "uint64_t";
                    break;

                case "string":
                    result = "const char*";
                    break;

                case "symbol":
                    result = "uint32_t";
                    break;

                case "map":
                    result = "AMQP_VALUE";
                    break;

                case "*":
                    result = "AMQP_VALUE";
                    break;
            }

            return result;
        }

        public static string GetMandatoryArgList(type type)
        {
            string result = string.Empty;

            foreach(field field in type.Items.Where(item => (item is field) && ((item as field).mandatory == "true")))
            {
                if (result.Length > 0)
                {
                    result += ", ";
                }

                result += GetCType(field.type).Replace('-', '_').Replace(':', '_') + " " + field.name.Replace('-', '_').Replace(':', '_');
            }

            return result;
        }

        static void Main(string[] args)
        {
            LoadAMQPTypes();
            amqp_definitions_h amqp_definitions_h = new amqp_definitions_h();
            System.IO.File.WriteAllText("../../../inc/amqp_definitions.h", amqp_definitions_h.TransformText());
            amqp_definitions_c amqp_definitions_c = new amqp_definitions_c();
            System.IO.File.WriteAllText("../../../src/amqp_definitions.c", amqp_definitions_c.TransformText());
        }
    }
}
