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
        readonly IPrincipal principal;

        public SaslExternalHandler()
            : this(new GenericPrincipal(new GenericIdentity("dummy-identity", "dummy-identity"), null))
        {
        }

        public SaslExternalHandler(IPrincipal principal)
        {
            this.Mechanism = Name;
            this.principal = principal;
        }

        public override SaslHandler Clone()
        {
            return new SaslExternalHandler(this.principal);
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
                this.SetPrincipal();
                this.Negotiator.CompleteNegotiation(SaslCode.Ok, null);
            }
        }

        void SetPrincipal()
        {
            this.Principal = this.principal;
        }
    }
}
