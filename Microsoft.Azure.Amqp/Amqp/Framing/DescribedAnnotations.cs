// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using Microsoft.Azure.Amqp.Encoding;

    public abstract class DescribedAnnotations : DescribedMap
    {
        Annotations annotations;

        protected DescribedAnnotations(AmqpSymbol name, ulong code) 
            : base(name, code) 
        {
        }

        public Annotations Map
        {
            get 
            {
                if (this.annotations == null)
                {
                    this.annotations = new Annotations();
                    this.Value = this.annotations;
                }

                return this.annotations; 
            }
        }

        internal override AmqpMap InnerMap => this.Map;
    }
}
