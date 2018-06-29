// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;
    using System.Threading.Tasks;
    using Microsoft.Azure.Amqp.Framing;

    /// <summary>
    /// Encapsulates a pair of links to '$cbs' for managing CBS tokens
    /// </summary>
    public sealed class AmqpCbsLink
    {
        readonly AmqpConnection connection;
        readonly FaultTolerantAmqpObject<RequestResponseAmqpLink> linkFactory;

        /// <summary>
        /// Constructs a new instance
        /// </summary>
        public AmqpCbsLink(AmqpConnection connection)
        {
            this.connection = connection ?? throw new ArgumentNullException(nameof(connection));
            this.linkFactory = new FaultTolerantAmqpObject<RequestResponseAmqpLink>(
                t => TaskHelpers.CreateTask<RequestResponseAmqpLink>((c, s) => this.BeginCreateCbsLink(t, c, s), this.EndCreateCbsLink),
                link => CloseLink(link));

            this.connection.Extensions.Add(this);
        }

        public void Close()
        {
            this.linkFactory.Close();
        }

        public Task<DateTime> SendTokenAsync(ICbsTokenProvider tokenProvider, Uri namespaceAddress, string audience, string resource, string[] requiredClaims, TimeSpan timeout)
        {
            return TaskHelpers.CreateTask(
                (c, s) => this.BeginSendToken(
                    tokenProvider, namespaceAddress, audience, resource, requiredClaims, timeout, c, s),
                (a) => this.EndSendToken(a));
        }

        public IAsyncResult BeginSendToken(ICbsTokenProvider tokenProvider, Uri namespaceAddress, string audience, string resource, string[] requiredClaims, TimeSpan timeout, AsyncCallback callback, object state)
        {
            if (tokenProvider == null || namespaceAddress == null || audience == null || resource == null || requiredClaims == null)
            {
                throw new ArgumentNullException(
                    tokenProvider == null ? "tokenProvider" : namespaceAddress == null ? "namespaceAddress" : audience == null ? "audience" : resource == null ? "resource" : "requiredClaims");
            }

            if (this.connection.IsClosing())
            {
                throw new ObjectDisposedException(CbsConstants.CbsAddress);
            }

            return new SendTokenAsyncResult(this, tokenProvider, namespaceAddress, audience, resource, requiredClaims, timeout, callback, state);
        }

        public DateTime EndSendToken(IAsyncResult result)
        {
            return SendTokenAsyncResult.End(result).ExpiresAtUtc;
        }

        static void CloseLink(RequestResponseAmqpLink link)
        {
            AmqpSession session = link.SendingLink?.Session;
            link.Abort();
            session?.SafeClose();
        }

        IAsyncResult BeginCreateCbsLink(TimeSpan timeout, AsyncCallback callback, object state)
        {
            return new OpenCbsRequestResponseLinkAsyncResult(this.connection, timeout, callback, state);
        }

        RequestResponseAmqpLink EndCreateCbsLink(IAsyncResult result)
        {
            RequestResponseAmqpLink link = OpenCbsRequestResponseLinkAsyncResult.End(result).Link;
            return link;
        }

        sealed class OpenCbsRequestResponseLinkAsyncResult : IteratorAsyncResult<OpenCbsRequestResponseLinkAsyncResult>, ILinkFactory
        {
            readonly AmqpConnection connection;
            AmqpSession session = null;

            public OpenCbsRequestResponseLinkAsyncResult(AmqpConnection connection, TimeSpan timeout, AsyncCallback callback, object state)
                : base(timeout, callback, state)
            {
                this.connection = connection;

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
                        this.session = new AmqpSession(this.connection, sessionSettings, this);
                        connection.AddSession(session, null);
                    }
                    catch (InvalidOperationException exception)
                    {
                        this.Complete(exception);
                        yield break;
                    }

                    yield return this.CallAsync(
                        (thisPtr, t, c, s) => thisPtr.session.BeginOpen(t, c, s),
                        (thisPtr, r) => thisPtr.session.EndOpen(r),
                        ExceptionPolicy.Continue);

                    Exception lastException = this.LastAsyncStepException;
                    if (lastException != null)
                    {
                        AmqpTrace.Provider.AmqpOpenEntityFailed(this.connection, this.session, string.Empty, address, lastException.Message);
                        this.session.Abort();
                        this.Complete(lastException);
                        yield break;
                    }

                    Fields properties = new Fields();
                    properties.Add(CbsConstants.TimeoutName, (uint)this.RemainingTime().TotalMilliseconds);
                    this.Link = new RequestResponseAmqpLink("cbs", this.session, address, properties);
                    yield return this.CallAsync(
                        (thisPtr, t, c, s) => thisPtr.Link.BeginOpen(t, c, s),
                        (thisPtr, r) => thisPtr.Link.EndOpen(r),
                        ExceptionPolicy.Continue);

                    lastException = this.LastAsyncStepException;
                    if (lastException != null)
                    {
                        AmqpTrace.Provider.AmqpOpenEntityFailed(this.connection, this.Link, this.Link.Name, address, lastException.Message);
                        this.session.SafeClose();
                        this.Link = null;
                        this.Complete(lastException);
                        yield break;
                    }

                    AmqpTrace.Provider.AmqpOpenEntitySucceeded(this.connection, this.Link, this.Link.Name, address);
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
            CbsToken token;
            Task<RequestResponseAmqpLink> requestResponseLinkTask;

            public SendTokenAsyncResult(
                AmqpCbsLink cbsLink,
                ICbsTokenProvider tokenProvider,
                Uri namespaceAddress,
                string audience,
                string resource,
                string[] requiredClaims,
                TimeSpan timeout,
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

                this.Start();
            }

            public DateTime ExpiresAtUtc { get; private set; }

            protected override IEnumerator<AsyncStep> GetAsyncSteps()
            {
                Task<CbsToken> getTokenTask = null;
                yield return this.CallTask(
                    (thisPtr, t) => getTokenTask = thisPtr.tokenProvider.GetTokenAsync(thisPtr.namespaceAddress, thisPtr.resource, thisPtr.requiredClaims),
                    ExceptionPolicy.Transfer);
                this.token = getTokenTask.Result;

                string tokenType = this.token.TokenType;
                if (tokenType == null)
                {
                    this.Complete(new InvalidOperationException(AmqpResources.AmqpUnsupportedTokenType));
                    yield break;
                }

                RequestResponseAmqpLink requestResponseLink;
                if (this.cbsLink.linkFactory.TryGetOpenedObject(out requestResponseLink))
                {
                    this.requestResponseLinkTask = Task.FromResult(requestResponseLink);
                }
                else
                {
                    yield return this.CallTask(
                        (thisPtr, t) => thisPtr.requestResponseLinkTask = thisPtr.cbsLink.linkFactory.GetOrCreateAsync(t),
                        ExceptionPolicy.Transfer);
                }

                AmqpValue value = new AmqpValue();
                value.Value = this.token.TokenValue;
                AmqpMessage putTokenRequest = AmqpMessage.Create(value);
                putTokenRequest.ApplicationProperties = new ApplicationProperties();
                putTokenRequest.ApplicationProperties.Map[CbsConstants.Operation] = CbsConstants.PutToken.OperationValue;
                putTokenRequest.ApplicationProperties.Map[CbsConstants.PutToken.Type] = tokenType;
                putTokenRequest.ApplicationProperties.Map[CbsConstants.PutToken.Audience] = this.audience;
                putTokenRequest.ApplicationProperties.Map[CbsConstants.PutToken.Expiration] = this.token.ExpiresAtUtc;

                AmqpMessage putTokenResponse = null;
                Fx.AssertIsNotNull(this.requestResponseLinkTask.Result, "requestResponseLink cannot be null without exception");
                yield return this.CallAsync(
                    (thisPtr, t, c, s) => thisPtr.requestResponseLinkTask.Result.BeginRequest(putTokenRequest, t, c, s),
                    (thisPtr, r) => putTokenResponse = thisPtr.requestResponseLinkTask.Result.EndRequest(r),
                    ExceptionPolicy.Transfer);

                int statusCode = (int)putTokenResponse.ApplicationProperties.Map[CbsConstants.PutToken.StatusCode];
                string statusDescription = (string)putTokenResponse.ApplicationProperties.Map[CbsConstants.PutToken.StatusDescription];
                if (statusCode == (int)AmqpResponseStatusCode.Accepted || statusCode == (int)AmqpResponseStatusCode.OK)
                {
                    this.ExpiresAtUtc = this.token.ExpiresAtUtc;
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
