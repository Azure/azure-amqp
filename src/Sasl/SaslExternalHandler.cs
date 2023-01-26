// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Sasl
{
    using System;

    // http://tools.ietf.org/html/rfc4422#appendix-A
    //
    // "The EXTERNAL mechanism allows a client to request the server to use
    // credentials established by means external to the mechanism to
    // authenticate the client.  The external means may be, for instance, IP
    // Security [RFC4301] or TLS [RFC4346] services.  In absence of some a
    // priori agreement between the client and the server, the client cannot
    // make any assumption as to what external means the server has used to
    // obtain the client's credentials, nor make an assumption as to the
    // form of credentials."
    /// <summary>
    /// A SASL handler for the EXTERNAL mechanism.
    /// </summary>
    public class SaslExternalHandler : SaslHandler
    {
        /// <summary>
        /// The name of the EXTERNAL mechanism.
        /// </summary>
        public static readonly string Name = "EXTERNAL";

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public SaslExternalHandler()
        {
            this.Mechanism = Name;
        }

        /// <summary>
        /// Clones the object.
        /// </summary>
        /// <returns>A new SaslExternalHandler object.</returns>
        public override SaslHandler Clone()
        {
            return new SaslExternalHandler();
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
                this.Negotiator.WriteFrame(init, true);
            }
            else
            {
                // at this point we should check if the client id is established
                // by other means (e.g. cert) and set a Principal, but we have
                // been using EXTERNAL to do CBS which is anonymous so we cannot
                // do the check here without breaking old clients
                this.Negotiator.CompleteNegotiation(SaslCode.Ok, null);
            }
        }
    }
}
