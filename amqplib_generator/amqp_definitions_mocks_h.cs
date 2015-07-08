﻿// ------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//     Runtime Version: 12.0.0.0
//  
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
// </auto-generated>
// ------------------------------------------------------------------------------
namespace amqplib_generator
{
    using System.Linq;
    using System.Text;
    using System.Collections.Generic;
    using amqplib_generator;
    using System;
    
    /// <summary>
    /// Class to produce the template output
    /// </summary>
    
    #line 1 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
    [global::System.CodeDom.Compiler.GeneratedCodeAttribute("Microsoft.VisualStudio.TextTemplating", "12.0.0.0")]
    public partial class amqp_definitions_mocks_h : amqp_definitions_mocks_hBase
    {
#line hidden
        /// <summary>
        /// Create the template output
        /// </summary>
        public virtual string TransformText()
        {
            this.Write("\r\n");
            
            #line 8 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
 amqp amqp = Program.LoadAMQPTypes(); 
            
            #line default
            #line hidden
            this.Write("\r\n#ifndef AMQP_DEFINITIONS_MOCKS_H\r\n#define AMQP_DEFINITIONS_MOCKS_H\r\n\r\n#include " +
                    "\"amqp_definitions.h\"\r\n\r\n");
            
            #line 15 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
	int i = 0; 
            
            #line default
            #line hidden
            
            #line 16 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
	foreach (section section in amqp.Items.Where(item => item is section)) 
            
            #line default
            #line hidden
            
            #line 17 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
	{ 
            
            #line default
            #line hidden
            
            #line 18 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
		List<type> types = new List<type>(); 
            
            #line default
            #line hidden
            
            #line 19 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
		types.AddRange(section.Items.Where(item => item is type).Cast<type>()); 
            
            #line default
            #line hidden
            
            #line 20 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
		foreach (type type in types) 
            
            #line default
            #line hidden
            
            #line 21 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
		{ 
            
            #line default
            #line hidden
            
            #line 22 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
			string type_name = type.name.ToLower().Replace('-', '_'); 
            
            #line default
            #line hidden
            this.Write("/* ");
            
            #line 23 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(type.name));
            
            #line default
            #line hidden
            this.Write(" */\r\n\r\n");
            
            #line 25 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
			if (type.@class == typeClass.composite) 
            
            #line default
            #line hidden
            
            #line 26 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
			{ 
            
            #line default
            #line hidden
            
            #line 27 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
				string arg_list = Program.GetMandatoryArgListMock(type); 
            
            #line default
            #line hidden
            
            #line 28 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
				ICollection<field> args = Program.GetMandatoryArgs(type); 
            
            #line default
            #line hidden
            this.Write("static const ");
            
            #line 29 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(type_name.ToUpper()));
            
            #line default
            #line hidden
            this.Write("_HANDLE test_");
            
            #line 29 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(type_name));
            
            #line default
            #line hidden
            this.Write("_handle = (");
            
            #line 29 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(type_name.ToUpper()));
            
            #line default
            #line hidden
            this.Write("_HANDLE)");
            
            #line 29 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(0x4242 + i));
            
            #line default
            #line hidden
            this.Write(";\r\nstatic const AMQP_VALUE test_");
            
            #line 30 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(type_name));
            
            #line default
            #line hidden
            this.Write("_amqp_value = (AMQP_VALUE)");
            
            #line 30 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(0x4243 + i));
            
            #line default
            #line hidden
            this.Write(";\r\n");
            
            #line 31 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
				i++; 
            
            #line default
            #line hidden
            
            #line 32 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
			} 
            
            #line default
            #line hidden
            
            #line 33 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
		} 
            
            #line default
            #line hidden
            
            #line 34 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
	} 
            
            #line default
            #line hidden
            this.Write("\r\nTYPED_MOCK_CLASS(amqp_definitions_mocks, CGlobalMock)\r\n{\r\npublic:\r\n");
            
            #line 39 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
	foreach (section section in amqp.Items.Where(item => item is section)) 
            
            #line default
            #line hidden
            
            #line 40 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
	{ 
            
            #line default
            #line hidden
            
            #line 41 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
		List<type> types = new List<type>(); 
            
            #line default
            #line hidden
            
            #line 42 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
		types.AddRange(section.Items.Where(item => item is type).Cast<type>()); 
            
            #line default
            #line hidden
            
            #line 43 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
		foreach (type type in types) 
            
            #line default
            #line hidden
            
            #line 44 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
		{ 
            
            #line default
            #line hidden
            
            #line 45 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
			string type_name = type.name.ToLower().Replace('-', '_'); 
            
            #line default
            #line hidden
            this.Write("/* ");
            
            #line 46 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(type.name));
            
            #line default
            #line hidden
            this.Write(" */\r\n\r\n");
            
            #line 48 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
			if (type.@class == typeClass.composite) 
            
            #line default
            #line hidden
            
            #line 49 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
			{ 
            
            #line default
            #line hidden
            
            #line 50 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
				string arg_list = Program.GetMandatoryArgListMock(type); 
            
            #line default
            #line hidden
            
            #line 51 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
				ICollection<field> args = Program.GetMandatoryArgs(type); 
            
            #line default
            #line hidden
            this.Write("\tMOCK_STATIC_METHOD_");
            
            #line 52 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(args.Count()));
            
            #line default
            #line hidden
            this.Write("(,");
            
            #line 52 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(type_name.ToUpper()));
            
            #line default
            #line hidden
            this.Write("_HANDLE, ");
            
            #line 52 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(type_name));
            
            #line default
            #line hidden
            this.Write("_create");
            
            #line 52 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(arg_list));
            
            #line default
            #line hidden
            this.Write(");\r\n\tMOCK_METHOD_END(");
            
            #line 53 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(type_name.ToUpper()));
            
            #line default
            #line hidden
            this.Write("_HANDLE, test_");
            
            #line 53 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(type_name));
            
            #line default
            #line hidden
            this.Write("_handle);\r\n\tMOCK_STATIC_METHOD_1(, void, ");
            
            #line 54 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(type_name));
            
            #line default
            #line hidden
            this.Write("_destroy, ");
            
            #line 54 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(type_name.ToUpper()));
            
            #line default
            #line hidden
            this.Write("_HANDLE, ");
            
            #line 54 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(type_name));
            
            #line default
            #line hidden
            this.Write(");\r\n\tMOCK_VOID_METHOD_END();\r\n\tMOCK_STATIC_METHOD_1(, AMQP_VALUE, amqpvalue_creat" +
                    "e_");
            
            #line 56 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(type_name));
            
            #line default
            #line hidden
            this.Write(", ");
            
            #line 56 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(type_name.ToUpper()));
            
            #line default
            #line hidden
            this.Write("_HANDLE, ");
            
            #line 56 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(type_name));
            
            #line default
            #line hidden
            this.Write(");\r\n\tMOCK_METHOD_END(AMQP_VALUE, test_");
            
            #line 57 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(type_name));
            
            #line default
            #line hidden
            this.Write("_amqp_value);\r\n\r\n");
            
            #line 59 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
				foreach (field field in type.Items.Where(item => item is field)) 
            
            #line default
            #line hidden
            
            #line 60 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
				{ 
            
            #line default
            #line hidden
            
            #line 61 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
					string field_name = field.name.ToLower().Replace('-', '_'); 
            
            #line default
            #line hidden
            
            #line 62 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
					string c_type = Program.GetCType(field.type).Replace('-', '_').Replace(':', '_'); 
            
            #line default
            #line hidden
            
            #line 63 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
					type field_type = Program.GetTypeByName(types, field.type); 
            
            #line default
            #line hidden
            
            #line 64 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
					if ((field_type != null) && (field_type.@class == typeClass.composite)) c_type = field_type.name.ToUpper().Replace('-', '_').Replace(':', '_') + "_HANDLE"; 
            
            #line default
            #line hidden
            this.Write("\tMOCK_STATIC_METHOD_2(, int, ");
            
            #line 65 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(type_name));
            
            #line default
            #line hidden
            this.Write("_get_");
            
            #line 65 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(field_name));
            
            #line default
            #line hidden
            this.Write(", ");
            
            #line 65 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(type_name.ToUpper()));
            
            #line default
            #line hidden
            this.Write("_HANDLE, ");
            
            #line 65 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(type_name));
            
            #line default
            #line hidden
            this.Write(", ");
            
            #line 65 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(c_type));
            
            #line default
            #line hidden
            this.Write("*, ");
            
            #line 65 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(field_name));
            
            #line default
            #line hidden
            this.Write("_value);\r\n\tMOCK_METHOD_END(int, 0);\r\n\tMOCK_STATIC_METHOD_2(, int, ");
            
            #line 67 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(type_name));
            
            #line default
            #line hidden
            this.Write("_set_");
            
            #line 67 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(field_name));
            
            #line default
            #line hidden
            this.Write(", ");
            
            #line 67 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(type_name.ToUpper()));
            
            #line default
            #line hidden
            this.Write("_HANDLE, ");
            
            #line 67 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(type_name));
            
            #line default
            #line hidden
            this.Write(", ");
            
            #line 67 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(c_type));
            
            #line default
            #line hidden
            this.Write(", ");
            
            #line 67 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(field_name));
            
            #line default
            #line hidden
            this.Write("_value);\r\n\tMOCK_METHOD_END(int, 0);\r\n");
            
            #line 69 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
				} 
            
            #line default
            #line hidden
            this.Write("\r\n");
            
            #line 71 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
			} 
            
            #line default
            #line hidden
            
            #line 72 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
		} 
            
            #line default
            #line hidden
            
            #line 73 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
	} 
            
            #line default
            #line hidden
            this.Write("};\r\n\r\n");
            
            #line 76 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
	foreach (section section in amqp.Items.Where(item => item is section)) 
            
            #line default
            #line hidden
            
            #line 77 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
	{ 
            
            #line default
            #line hidden
            
            #line 78 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
		List<type> types = new List<type>(); 
            
            #line default
            #line hidden
            
            #line 79 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
		types.AddRange(section.Items.Where(item => item is type).Cast<type>()); 
            
            #line default
            #line hidden
            
            #line 80 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
		foreach (type type in types) 
            
            #line default
            #line hidden
            
            #line 81 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
		{ 
            
            #line default
            #line hidden
            
            #line 82 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
			string type_name = type.name.ToLower().Replace('-', '_'); 
            
            #line default
            #line hidden
            this.Write("/* ");
            
            #line 83 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(type.name));
            
            #line default
            #line hidden
            this.Write(" */\r\n\r\n");
            
            #line 85 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
			if (type.@class == typeClass.composite) 
            
            #line default
            #line hidden
            
            #line 86 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
			{ 
            
            #line default
            #line hidden
            
            #line 87 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
				string arg_list = Program.GetMandatoryArgListMock(type); 
            
            #line default
            #line hidden
            
            #line 88 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
				ICollection<field> args = Program.GetMandatoryArgs(type); 
            
            #line default
            #line hidden
            this.Write("\tDECLARE_GLOBAL_MOCK_METHOD_");
            
            #line 89 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(args.Count()));
            
            #line default
            #line hidden
            this.Write("(amqp_definitions_mocks, ,");
            
            #line 89 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(type_name.ToUpper()));
            
            #line default
            #line hidden
            this.Write("_HANDLE, ");
            
            #line 89 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(type_name));
            
            #line default
            #line hidden
            this.Write("_create");
            
            #line 89 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(arg_list));
            
            #line default
            #line hidden
            this.Write(");\r\n\tDECLARE_GLOBAL_MOCK_METHOD_1(amqp_definitions_mocks, , void, ");
            
            #line 90 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(type_name));
            
            #line default
            #line hidden
            this.Write("_destroy, ");
            
            #line 90 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(type_name.ToUpper()));
            
            #line default
            #line hidden
            this.Write("_HANDLE, ");
            
            #line 90 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(type_name));
            
            #line default
            #line hidden
            this.Write(");\r\n\tDECLARE_GLOBAL_MOCK_METHOD_1(amqp_definitions_mocks, , AMQP_VALUE, amqpvalue" +
                    "_create_");
            
            #line 91 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(type_name));
            
            #line default
            #line hidden
            this.Write(", ");
            
            #line 91 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(type_name.ToUpper()));
            
            #line default
            #line hidden
            this.Write("_HANDLE, ");
            
            #line 91 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(type_name));
            
            #line default
            #line hidden
            this.Write(");\r\n\r\n");
            
            #line 93 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
				foreach (field field in type.Items.Where(item => item is field)) 
            
            #line default
            #line hidden
            
            #line 94 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
				{ 
            
            #line default
            #line hidden
            
            #line 95 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
					string field_name = field.name.ToLower().Replace('-', '_'); 
            
            #line default
            #line hidden
            
            #line 96 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
					string c_type = Program.GetCType(field.type).Replace('-', '_').Replace(':', '_'); 
            
            #line default
            #line hidden
            
            #line 97 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
					type field_type = Program.GetTypeByName(types, field.type); 
            
            #line default
            #line hidden
            
            #line 98 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
					if ((field_type != null) && (field_type.@class == typeClass.composite)) c_type = field_type.name.ToUpper().Replace('-', '_').Replace(':', '_') + "_HANDLE"; 
            
            #line default
            #line hidden
            this.Write("\tDECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, ");
            
            #line 99 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(type_name));
            
            #line default
            #line hidden
            this.Write("_get_");
            
            #line 99 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(field_name));
            
            #line default
            #line hidden
            this.Write(", ");
            
            #line 99 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(type_name.ToUpper()));
            
            #line default
            #line hidden
            this.Write("_HANDLE, ");
            
            #line 99 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(type_name));
            
            #line default
            #line hidden
            this.Write(", ");
            
            #line 99 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(c_type));
            
            #line default
            #line hidden
            this.Write("*, ");
            
            #line 99 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(field_name));
            
            #line default
            #line hidden
            this.Write("_value);\r\n\tDECLARE_GLOBAL_MOCK_METHOD_2(amqp_definitions_mocks, , int, ");
            
            #line 100 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(type_name));
            
            #line default
            #line hidden
            this.Write("_set_");
            
            #line 100 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(field_name));
            
            #line default
            #line hidden
            this.Write(", ");
            
            #line 100 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(type_name.ToUpper()));
            
            #line default
            #line hidden
            this.Write("_HANDLE, ");
            
            #line 100 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(type_name));
            
            #line default
            #line hidden
            this.Write(", ");
            
            #line 100 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(c_type));
            
            #line default
            #line hidden
            this.Write(", ");
            
            #line 100 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(field_name));
            
            #line default
            #line hidden
            this.Write("_value);\r\n");
            
            #line 101 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
				} 
            
            #line default
            #line hidden
            this.Write("\r\n");
            
            #line 103 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
			} 
            
            #line default
            #line hidden
            
            #line 104 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
		} 
            
            #line default
            #line hidden
            
            #line 105 "D:\AMQPLib\amqplib_generator\amqp_definitions_mocks_h.tt"
	} 
            
            #line default
            #line hidden
            this.Write("\r\n#endif /* AMQP_DEFINITIONS_MOCKS_H */\r\n");
            return this.GenerationEnvironment.ToString();
        }
    }
    
