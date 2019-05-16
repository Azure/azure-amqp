// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Sasl
{
    using System;
    using System.Security.Principal;
    
    /// <summary>
    /// The base class of SASL handlers.
    /// </summary>
    public abstract class SaslHandler
    {
        SaslNegotiator saslNegotiator;

        /// <summary>
        /// Gets or sets the SASL mechanism.
        /// </summary>
        public string Mechanism
        {
            get;
            protected set;
        }

        /// <summary>
        /// Gets or sets the principal after authentication is done.
        /// </summary>
        public IPrincipal Principal
        {
            get;
            protected set;
        }

        /// <summary>
        /// Gets the SaslNegotiator.
        /// </summary>
        protected SaslNegotiator Negotiator
        {
            get { return this.saslNegotiator; }
        }

        /// <summary>
        /// Starts the SASL negotiation.
        /// </summary>
        /// <param name="saslNegotiator">The SaslNegotiator.</param>
        /// <param name="init">The <see cref="SaslInit"/> performative to be sent.</param>
        /// <param name="isClient">true if it is the initiator, otherwise false.</param>
        public void Start(SaslNegotiator saslNegotiator, SaslInit init, bool isClient)
        {
            this.saslNegotiator = saslNegotiator;

            try
            {
                this.OnStart(init, isClient);
            }
            catch (Exception exception) when (!Fx.IsFatal(exception))
            {
                this.saslNegotiator.CompleteNegotiation(SaslCode.Sys, exception);
            }
        }

        /// <summary>
        /// Gets a string representing the object.
        /// </summary>
        /// <returns>A string representing the object.</returns>
        public override string ToString()
        {
            return this.Mechanism;
        }

        /// <summary>
        /// When overriden in a derived class, clones the handler.
        /// </summary>
        /// <returns></returns>
        public abstract SaslHandler Clone();

        /// <summary>
        /// When overriden in a derived class, handles the received challenge.
        /// </summary>
        /// <param name="challenge">The challenge.</param>
        public abstract void OnChallenge(SaslChallenge challenge);

        /// <summary>
        /// When overriden in a derived class, handles the received response.
        /// </summary>
        /// <param name="response">The response.</param>
        public abstract void OnResponse(SaslResponse response);

        /// <summary>
        /// When overriden in a derived class, starts the SASL negotiation.
        /// </summary>
        /// <param name="init">The <see cref="SaslInit"/> performative to be sent.</param>
        /// <param name="isClient">true if it is the initiator, otherwise false.</param>
        protected abstract void OnStart(SaslInit init, bool isClient);
    }
}
