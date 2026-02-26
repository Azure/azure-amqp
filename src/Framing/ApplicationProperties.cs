// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using Microsoft.Azure.Amqp.Encoding;

    /// <summary>
    /// Defines the application-properties section of a message.
    /// </summary>
    public sealed class ApplicationProperties : DescribedMap
    {
        /// <summary>
        /// The descriptor name.
        /// </summary>
        public static readonly string Name = "amqp:application-properties:map";
        /// <summary>
        /// The descriptor code.
        /// </summary>
        public static readonly ulong Code = 0x0000000000000074;

        PropertiesMap propMap;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public ApplicationProperties() 
            : base(Name, Code) 
        {
        }

        /// <summary>
        /// Gets the map that stores the properties.
        /// </summary>
        public PropertiesMap Map
        {
            get 
            {
                if (this.propMap == null)
                {
                    this.propMap = new PropertiesMap();
                    this.propMap.SetMap(this.InnerMap);
                }

                return this.propMap; 
            }
        }
    }
}
