// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    /// <summary>
    /// Represents a state transition in the AMQP object state machine.
    /// </summary>
    public sealed class StateTransition
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
        };

        /// <summary>
        /// Initializes a new instance with source and target states.
        /// </summary>
        /// <param name="from">The source state.</param>
        /// <param name="to">The target state.</param>
        public StateTransition(AmqpObjectState from, AmqpObjectState to)
        {
            this.From = from;
            this.To = to;
        }

        /// <summary>Gets the valid transitions for sending a protocol header.</summary>
        public static StateTransition[] SendHeader
        {
            get { return sendHeader; }
        }

        /// <summary>Gets the valid transitions for sending an open frame.</summary>
        public static StateTransition[] SendOpen
        {
            get { return sendOpen; }
        }

        /// <summary>Gets the valid transitions for sending a close frame.</summary>
        public static StateTransition[] SendClose
        {
            get { return sendClose; }
        }

        /// <summary>Gets the valid transitions for receiving a protocol header.</summary>
        public static StateTransition[] ReceiveHeader
        {
            get { return receiveHeader; }
        }

        /// <summary>Gets the valid transitions for receiving an open frame.</summary>
        public static StateTransition[] ReceiveOpen
        {
            get { return receiveOpen; }
        }

        /// <summary>Gets the valid transitions for receiving a close frame.</summary>
        public static StateTransition[] ReceiveClose
        {
            get { return receiveClose; }
        }

        /// <summary>Gets the source state of this transition.</summary>
        public AmqpObjectState From
        {
            get;
            private set;
        }

        /// <summary>Gets the target state of this transition.</summary>
        public AmqpObjectState To
        {
            get;
            private set;
        }

        /// <summary>
        /// Checks if a transition from the specified state is valid.
        /// </summary>
        /// <param name="from">The current state.</param>
        /// <param name="states">The allowed transitions.</param>
        /// <returns>true if the transition is valid; false otherwise.</returns>
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
