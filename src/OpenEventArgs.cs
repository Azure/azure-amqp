// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using Microsoft.Azure.Amqp.Framing;

    public sealed class OpenEventArgs : EventArgs
    {
        readonly Performative command;

        public OpenEventArgs(Performative command)
        {
            this.command = command;
        }

        public Performative Command
        {
            get { return this.command; }
        }
    }
}
