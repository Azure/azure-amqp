﻿using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

// General Information about an assembly is controlled through the following 
// set of attributes. Change these attribute values to modify the information
// associated with an assembly.
[assembly: AssemblyTitle("Microsoft.Azure.Amqp")]
[assembly: AssemblyDescription("")]
[assembly: AssemblyConfiguration("")]
[assembly: DefaultDllImportSearchPathsAttribute(DllImportSearchPath.SafeDirectories)] 

// Setting ComVisible to false makes the types in this assembly not visible 
// to COM components.  If you need to access a type in this assembly from 
// COM, set the ComVisible attribute to true on that type.
[assembly: ComVisible(false)]

// Make this internal visible for testing
[assembly: InternalsVisibleTo("Test.Microsoft.Amqp")]
[assembly: InternalsVisibleTo("TestAmqpBroker")]

// The following GUID is for the ID of the typelib if this project is exposed to COM
[assembly: Guid("A0D1C509-8C92-4AA1-983C-00E5254F8AE0")]

#if RELEASE_DELAY_SIGN
[assembly: AssemblyDelaySignAttribute(true)]
[assembly: AssemblyKeyFileAttribute("..\\35MSSharedLib1024.snk")]
#endif