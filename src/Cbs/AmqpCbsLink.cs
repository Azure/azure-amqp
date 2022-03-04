// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Runtime.ExceptionServices;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.Azure.Amqp.Framing;

    /// <summary>
    /// Encapsulates a pair of links to the '$cbs' node for managing security tokens.
    /// </summary>
    public sealed class AmqpCbsLink
    {
        readonly AmqpConnection connection;
        readonly FaultTolerantAmqpObject<RequestResponseAmqpLink> linkFactory;

        /// <summary>
        /// Initializes the CBS link.
        /// </summary>
        /// <param name="connection">The connection in which to create the links.</param>
        public AmqpCbsLink(AmqpConnection connection)
        {
            this.connection = connection ?? throw new ArgumentNullException(nameof(connection));
            this.connection.AddExtension(this);
            this.linkFactory = new FaultTolerantAmqpObject<RequestResponseAmqpLink>(
                (t, c) => this.CreateCbsLinkAsync(t, c),
                static link => CloseLink(link));
        }

        /// <summary>
        /// Gets or sets the timeout for CBS operations.
        /// </summary>
        public TimeSpan OperationTimeout
        {
            get { return this.connection.OperationTimeout; }
            set { }
        }

        /// <summary>
        /// Closes the link.
        /// </summary>
        public void Close()
        {
            this.linkFactory.Close();
        }

        /// <summary>
        /// Sends a security token for a resource for later access.
        /// </summary>
        /// <param name="tokenProvider">The provider for issuing security tokens.</param>
        /// <param name="namespaceAddress">The namespace (or tenant) name.</param>
        /// <param name="audience">The audience. In most cases it is the same as resource.</param>
        /// <param name="resource">The resource to access.</param>
        /// <param name="requiredClaims">The required claims to access the resource.</param>
        /// <param name="timeout">A timeout for the send token operation.</param>
        /// <returns>The expiration time of the token as the task result.</returns>
        public Task<DateTime> SendTokenAsync(ICbsTokenProvider tokenProvider, Uri namespaceAddress, string audience, string resource, string[] requiredClaims, TimeSpan timeout)
        {
            return this.SendTokenAsync(tokenProvider, namespaceAddress, audience, resource, requiredClaims, timeout, CancellationToken.None);
        }

        /// <summary>
        /// Sends a security token for a resource for later access.
        /// </summary>
        /// <param name="tokenProvider">The provider for issuing security tokens.</param>
        /// <param name="namespaceAddress">The namespace (or tenant) name.</param>
        /// <param name="audience">The audience. In most cases it is the same as resource.</param>
        /// <param name="resource">The resource to access.</param>
        /// <param name="requiredClaims">The required claims to access the resource.</param>
        /// <param name="cancellationToken">A cancellation token that can be used to signal the asynchronous operation should be canceled.</param>
        /// <returns>The expiration time of the token as the task result.</returns>
        public Task<DateTime> SendTokenAsync(ICbsTokenProvider tokenProvider, Uri namespaceAddress, string audience, string resource, string[] requiredClaims, CancellationToken cancellationToken)
        {
            return this.SendTokenAsync(tokenProvider, namespaceAddress, audience, resource, requiredClaims, this.OperationTimeout, cancellationToken);
        }

        /// <summary>
        /// Begin an async operation to send a security token for a resource for later access.
        /// </summary>
        /// <param name="tokenProvider">The provider for issuing security tokens.</param>
        /// <param name="namespaceAddress">The namespace (or tenant) name.</param>
        /// <param name="audience">The audience. In most cases it is the same as resource.</param>
        /// <param name="resource">The resource to access.</param>
        /// <param name="requiredClaims">The required claims to access the resource.</param>
        /// <param name="timeout">A timeout for the send token operation.</param>
        /// <param name="callback">The callback to invoke when the operation completes.</param>
        /// <param name="state">The object associated with the operation.</param>
        /// <returns>An <see cref="IAsyncResult"/> for the operation.</returns>
        public IAsyncResult BeginSendToken(ICbsTokenProvider tokenProvider, Uri namespaceAddress, string audience, string resource, string[] requiredClaims, TimeSpan timeout, AsyncCallback callback, object state)
        {
            return this.SendTokenAsync(tokenProvider, namespaceAddress, audience, resource, requiredClaims, timeout, CancellationToken.None).ToAsyncResult(callback, state);
        }

        /// <summary>
        /// Ends the asynchronous send token operation.
        /// </summary>
        /// <param name="result">The <see cref="IAsyncResult"/> returned by the begin method.</param>
        /// <returns>The expiration time of the token.</returns>
        public DateTime EndSendToken(IAsyncResult result)
        {
            return Extensions.EndAsyncResult<DateTime>(result);
        }

        async Task<DateTime> SendTokenAsync(ICbsTokenProvider tokenProvider, Uri namespaceAddress, string audience,
            string resource, string[] requiredClaims, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIfNull(tokenProvider, nameof(tokenProvider));
            ThrowIfNull(namespaceAddress, nameof(namespaceAddress));
            ThrowIfNull(audience, nameof(audience));
            ThrowIfNull(resource, nameof(resource));
            ThrowIfNull(requiredClaims, nameof(requiredClaims));
            if (this.connection.IsClosing())
            {
                throw new OperationCanceledException("Connection is closing or closed.");
            }

            TimeoutHelper timeoutHelper = new TimeoutHelper(timeout);
            CbsToken token = await tokenProvider.GetTokenAsync(namespaceAddress, resource, requiredClaims, cancellationToken).ConfigureAwait(false);
            string tokenType = token.TokenType;
            if (tokenType == null)
            {
                throw new NotSupportedException(AmqpResources.AmqpUnsupportedTokenType);
            }

            RequestResponseAmqpLink requestResponseLink;
            if (!this.linkFactory.TryGetOpenedObject(out requestResponseLink))
            {
                requestResponseLink = await this.linkFactory.GetOrCreateAsync(timeoutHelper.RemainingTime(), cancellationToken).ConfigureAwait(false);
            }

            AmqpValue value = new AmqpValue();
            value.Value = token.TokenValue;
            AmqpMessage putTokenRequest = AmqpMessage.Create(value);
            putTokenRequest.ApplicationProperties.Map[CbsConstants.Operation] = CbsConstants.PutToken.OperationValue;
            putTokenRequest.ApplicationProperties.Map[CbsConstants.PutToken.Type] = tokenType;
            putTokenRequest.ApplicationProperties.Map[CbsConstants.PutToken.Audience] = audience;
            putTokenRequest.ApplicationProperties.Map[CbsConstants.PutToken.Expiration] = token.ExpiresAtUtc;

            AmqpMessage putTokenResponse = await requestResponseLink.RequestAsync(putTokenRequest, AmqpConstants.NullBinary,
                timeoutHelper.RemainingTime(), cancellationToken).ConfigureAwait(false);

            int statusCode = (int)putTokenResponse.ApplicationProperties.Map[CbsConstants.PutToken.StatusCode];
            string statusDescription = (string)putTokenResponse.ApplicationProperties.Map[CbsConstants.PutToken.StatusDescription];
            if (statusCode == (int)AmqpResponseStatusCode.Accepted || statusCode == (int)AmqpResponseStatusCode.OK)
            {
                return token.ExpiresAtUtc;
            }

            Exception exception;
            AmqpResponseStatusCode amqpResponseStatusCode = (AmqpResponseStatusCode)statusCode;
            switch (amqpResponseStatusCode)
            {
                case AmqpResponseStatusCode.BadRequest:
                    exception = new AmqpException(AmqpErrorCode.InvalidField, AmqpResources.GetString(AmqpResources.AmqpPutTokenFailed, statusCode, statusDescription));
                    break;
                case AmqpResponseStatusCode.NotFound:
                    exception = new AmqpException(AmqpErrorCode.NotFound, AmqpResources.GetString(AmqpResources.AmqpPutTokenFailed, statusCode, statusDescription));
                    break;
                case AmqpResponseStatusCode.Forbidden:
                    exception = new AmqpException(AmqpErrorCode.TransferLimitExceeded, AmqpResources.GetString(AmqpResources.AmqpPutTokenFailed, statusCode, statusDescription));
                    break;
                case AmqpResponseStatusCode.Unauthorized:
                    exception = new AmqpException(AmqpErrorCode.UnauthorizedAccess, AmqpResources.GetString(AmqpResources.AmqpPutTokenFailed, statusCode, statusDescription));
                    break;
                default:
                    exception = new AmqpException(AmqpErrorCode.InvalidField, AmqpResources.GetString(AmqpResources.AmqpPutTokenFailed, statusCode, statusDescription));
                    break;
            }

            throw exception;
        }

        static void ThrowIfNull<T>(T arg, string name) where T : class
        {
            if (arg == null)
            {
                throw new ArgumentNullException(name);
            }
        }

        static void CloseLink(RequestResponseAmqpLink link)
        {
            AmqpSession session = link.SendingLink?.Session;
            link.Abort();
            session?.SafeClose();
        }

        async Task<RequestResponseAmqpLink> CreateCbsLinkAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            string address = CbsConstants.CbsAddress;
            AmqpSession session = null;
            RequestResponseAmqpLink link = null;
            ExceptionDispatchInfo lastException = null;
            TimeoutHelper timeoutHelper = new TimeoutHelper(timeout);

            while (timeoutHelper.RemainingTime() > TimeSpan.Zero)
            {
                cancellationToken.ThrowIfCancellationRequested();

                try
                {
                    AmqpSessionSettings sessionSettings = new AmqpSessionSettings() { Properties = new Fields() };
                    session = this.connection.CreateSession(sessionSettings);
                    await Task.Factory.FromAsync(
                        (sn, t, k, c, s) => sn.BeginOpen(t, k, c, s),
                        (r) => ((AmqpSession)r.AsyncState).EndOpen(r),
                        session,
                        timeoutHelper.RemainingTime(),
                        cancellationToken,
                        session).ConfigureAwait(false);

                    Fields properties = new Fields();
                    properties.Add(CbsConstants.TimeoutName, (uint)AmqpConstants.DefaultTimeout.TotalMilliseconds);
                    link = new RequestResponseAmqpLink("cbs", session, address, properties);
                    await Task.Factory.FromAsync(
                        (lk, t, k, c, s) => lk.BeginOpen(t, k, c, s),
                        (r) => ((AmqpLink)r.AsyncState).EndOpen(r),
                        link,
                        timeoutHelper.RemainingTime(),
                        cancellationToken,
                        link).ConfigureAwait(false);

                    AmqpTrace.Provider.AmqpOpenEntitySucceeded(this, link.Name, address);
                    return link;
                }
                catch (Exception exception) when (!cancellationToken.IsCancellationRequested)
                {
                    if (this.connection.IsClosing())
                    {
                        throw new OperationCanceledException("Connection is closing or closed.", exception);
                    }

                    lastException = ExceptionDispatchInfo.Capture(exception);
                    AmqpTrace.Provider.AmqpOpenEntityFailed(this, this.GetType().Name, address, exception);
                }

                await Task.Delay(1000, cancellationToken).ConfigureAwait(false);
            }

            link?.Abort();
            session?.SafeClose();

            cancellationToken.ThrowIfCancellationRequested();

            lastException?.Throw();
            return null;
        }
    }
}
