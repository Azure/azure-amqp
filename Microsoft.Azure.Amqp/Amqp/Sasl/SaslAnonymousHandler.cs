// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Sasl
{
    using System;
    using System.Text;

    public sealed class SaslAnonymousHandler : SaslHandler
    {
        public static readonly string Name = "ANONYMOUS";

        public SaslAnonymousHandler()
            : this(Name)
        {
        }

        public SaslAnonymousHandler(string name)
        {
            if (string.IsNullOrEmpty(name))
            {
                throw Fx.Exception.ArgumentNullOrEmpty("name");
            }

            // Allow users to create anonymous handler with other names
            this.Mechanism = name;
        }

        public string Identity
        {
            get;
            set;
        }

        public override SaslHandler Clone()
        {
            return new SaslAnonymousHandler(this.Mechanism);
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
