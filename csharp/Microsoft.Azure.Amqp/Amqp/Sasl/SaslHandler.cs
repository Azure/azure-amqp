// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Sasl
{
    using System;
    using System.Security.Principal;
    
    public abstract class SaslHandler
    {
        SaslNegotiator saslNegotiator;

        public string Mechanism
        {
            get;
            protected set;
        }

        public IPrincipal Principal
        {
            get;
            protected set;
        }

        protected SaslNegotiator Negotiator
        {
            get { return this.saslNegotiator; }
        }

        public void Start(SaslNegotiator saslNegotiator, SaslInit init, bool isClient)
        {
            this.saslNegotiator = saslNegotiator;

            try
            {
                this.OnStart(init, isClient);
            }
            catch (Exception exception)
            {
                if (Fx.IsFatal(exception))
                {
                    throw;
                }

                this.saslNegotiator.CompleteNegotiation(SaslCode.Sys, exception);
            }
        }

        public override string ToString()
        {
            return this.Mechanism;
        }

        public abstract SaslHandler Clone();

        public abstract void OnChallenge(SaslChallenge challenge);

        public abstract void OnResponse(SaslResponse response);

        protected abstract void OnStart(SaslInit init, bool isClient);
    }
}
