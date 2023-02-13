// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Sasl
{
    using System;
    using System.Globalization;
    using System.Security.Principal;
    using System.Text;
    using System.Threading.Tasks;

    /// <summary>
    /// A SASL handler for the PLAIN mechanism.
    /// </summary>
    public sealed class SaslPlainHandler : SaslHandler
    {
        /// <summary>
        /// The name of the PLAIN mechanism.
        /// </summary>
        public const string Name = "PLAIN";
        const string InvalidCredential = "Invalid user name or password.";
        ISaslPlainAuthenticator authenticator;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public SaslPlainHandler()
        {
            this.Mechanism = Name;
        }

        /// <summary>
        /// Initializes the object.
        /// </summary>
        /// <param name="authenticator">The SASL PLAIN authenticator.</param>
        public SaslPlainHandler(ISaslPlainAuthenticator authenticator)
            : this()
        {
            this.authenticator = authenticator;
        }

        /// <summary>
        /// Gets or sets the authorization identity.
        /// </summary>
        public string AuthorizationIdentity
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the authentication identity.
        /// </summary>
        public string AuthenticationIdentity
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the password.
        /// </summary>
        public string Password
        {
            get;
            set;
        }

        /// <summary>
        /// Clones the object.
        /// </summary>
        /// <returns>A new SaslPlainHandler object.</returns>
        public override SaslHandler Clone()
        {
            return new SaslPlainHandler(this.authenticator)
            {
                AuthorizationIdentity = this.AuthorizationIdentity,
                AuthenticationIdentity = this.AuthenticationIdentity,
                Password = this.Password
            };
        }

        /// <summary>
        /// Handles the received challenge. It is not implemented by this handler.
        /// </summary>
        /// <param name="challenge">The challenge.</param>
        public override void OnChallenge(SaslChallenge challenge)
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// Handles the received response. It is not implemented by this handler.
        /// </summary>
        /// <param name="response">The response.</param>
        public override void OnResponse(SaslResponse response)
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// Starts the SASL negotiation.
        /// </summary>
        /// <param name="init">The <see cref="SaslInit"/> performative to be sent.</param>
        /// <param name="isClient">true if it is the initiator, otherwise false.</param>
        protected override void OnStart(SaslInit init, bool isClient)
        {
            if (isClient)
            {
                string message = this.GetClientMessage();
                init.InitialResponse = new ArraySegment<byte>(Encoding.UTF8.GetBytes(message));
                this.Negotiator.WriteFrame(init, true);
            }
            else
            {
                this.OnInit(init);
            }
        }

        void OnInit(SaslInit init)
        {
            // the client message is specified by RFC4616
            // message = [authzid] UTF8NUL authcid UTF8NUL passwd
            // authcid and passwd should be prepared [SASLPrep] before
            // the verification process.
            string password = null;
            if (init.InitialResponse.Count > 0)
            {
                string message = Encoding.UTF8.GetString(init.InitialResponse.Array, init.InitialResponse.Offset, init.InitialResponse.Count);
                string[] items = message.Split('\0');
                if (items.Length != 3)
                {
                    throw new UnauthorizedAccessException(SaslPlainHandler.InvalidCredential);
                }

                this.AuthorizationIdentity = items[0];
                this.AuthenticationIdentity = items[1];
                password = items[2];
            }

            if (string.IsNullOrEmpty(this.AuthenticationIdentity))
            {
                throw new UnauthorizedAccessException(SaslPlainHandler.InvalidCredential);
            }

            if (this.authenticator != null)
            {
                this.authenticator.AuthenticateAsync(this.AuthenticationIdentity, password).ContinueWith((t) => this.CompleteNegotiation(t), TaskContinuationOptions.ExecuteSynchronously);
            }
        }

        void CompleteNegotiation(Task<IPrincipal> authenticateTask)
        {
            if (authenticateTask.IsFaulted)
            {
                this.Negotiator.CompleteNegotiation(SaslCode.Sys, authenticateTask.Exception);
            }
            else if (authenticateTask.IsCanceled)
            {
                this.Negotiator.CompleteNegotiation(SaslCode.Sys, new OperationCanceledException());
            }
            else
            {
                this.Principal = authenticateTask.Result;
                this.Negotiator.CompleteNegotiation(SaslCode.Ok, null);
            }
        }

        string GetClientMessage()
        {
            return string.Format(CultureInfo.InvariantCulture, "{0}\0{1}\0{2}", this.AuthorizationIdentity, this.AuthenticationIdentity, this.Password);
        }
    }
}
