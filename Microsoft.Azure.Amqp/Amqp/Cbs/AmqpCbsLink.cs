// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.Azure.Amqp.Framing;

    /// <summary>
    /// Encapsulates a pair of links to '$cbs' for managing CBS tokens
    /// </summary>
    public sealed class AmqpCbsLink
    {
        readonly AmqpConnection connection;
        readonly CbsSingleton cbsSingleton;

        /// <summary>
        /// Constructs a new instance
        /// </summary>
        public AmqpCbsLink(AmqpConnection connection)
        {
            this.connection = connection ?? throw new ArgumentNullException(nameof(connection));
            this.connection.Extensions.Add(this);
            this.cbsSingleton = new CbsSingleton(this);
        }

        public void Close()
        {
            this.cbsSingleton.Close();
        }

        public Task<DateTime> SendTokenAsync(ICbsTokenProvider tokenProvider, Uri namespaceAddress, string audience, string resource, string[] requiredClaims, TimeSpan timeout)
        {
            return Task.Factory.FromAsync(
                (p, t, k, c, s) => ((AmqpCbsLink)s).BeginSendToken(p.TokenProvider, p.NamespaceAddress, p.Audience, p.Resource, p.RequiredClaims, t, k, c, s),
                (r) => ((AmqpCbsLink)r.AsyncState).EndSendToken(r),
                new SendTokenParams() { TokenProvider = tokenProvider, NamespaceAddress = namespaceAddress, Audience = audience, Resource = resource, RequiredClaims = requiredClaims },
                timeout,
                CancellationToken.None,
                this);
        }

        public Task<DateTime> SendTokenAsync(ICbsTokenProvider tokenProvider, Uri namespaceAddress, string audience, string resource, string[] requiredClaims, CancellationToken cancellationToken)
        {
            return Task.Factory.FromAsync(
                (p, t, k, c, s) => ((AmqpCbsLink)s).BeginSendToken(p.TokenProvider, p.NamespaceAddress, p.Audience, p.Resource, p.RequiredClaims, t, k, c, s),
                (r) => ((AmqpCbsLink)r.AsyncState).EndSendToken(r),
                new SendTokenParams() { TokenProvider = tokenProvider, NamespaceAddress = namespaceAddress, Audience = audience, Resource = resource, RequiredClaims = requiredClaims },
                this.connection.OperationTimeout,
                cancellationToken,
                this);
        }

        public IAsyncResult BeginSendToken(ICbsTokenProvider tokenProvider, Uri namespaceAddress, string audience, string resource, string[] requiredClaims, TimeSpan timeout, AsyncCallback callback, object state)
        {
            return this.BeginSendToken(tokenProvider, namespaceAddress, audience, resource, requiredClaims, timeout, CancellationToken.None, callback, state);
        }

        public DateTime EndSendToken(IAsyncResult result)
        {
            return SendTokenAsyncResult.End(result).ExpiresAtUtc;
        }

        static void ThrowIfNull<T>(T arg, string name) where T : class
        {
            if (arg == null)
            {
                throw new ArgumentNullException(name);
            }
        }

        IAsyncResult BeginSendToken(ICbsTokenProvider tokenProvider, Uri namespaceAddress, string audience, string resource,
            string[] requiredClaims, TimeSpan timeout, CancellationToken cancellationToken, AsyncCallback callback, object state)
        {
            ThrowIfNull(tokenProvider, nameof(tokenProvider));
            ThrowIfNull(namespaceAddress, nameof(namespaceAddress));
            ThrowIfNull(audience, nameof(audience));
            ThrowIfNull(resource, nameof(resource));
            ThrowIfNull(requiredClaims, nameof(requiredClaims));
            if (this.connection.IsClosing())
            {
                throw new ObjectDisposedException(CbsConstants.CbsAddress);
            }

            return new SendTokenAsyncResult(this, tokenProvider, namespaceAddress, audience, resource, requiredClaims, timeout, cancellationToken, callback, state);
        }

        struct SendTokenParams
        {
            public ICbsTokenProvider TokenProvider { get; set; }

            public Uri NamespaceAddress { get; set; }

            public string Audience { get; set; }

            public string Resource { get; set; }

            public string[] RequiredClaims { get; set; }
        }

        sealed class CbsSingleton : Singleton<RequestResponseAmqpLink>
        {
            readonly AmqpCbsLink parent;

            public CbsSingleton(AmqpCbsLink parent)
            {
                this.parent = parent;
            }

            protected override Task<RequestResponseAmqpLink> OnCreateAsync(TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Task.Factory.FromAsync(
                    (p, t, k, c, s) => new OpenCbsRequestResponseLinkAsyncResult(p, t, k, c, s),
                    (r) => OpenCbsRequestResponseLinkAsyncResult.End(r).Link,
                    this.parent,
                    timeout,
                    cancellationToken,
                    this);
            }

            // Deprecated, but needs to stay available to avoid
            // breaking changes. The attribute removes it from
            // any code completion listings and doc generation.
            [EditorBrowsable(EditorBrowsableState.Never)]
            protected override Task<RequestResponseAmqpLink> OnCreateAsync(TimeSpan timeout) => OnCreateAsync(timeout, CancellationToken.None);

            protected override void OnSafeClose(RequestResponseAmqpLink value)
            {
                value.Abort();
                value.Session.SafeClose();
            }
        }

        sealed class OpenCbsRequestResponseLinkAsyncResult : IteratorAsyncResult<OpenCbsRequestResponseLinkAsyncResult>, ILinkFactory
        {
            readonly AmqpCbsLink parent;
            readonly CancellationToken cancellationToken;
            AmqpSession session;

            public OpenCbsRequestResponseLinkAsyncResult(AmqpCbsLink parent, TimeSpan timeout,
                CancellationToken cancellationToken, AsyncCallback callback, object state)
                : base(timeout, callback, state)
            {
                this.parent = parent;
                this.cancellationToken = cancellationToken;

                this.Start();
            }

            public RequestResponseAmqpLink Link { get; private set; }

            protected override IEnumerator<AsyncStep> GetAsyncSteps()
            {
                string address = CbsConstants.CbsAddress;
                while (this.RemainingTime() > TimeSpan.Zero)
                {
                    try
                    {
                        AmqpSessionSettings sessionSettings = new AmqpSessionSettings() { Properties = new Fields() };
                        this.session = new AmqpSession(this.parent.connection, sessionSettings, this);
                        this.parent.connection.AddSession(session, null);
                    }
                    catch (InvalidOperationException exception)
                    {
                        this.Complete(exception);
                        yield break;
                    }

                    yield return this.CallAsync(
                        (thisPtr, t, c, s) => thisPtr.session.BeginOpen(t, thisPtr.cancellationToken, c, s),
                        (thisPtr, r) => thisPtr.session.EndOpen(r),
                        ExceptionPolicy.Continue);

                    Exception lastException = this.LastAsyncStepException;
                    if (lastException != null)
                    {
                        AmqpTrace.Provider.AmqpOpenEntityFailed(this.parent.connection, this.session, string.Empty, address, lastException.Message);
                        this.session.Abort();
                        this.Complete(lastException);
                        yield break;
                    }

                    Fields properties = new Fields();
                    properties.Add(CbsConstants.TimeoutName, (uint)this.RemainingTime().TotalMilliseconds);
                    this.Link = new RequestResponseAmqpLink("cbs", this.session, address, properties);

                    yield return this.CallAsync(
                        (thisPtr, t, c, s) => thisPtr.Link.BeginOpen(t, thisPtr.cancellationToken, c, s),
                        (thisPtr, r) => thisPtr.Link.EndOpen(r),
                        ExceptionPolicy.Continue);

                    lastException = this.LastAsyncStepException;
                    if (lastException != null)
                    {
                        AmqpTrace.Provider.AmqpOpenEntityFailed(this.parent.connection, this.Link, this.Link.Name, address, lastException.Message);
                        this.session.SafeClose();
                        this.Link = null;
                        this.Complete(lastException);
                        yield break;
                    }

                    AmqpTrace.Provider.AmqpOpenEntitySucceeded(this.parent.connection, this.Link, this.Link.Name, address);
                    yield break;
                }

                if (this.session != null)
                {
                    this.session.SafeClose();
                }

                this.Complete(new TimeoutException(AmqpResources.GetString(AmqpResources.AmqpTimeout, this.OriginalTimeout, address)));
            }

            AmqpLink ILinkFactory.CreateLink(AmqpSession session, AmqpLinkSettings settings)
            {
                AmqpLink link;
                if (settings.IsReceiver())
                {
                    link = new ReceivingAmqpLink(session, settings);
                }
                else
                {
                    link = new SendingAmqpLink(session, settings);
                }

                AmqpTrace.Provider.AmqpLogOperationInformational(this, TraceOperation.Create, link);
                return link;
            }

            IAsyncResult ILinkFactory.BeginOpenLink(AmqpLink link, TimeSpan timeout, AsyncCallback callback, object state)
            {
                return new CompletedAsyncResult(callback, state);
            }

            void ILinkFactory.EndOpenLink(IAsyncResult result)
            {
                CompletedAsyncResult.End(result);
            }
        }

        sealed class SendTokenAsyncResult : IteratorAsyncResult<SendTokenAsyncResult>
        {
            readonly ICbsTokenProvider tokenProvider;
            readonly AmqpCbsLink cbsLink;
            readonly string[] requiredClaims;
            readonly Uri namespaceAddress;
            readonly string audience;
            readonly string resource;
            readonly CancellationToken cancellationToken;
            Task<CbsToken> tokenTask;
            Task<RequestResponseAmqpLink> requestResponseLinkTask;
            RequestResponseAmqpLink requestResponseLink;
            AmqpMessage putTokenRequest;
            AmqpMessage putTokenResponse;

            public SendTokenAsyncResult(
                AmqpCbsLink cbsLink,
                ICbsTokenProvider tokenProvider,
                Uri namespaceAddress,
                string audience,
                string resource,
                string[] requiredClaims,
                TimeSpan timeout,
                CancellationToken cancellationToken,
                AsyncCallback callback,
                object state)
                : base(timeout, callback, state)
            {
                this.cbsLink = cbsLink;
                this.namespaceAddress = namespaceAddress;
                this.audience = audience;
                this.resource = resource;
                this.requiredClaims = requiredClaims;
                this.tokenProvider = tokenProvider;
                this.cancellationToken = cancellationToken;

                this.Start();
            }

            public DateTime ExpiresAtUtc { get; private set; }

            protected override IEnumerator<AsyncStep> GetAsyncSteps()
            {
                yield return this.CallTask(
                    (thisPtr, t) => thisPtr.tokenTask = thisPtr.tokenProvider.GetTokenAsync(thisPtr.namespaceAddress, thisPtr.resource, thisPtr.requiredClaims),
                    ExceptionPolicy.Transfer);

                CbsToken token = this.tokenTask.Result;
                string tokenType = token.TokenType;
                if (tokenType == null)
                {
                    this.Complete(new InvalidOperationException(AmqpResources.AmqpUnsupportedTokenType));
                    yield break;
                }

                if (!this.cbsLink.cbsSingleton.TryGet(out this.requestResponseLink, link => link.State == AmqpObjectState.Opened))
                {
                    yield return this.CallTask(
                        (thisPtr, t) => thisPtr.requestResponseLinkTask = thisPtr.cbsLink.cbsSingleton.GetOrCreateAsync(t, thisPtr.cancellationToken),
                        ExceptionPolicy.Transfer);

                    this.requestResponseLink = this.requestResponseLinkTask.Result;
                    Fx.AssertIsNotNull(this.requestResponseLink, "requestResponseLink cannot be null without exception");
                }

                AmqpValue value = new AmqpValue() { Value = token.TokenValue };
                this.putTokenRequest = AmqpMessage.Create(value);
                this.putTokenRequest.ApplicationProperties = new ApplicationProperties();
                this.putTokenRequest.ApplicationProperties.Map[CbsConstants.Operation] = CbsConstants.PutToken.OperationValue;
                this.putTokenRequest.ApplicationProperties.Map[CbsConstants.PutToken.Type] = tokenType;
                this.putTokenRequest.ApplicationProperties.Map[CbsConstants.PutToken.Audience] = this.audience;
                this.putTokenRequest.ApplicationProperties.Map[CbsConstants.PutToken.Expiration] = token.ExpiresAtUtc;

                yield return this.CallAsync(
                    (thisPtr, t, c, s) => thisPtr.requestResponseLink.BeginRequest(thisPtr.putTokenRequest, AmqpConstants.NullBinary, t, thisPtr.cancellationToken, c, s),
                    (thisPtr, r) => thisPtr.putTokenResponse = thisPtr.requestResponseLink.EndRequest(r),
                    ExceptionPolicy.Transfer);

                int statusCode = (int)this.putTokenResponse.ApplicationProperties.Map[CbsConstants.PutToken.StatusCode];
                string statusDescription = (string)this.putTokenResponse.ApplicationProperties.Map[CbsConstants.PutToken.StatusDescription];
                this.putTokenResponse.Dispose();

                if (statusCode == (int)AmqpResponseStatusCode.Accepted || statusCode == (int)AmqpResponseStatusCode.OK)
                {
                    this.ExpiresAtUtc = token.ExpiresAtUtc;
                }
                else
                {
                    this.Complete(ConvertToException(statusCode, statusDescription));
                }
            }

            static Exception ConvertToException(int statusCode, string statusDescription)
            {
                Exception exception;
                if (Enum.IsDefined(typeof(AmqpResponseStatusCode), statusCode))
                {
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
                }
                else
                {
                    exception = new AmqpException(AmqpErrorCode.InvalidField, AmqpResources.GetString(AmqpResources.AmqpPutTokenFailed, statusCode, statusDescription));
                }

                return exception;
            }
        }
    }
}
