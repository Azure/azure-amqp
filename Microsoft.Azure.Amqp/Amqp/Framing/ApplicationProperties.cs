// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using Microsoft.Azure.Amqp.Encoding;

    public sealed class ApplicationProperties : DescribedMap
    {
        public static readonly string Name = "amqp:application-properties:map";
        public static readonly ulong Code = 0x0000000000000074;

        PropertiesMap propMap;

        public ApplicationProperties() 
            : base(Name, Code) 
        {
        }

        public PropertiesMap Map
        {
            get 
            {
                if (this.propMap == null)
                {
                    this.propMap = new PropertiesMap();
                    this.Value = this.propMap;
                }

                return this.propMap; 
            }
        }

        internal override AmqpMap InnerMap => this.Map;
    }
}
