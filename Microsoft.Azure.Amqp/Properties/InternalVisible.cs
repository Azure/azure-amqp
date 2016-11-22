using System.Runtime.CompilerServices;

#if !RELEASE_DELAY_SIGN
[assembly: InternalsVisibleTo("Test.Microsoft.Azure.Amqp")]
[assembly: InternalsVisibleTo("Test.Microsoft.Azure.Amqp.Core")]
#endif
