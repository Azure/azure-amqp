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
                    result = "";
                    break;

                case "boolean":
                    result = "bool";
                    break;
            }

            return result;
        }

        static void Main(string[] args)
        {
            LoadAMQPTypes();
            amqp_definitions_h amqp_definitions_h = new amqp_definitions_h();
            string text = amqp_definitions_h.TransformText();
            System.IO.File.WriteAllText("../../../inc/amqp_definitions.h", text);
        }
    }
}