    #line default
    #line hidden
    #region Base class
    /// <summary>
    /// Base class for this transformation
    /// </summary>
    [global::System.CodeDom.Compiler.GeneratedCodeAttribute("Microsoft.VisualStudio.TextTemplating", "12.0.0.0")]
    public class amqp_definitions_mocks_hBase
    {
        #region Fields
        private global::System.Text.StringBuilder generationEnvironmentField;
        private global::System.CodeDom.Compiler.CompilerErrorCollection errorsField;
        private global::System.Collections.Generic.List<int> indentLengthsField;
        private string currentIndentField = "";
        private bool endsWithNewline;
        private global::System.Collections.Generic.IDictionary<string, object> sessionField;
        #endregion
        #region Properties
        /// <summary>
        /// The string builder that generation-time code is using to assemble generated output
        /// </summary>
        protected System.Text.StringBuilder GenerationEnvironment
        {
            get
            {
                if ((this.generationEnvironmentField == null))
                {
                    this.generationEnvironmentField = new global::System.Text.StringBuilder();
                }
                return this.generationEnvironmentField;
            }
            set
            {
                this.generationEnvironmentField = value;
            }
        }
        /// <summary>
        /// The error collection for the generation process
        /// </summary>
        public System.CodeDom.Compiler.CompilerErrorCollection Errors
        {
            get
            {
                if ((this.errorsField == null))
                {
                    this.errorsField = new global::System.CodeDom.Compiler.CompilerErrorCollection();
                }
                return this.errorsField;
            }
        }
        /// <summary>
        /// A list of the lengths of each indent that was added with PushIndent
        /// </summary>
        private System.Collections.Generic.List<int> indentLengths
        {
            get
            {
                if ((this.indentLengthsField == null))
                {
                    this.indentLengthsField = new global::System.Collections.Generic.List<int>();
                }
                return this.indentLengthsField;
            }
        }
        /// <summary>
        /// Gets the current indent we use when adding lines to the output
        /// </summary>
        public string CurrentIndent
        {
            get
            {
                return this.currentIndentField;
            }
        }
        /// <summary>
        /// Current transformation session
        /// </summary>
        public virtual global::System.Collections.Generic.IDictionary<string, object> Session
        {
            get
            {
                return this.sessionField;
            }
            set
            {
                this.sessionField = value;
            }
        }
        #endregion
        #region Transform-time helpers
        /// <summary>
        /// Write text directly into the generated output
        /// </summary>
        public void Write(string textToAppend)
        {
            if (string.IsNullOrEmpty(textToAppend))
            {
                return;
            }
            // If we're starting off, or if the previous text ended with a newline,
            // we have to append the current indent first.
            if (((this.GenerationEnvironment.Length == 0) 
                        || this.endsWithNewline))
            {
                this.GenerationEnvironment.Append(this.currentIndentField);
                this.endsWithNewline = false;
            }
            // Check if the current text ends with a newline
            if (textToAppend.EndsWith(global::System.Environment.NewLine, global::System.StringComparison.CurrentCulture))
            {
                this.endsWithNewline = true;
            }
            // This is an optimization. If the current indent is "", then we don't have to do any
            // of the more complex stuff further down.
            if ((this.currentIndentField.Length == 0))
            {
                this.GenerationEnvironment.Append(textToAppend);
                return;
            }
            // Everywhere there is a newline in the text, add an indent after it
            textToAppend = textToAppend.Replace(global::System.Environment.NewLine, (global::System.Environment.NewLine + this.currentIndentField));
            // If the text ends with a newline, then we should strip off the indent added at the very end
            // because the appropriate indent will be added when the next time Write() is called
            if (this.endsWithNewline)
            {
                this.GenerationEnvironment.Append(textToAppend, 0, (textToAppend.Length - this.currentIndentField.Length));
            }
            else
            {
                this.GenerationEnvironment.Append(textToAppend);
            }
        }
        /// <summary>
        /// Write text directly into the generated output
        /// </summary>
        public void WriteLine(string textToAppend)
        {
            this.Write(textToAppend);
            this.GenerationEnvironment.AppendLine();
            this.endsWithNewline = true;
        }
        /// <summary>
        /// Write formatted text directly into the generated output
        /// </summary>
        public void Write(string format, params object[] args)
        {
            this.Write(string.Format(global::System.Globalization.CultureInfo.CurrentCulture, format, args));
        }
        /// <summary>
        /// Write formatted text directly into the generated output
        /// </summary>
        public void WriteLine(string format, params object[] args)
        {
            this.WriteLine(string.Format(global::System.Globalization.CultureInfo.CurrentCulture, format, args));
        }
        /// <summary>
        /// Raise an error
        /// </summary>
        public void Error(string message)
        {
            System.CodeDom.Compiler.CompilerError error = new global::System.CodeDom.Compiler.CompilerError();
            error.ErrorText = message;
            this.Errors.Add(error);
        }
        /// <summary>
        /// Raise a warning
        /// </summary>
        public void Warning(string message)
        {
            System.CodeDom.Compiler.CompilerError error = new global::System.CodeDom.Compiler.CompilerError();
            error.ErrorText = message;
            error.IsWarning = true;
            this.Errors.Add(error);
        }
        /// <summary>
        /// Increase the indent
        /// </summary>
        public void PushIndent(string indent)
        {
            if ((indent == null))
            {
                throw new global::System.ArgumentNullException("indent");
            }
            this.currentIndentField = (this.currentIndentField + indent);
            this.indentLengths.Add(indent.Length);
        }
        /// <summary>
        /// Remove the last indent that was added with PushIndent
        /// </summary>
        public string PopIndent()
        {
            string returnValue = "";
            if ((this.indentLengths.Count > 0))
            {
                int indentLength = this.indentLengths[(this.indentLengths.Count - 1)];
                this.indentLengths.RemoveAt((this.indentLengths.Count - 1));
                if ((indentLength > 0))
                {
                    returnValue = this.currentIndentField.Substring((this.currentIndentField.Length - indentLength));
                    this.currentIndentField = this.currentIndentField.Remove((this.currentIndentField.Length - indentLength));
                }
            }
            return returnValue;
        }
        /// <summary>
        /// Remove any indentation
        /// </summary>
        public void ClearIndent()
        {
            this.indentLengths.Clear();
            this.currentIndentField = "";
        }
        #endregion
        #region ToString Helpers
        /// <summary>
        /// Utility class to produce culture-oriented representation of an object as a string.
        /// </summary>
        public class ToStringInstanceHelper
        {
            private System.IFormatProvider formatProviderField  = global::System.Globalization.CultureInfo.InvariantCulture;
            /// <summary>
            /// Gets or sets format provider to be used by ToStringWithCulture method.
            /// </summary>
            public System.IFormatProvider FormatProvider
            {
                get
                {
                    return this.formatProviderField ;
                }
                set
                {
                    if ((value != null))
                    {
                        this.formatProviderField  = value;
                    }
                }
            }
            /// <summary>
            /// This is called from the compile/run appdomain to convert objects within an expression block to a string
            /// </summary>
            public string ToStringWithCulture(object objectToConvert)
            {
                if ((objectToConvert == null))
                {
                    throw new global::System.ArgumentNullException("objectToConvert");
                }
                System.Type t = objectToConvert.GetType();
                System.Reflection.MethodInfo method = t.GetMethod("ToString", new System.Type[] {
                            typeof(System.IFormatProvider)});
                if ((method == null))
                {
                    return objectToConvert.ToString();
                }
                else
                {
                    return ((string)(method.Invoke(objectToConvert, new object[] {
                                this.formatProviderField })));
                }
            }
        }
        private ToStringInstanceHelper toStringHelperField = new ToStringInstanceHelper();
        /// <summary>
        /// Helper to produce culture-oriented representation of an object as a string
        /// </summary>
        public ToStringInstanceHelper ToStringHelper
        {
            get
            {
                return this.toStringHelperField;
            }
        }
        #endregion
    }
    #endregion
}