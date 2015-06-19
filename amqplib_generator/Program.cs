using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace amqplib_generator
{
    class Program
    {
        static void Main(string[] args)
        {
            amqp_definitions_h amqp_definitions_h = new amqp_definitions_h();
            string text = amqp_definitions_h.TransformText();
            System.IO.File.WriteAllText("../../../inc/amqp_definitions.h", text);
        }
    }
}
