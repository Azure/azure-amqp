using System;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.Azure.Amqp;
using Xunit;

namespace Test.Microsoft.Azure.Amqp
{
    public class SingletonTests
    {
        [Fact]
        public async Task SingletonConcurrentCloseOpenTests()
        {
            var createTcs = new TaskCompletionSource<object>();
            var closeTcs = new TaskCompletionSource<object>();

            var singleton = new SingletonTester(createTcs.Task, closeTcs.Task);

            var creating = singleton.GetOrCreateAsync(CancellationToken.None);
            var closing = singleton.CloseAsync();

            closeTcs.SetResult(new object());
            await closing;

            createTcs.SetResult(new object());
            await creating;

            await Assert.ThrowsAsync<ObjectDisposedException>(() => singleton.GetOrCreateAsync(CancellationToken.None));

            var createdObj = GetInternalProperty<object>(singleton, "Value");
            Assert.Null(createdObj);
        }

        private class SingletonTester : Singleton<object>
        {
            private readonly Task<object> _onCreateComplete;
            private readonly Task<object> _onSafeCloseComplete;

            public SingletonTester(Task<object> onCreateComplete, Task<object> onSafeCloseComplete)
            {
                _onCreateComplete = onCreateComplete;
                _onSafeCloseComplete = onSafeCloseComplete;
            }

            protected override async Task<object> OnCreateAsync(TimeSpan timeout, CancellationToken cancellationToken)
            {
                await _onCreateComplete;
                return new object();
            }

            protected override void OnSafeClose(object value)
            {
                _onSafeCloseComplete.GetAwaiter().GetResult();
            }
        }

        private T GetInternalProperty<T>(object from, string propertyName)
          where T : class
        {
            var prop = from.GetType().GetProperty(propertyName, System.Reflection.BindingFlags.NonPublic | System.Reflection.BindingFlags.Instance);
            return prop.GetValue(from) as T;
        }
    }
}
