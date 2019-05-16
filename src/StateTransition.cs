// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    sealed class StateTransition
    {
        static StateTransition[] sendHeader = new StateTransition[]
        {
            new StateTransition(AmqpObjectState.Start, AmqpObjectState.HeaderSent),
            new StateTransition(AmqpObjectState.HeaderReceived, AmqpObjectState.HeaderExchanged),
        };

        static StateTransition[] sendOpen = new StateTransition[]
        {
            new StateTransition(AmqpObjectState.Start, AmqpObjectState.OpenSent),
            new StateTransition(AmqpObjectState.OpenReceived, AmqpObjectState.Opened),
            new StateTransition(AmqpObjectState.HeaderSent, AmqpObjectState.OpenPipe),
            new StateTransition(AmqpObjectState.HeaderExchanged, AmqpObjectState.OpenSent),
            new StateTransition(AmqpObjectState.ClosePipe, AmqpObjectState.CloseReceived),
        };

        static StateTransition[] sendClose = new StateTransition[]
        {
            new StateTransition(AmqpObjectState.Opened, AmqpObjectState.CloseSent),
            new StateTransition(AmqpObjectState.CloseReceived, AmqpObjectState.End),
            new StateTransition(AmqpObjectState.OpenSent, AmqpObjectState.ClosePipe),
            new StateTransition(AmqpObjectState.OpenPipe, AmqpObjectState.OpenClosePipe),
            new StateTransition(AmqpObjectState.Faulted, AmqpObjectState.Faulted),
        };

        static StateTransition[] receiveHeader = new StateTransition[]
        {
            new StateTransition(AmqpObjectState.Start, AmqpObjectState.HeaderReceived),
            new StateTransition(AmqpObjectState.HeaderSent, AmqpObjectState.HeaderExchanged),
            new StateTransition(AmqpObjectState.OpenPipe, AmqpObjectState.OpenSent),
            new StateTransition(AmqpObjectState.OpenClosePipe, AmqpObjectState.ClosePipe),
        };

        static StateTransition[] receiveOpen = new StateTransition[]
        {
            new StateTransition(AmqpObjectState.Start, AmqpObjectState.OpenReceived),
            new StateTransition(AmqpObjectState.OpenSent, AmqpObjectState.Opened),
            new StateTransition(AmqpObjectState.HeaderReceived, AmqpObjectState.OpenReceived),
            new StateTransition(AmqpObjectState.HeaderExchanged, AmqpObjectState.OpenReceived),
            new StateTransition(AmqpObjectState.ClosePipe, AmqpObjectState.CloseSent),
        };

        static StateTransition[] receiveClose = new StateTransition[]
        {
            new StateTransition(AmqpObjectState.Opened, AmqpObjectState.CloseReceived),
            new StateTransition(AmqpObjectState.CloseSent, AmqpObjectState.End),
            new StateTransition(AmqpObjectState.OpenReceived, AmqpObjectState.ClosePipe),
            new StateTransition(AmqpObjectState.Faulted, AmqpObjectState.End),
        };

        public StateTransition(AmqpObjectState from, AmqpObjectState to)
        {
            this.From = from;
            this.To = to;
        }

        public static StateTransition[] SendHeader
        {
            get { return sendHeader; }
        }

        public static StateTransition[] SendOpen
        {
            get { return sendOpen; }
        }

        public static StateTransition[] SendClose
        {
            get { return sendClose; }
        }

        public static StateTransition[] ReceiveHeader
        {
            get { return receiveHeader; }
        }

        public static StateTransition[] ReceiveOpen
        {
            get { return receiveOpen; }
        }

        public static StateTransition[] ReceiveClose
        {
            get { return receiveClose; }
        }

        public AmqpObjectState From
        {
            get;
            private set;
        }

        public AmqpObjectState To
        {
            get;
            private set;
        }

        public static bool CanTransite(AmqpObjectState from, StateTransition[] states)
        {
            for (int i = 0; i < states.Length; i++)
            {
                if (states[i].From == from)
                {
                    return true;
                }
            }

            return false;
        }
    }
}
