using System.Runtime.CompilerServices;

#if !RELEASE_DELAY_SIGN
[assembly: InternalsVisibleTo("Test.Microsoft.Amqp")]
[assembly: InternalsVisibleTo("Test.Microsoft.Amqp.Uwp")]
#endif
