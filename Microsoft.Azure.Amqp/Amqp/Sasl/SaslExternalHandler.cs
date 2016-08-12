// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Sasl
{
    using System;
    using System.Security.Principal;

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
    public class SaslExternalHandler : SaslHandler
    {
        public static readonly string Name = "EXTERNAL";

        public SaslExternalHandler()
        {
            this.Mechanism = Name;
        }

        public override SaslHandler Clone()
        {
            return new SaslExternalHandler();
        }

        public override void OnChallenge(SaslChallenge challenge)
        {
            throw new NotImplementedException();
        }

        public override void OnResponse(SaslResponse response)
        {
            throw new NotImplementedException();
        }

        protected override void OnStart(SaslInit init, bool isClient)
        {
            if (isClient)
            {
                this.Negotiator.WriteFrame(init, true);
            }
            else
            {
                // need a principal to mark the transport as 'authenticated'
                this.Principal = new GenericPrincipal(new GenericIdentity("dummy-identity", "dummy-identity"), null);
                // at this point we should check if the client id is established
                // by other means (e.g. cert) and set a Pricipal, but we have
                // been using EXTERNAL to do CBS which is anonymous so we cannot
                // do the check here without breaking old clients
                this.Negotiator.CompleteNegotiation(SaslCode.Ok, null);
            }
        }
    }
}
