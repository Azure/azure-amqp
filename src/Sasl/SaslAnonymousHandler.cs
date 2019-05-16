// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Sasl
{
    using System;
    using System.Text;

    /// <summary>
    /// A SASL handler for the ANONYMOUS mechanism.
    /// </summary>
    public sealed class SaslAnonymousHandler : SaslHandler
    {
        /// <summary>
        /// The name of the ANONYMOUS mechanism.
        /// </summary>
        public static readonly string Name = "ANONYMOUS";

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public SaslAnonymousHandler()
            : this(Name)
        {
        }

        /// <summary>
        /// Initializes the object with a different mechanism name.
        /// </summary>
        /// <param name="name">The mechamism.</param>
        public SaslAnonymousHandler(string name)
        {
            if (string.IsNullOrEmpty(name))
            {
                throw new ArgumentNullException ("name");
            }

            // Allow users to create anonymous handler with other names
            this.Mechanism = name;
        }

        /// <summary>
        /// The identity name.
        /// </summary>
        public string Identity
        {
            get;
            set;
        }

        /// <summary>
        /// Clones the object.
        /// </summary>
        /// <returns>A new SaslAnonymousHandler object.</returns>
        public override SaslHandler Clone()
        {
            return new SaslAnonymousHandler(this.Mechanism);
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
                if (this.Identity != null)
                {
                    init.InitialResponse = new ArraySegment<byte>(Encoding.UTF8.GetBytes(this.Identity));
                }

                this.Negotiator.WriteFrame(init, true);
            }
            else
            {
                // server side. send outcome
                this.Negotiator.CompleteNegotiation(SaslCode.Ok, null);
            }
        }
    }
}
