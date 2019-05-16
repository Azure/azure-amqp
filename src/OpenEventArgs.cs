// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using Microsoft.Azure.Amqp.Framing;

    /// <summary>
    /// Contains the object specific performative for <see cref="AmqpObject.Opening"/>
    /// and <see cref="AmqpObject.Opened"/> events.
    /// </summary>
    public sealed class OpenEventArgs : EventArgs
    {
        readonly Performative command;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        /// <param name="command">The performative.</param>
        public OpenEventArgs(Performative command)
        {
            this.command = command;
        }

        /// <summary>
        /// Gets the performative.
        /// </summary>
        public Performative Command
        {
            get { return this.command; }
        }
    }
}
