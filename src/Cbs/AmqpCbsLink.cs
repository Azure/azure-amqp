// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
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
            this.linkFactory = new FaultTolerantAmqpObject<RequestResponseAmqpLink>(
                timeout => this.CreateCbsLinkAsync(timeout),
                link => CloseLink(link));

            this.connection.AddExtension(this);
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
        /// <param name="timeout">The operation timeout.</param>
        /// <returns></returns>
        public async Task<DateTime> SendTokenAsync(ICbsTokenProvider tokenProvider, Uri namespaceAddress, string audience, string resource, string[] requiredClaims, TimeSpan timeout)
        {
            if (this.connection.IsClosing())
            {
                throw new OperationCanceledException("Connection is closing or closed.");
            }

            CbsToken token = await tokenProvider.GetTokenAsync(namespaceAddress, resource, requiredClaims).ConfigureAwait(false);
            string tokenType = token.TokenType;
            if (tokenType == null)
            {
                throw new NotSupportedException(AmqpResources.AmqpUnsupportedTokenType);
            }

            RequestResponseAmqpLink requestResponseLink;
            if (!this.linkFactory.TryGetOpenedObject(out requestResponseLink))
            {
                requestResponseLink = await this.linkFactory.GetOrCreateAsync(timeout).ConfigureAwait(false);
            }

            AmqpValue value = new AmqpValue();
            value.Value = token.TokenValue;
            AmqpMessage putTokenRequest = AmqpMessage.Create(value);
            putTokenRequest.ApplicationProperties.Map[CbsConstants.Operation] = CbsConstants.PutToken.OperationValue;
            putTokenRequest.ApplicationProperties.Map[CbsConstants.PutToken.Type] = tokenType;
            putTokenRequest.ApplicationProperties.Map[CbsConstants.PutToken.Audience] = audience;
            putTokenRequest.ApplicationProperties.Map[CbsConstants.PutToken.Expiration] = token.ExpiresAtUtc;

            AmqpMessage putTokenResponse = await requestResponseLink.RequestAsync(putTokenRequest, timeout).ConfigureAwait(false);

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

        static void CloseLink(RequestResponseAmqpLink link)
        {
            AmqpSession session = link.SendingLink?.Session;
            link.Abort();
            session?.SafeClose();
        }

        async Task<RequestResponseAmqpLink> CreateCbsLinkAsync(TimeSpan timeout)
        {
            string address = CbsConstants.CbsAddress;
            TimeoutHelper timeoutHelper = new TimeoutHelper(timeout);
            AmqpSession session = null;
            RequestResponseAmqpLink link = null;
            Exception lastException = null;

            while (timeoutHelper.RemainingTime() > TimeSpan.Zero)
            {
                try
                {
                    AmqpSessionSettings sessionSettings = new AmqpSessionSettings() { Properties = new Fields() };
                    session = this.connection.CreateSession(sessionSettings);
                    await session.OpenAsync(timeoutHelper.RemainingTime()).ConfigureAwait(false);

                    Fields properties = new Fields();
                    properties.Add(CbsConstants.TimeoutName, (uint)timeoutHelper.RemainingTime().TotalMilliseconds);
                    link = new RequestResponseAmqpLink("cbs", session, address, properties);
                    await link.OpenAsync(timeoutHelper.RemainingTime()).ConfigureAwait(false);

                    AmqpTrace.Provider.AmqpOpenEntitySucceeded(this, link.Name, address);
                    return link;
                }
                catch (Exception exception)
                {
                    if (this.connection.IsClosing())
                    {
                        throw new OperationCanceledException("Connection is closing or closed.", exception);
                    }

                    lastException = exception;
                    AmqpTrace.Provider.AmqpOpenEntityFailed(this, this.GetType().Name, address, exception);
                }

                await Task.Delay(1000).ConfigureAwait(false);
            }

            link?.Abort();
            session?.SafeClose();

            throw new TimeoutException(AmqpResources.GetString(AmqpResources.AmqpTimeout, timeout, address), lastException);
        }
    }
}
