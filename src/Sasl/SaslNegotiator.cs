// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Sasl
{
    using System;
    using System.Collections.Generic;
    using System.Text;
    using System.Threading;
    using Microsoft.Azure.Amqp.Encoding;
    using Microsoft.Azure.Amqp.Framing;
    using Microsoft.Azure.Amqp.Transport;

    /// <summary>
    /// This class performs the SASL negotiatioin.
    /// </summary>
    public sealed class SaslNegotiator : IIoHandler
    {
        enum SaslState
        {
            Start,
            WaitingForServerMechanisms,
            WaitingForInit,
            Negotiating,
            End
        }

        static readonly string welcome = "Welcome!";

        static readonly Action<TransportAsyncCallbackArgs> onWriteFrameComplete = OnWriteFrameComplete;
        readonly SaslTransport transport;
        readonly SaslTransportProvider provider;
        readonly bool isInitiator;
        SaslState state;
        SaslHandler saslHandler;
        AsyncIO.FrameBufferReader reader;
        AsyncIO.AsyncBufferWriter writer;
        Exception completeException;
        int completeTransport;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        /// <param name="transport">The transport.</param>
        /// <param name="provider">The SASL transport provider.</param>
        /// <param name="isInitiator">true if it is the initiator, false otherwise.</param>
        public SaslNegotiator(SaslTransport transport, SaslTransportProvider provider, bool isInitiator)
        {
            this.transport = transport;
            this.provider = provider;
            this.isInitiator = isInitiator;
            this.state = SaslState.Start;
        }

        /// <summary>
        /// Starts the negotiation.
        /// </summary>
        /// <returns>true if the negotiation is completed, false otherwise.</returns>
        public bool Start()
        {
            this.reader = new AsyncIO.FrameBufferReader(this, transport, this.provider.MaxFrameSize);
            this.writer = new AsyncIO.AsyncBufferWriter(transport);

            if (!this.isInitiator)
            {
                this.SendServerMechanisms();
            }
            else
            {
                this.state = SaslState.WaitingForServerMechanisms;
                AmqpTrace.Provider.AmqpLogOperationVerbose(this, TraceOperation.Execute, "WaitingForServerMechanisms");
                this.ReadFrame();
            }

            return false;
        }

        /// <summary>
        /// Starts reading a SASL frame from the transport.
        /// </summary>
        public void ReadFrame()
        {
            try
            {
                this.reader.ReadFrame();
            }
            catch (Exception exception) when (!Fx.IsFatal(exception))
            {
                this.HandleException(nameof(ReadFrame), exception);
            }
        }

        /// <summary>
        /// Writes a SASL performative.
        /// </summary>
        /// <param name="command">The SASL performative.</param>
        /// <param name="needReply">true if a response is needed, false otherwise.</param>
        public void WriteFrame(Performative command, bool needReply)
        {
            try
            {
#if DEBUG
                Frame frame = new Frame(FrameType.Sasl) { Command = command };
                frame.Trace(true, null);
                AmqpTrace.Provider.AmqpLogOperationVerbose(this, TraceOperation.Send, frame);
#endif

                ByteBuffer buffer = Frame.EncodeCommand(FrameType.Sasl, 0, command, 0);
                TransportAsyncCallbackArgs args = new TransportAsyncCallbackArgs();
                args.SetWriteBuffer(buffer);
                args.CompletedCallback = onWriteFrameComplete;
                args.UserToken = this;
                args.UserToken2 = needReply;
                this.writer.WriteBuffer(args);
            }
            catch (Exception exception) when (!Fx.IsFatal(exception))
            {
                this.HandleException("WriteFrame", exception);
            }
        }

        /// <summary>
        /// Completes the negotiation.
        /// </summary>
        /// <param name="code">The code.</param>
        /// <param name="exception">The exception.</param>
        public void CompleteNegotiation(SaslCode code, Exception exception)
        {
            this.state = SaslState.End;
            this.completeException = exception;

            if (!this.isInitiator)
            {
                SaslOutcome outcome = new SaslOutcome();
                outcome.OutcomeCode = code;
                if (code == SaslCode.Ok)
                {
                    outcome.AdditionalData = new ArraySegment<byte>(Encoding.UTF8.GetBytes(welcome));
                }

                // transported is completed in the callback
                this.WriteFrame(outcome, false);
            }
            else
            {
                this.CompleteTransport();
            }
        }

        /// <summary>
        /// Gets a string representing the object.
        /// </summary>
        /// <returns>A string representing the object.</returns>
        public override string ToString()
        {
            return "sasl-negotiator";
        }

        static void OnWriteFrameComplete(TransportAsyncCallbackArgs args)
        {
            args.ByteBuffer.Dispose();
            var thisPtr = (SaslNegotiator)args.UserToken;
            if (args.Exception != null)
            {
                thisPtr.HandleException(nameof(OnWriteFrameComplete), args.Exception);
            }
            else
            {
                bool readFrame = (bool)args.UserToken2;
                if (readFrame)
                {
                    thisPtr.ReadFrame();
                }
                else if (thisPtr.state == SaslState.End)
                {
                    thisPtr.CompleteTransport();
                }
            }
        }

        void CompleteTransport()
        {
            if (Interlocked.Exchange(ref this.completeTransport, 1) == 0)
            {
                if (this.completeException != null)
                {
                    this.transport.OnNegotiationFail(this.completeException);
                }
                else
                {
                    this.transport.OnNegotiationSucceed(this.saslHandler.Principal);
                }
            }
        }

        void HandleException(string action, Exception exception)
        {
            AmqpTrace.Provider.AmqpLogError(this, action, exception);
            this.state = SaslState.End;
            this.completeException = exception;
            this.CompleteTransport();
        }

        void SendServerMechanisms()
        {
            List<AmqpSymbol> mechanisms = new List<AmqpSymbol>();
            foreach (string mechanism in this.provider.Mechanisms)
            {
                mechanisms.Add(new AmqpSymbol(mechanism));
            }

            SaslMechanisms salsMechanisms = new SaslMechanisms();
            salsMechanisms.SaslServerMechanisms = new Multiple<AmqpSymbol>(mechanisms);
            this.state = SaslState.WaitingForInit;
            this.WriteFrame(salsMechanisms, true);
        }

        void IIoHandler.OnIoFault(Exception exception)
        {
            this.HandleException("OnIoFault", exception);
        }

        ByteBuffer IIoHandler.CreateBuffer(int frameSize)
        {
            var buffer = new ByteBuffer(frameSize, false);
            AmqpBitConverter.WriteUInt(buffer, (uint)frameSize);
            return buffer;
        }

        void IIoHandler.OnReceiveBuffer(ByteBuffer buffer)
        {
            Frame frame = new Frame();
            try
            {
                frame.Decode(buffer);
#if DEBUG
                frame.Trace(false, null);
                AmqpTrace.Provider.AmqpLogOperationVerbose(this, TraceOperation.Receive, frame);
#endif

                if (frame.Type != FrameType.Sasl)
                {
                    throw new AmqpException(AmqpErrorCode.InvalidField, "sasl-frame-type");
                }

                if (frame.Command == null)
                {
                    throw new AmqpException(AmqpErrorCode.InvalidField, "sasl-frame-body");
                }
            }
            catch (Exception exp) when (!Fx.IsFatal(exp))
            {
                AmqpTrace.Provider.AmqpLogError(this, "SaslDecode", exp);
                this.CompleteNegotiation(SaslCode.Sys, exp);
                return;
            }

            try
            {
                this.HandleSaslCommand(frame.Command);
            }
            catch (UnauthorizedAccessException authzExp)
            {
                AmqpTrace.Provider.AmqpLogError(this, "Authorize", authzExp);
                this.CompleteNegotiation(SaslCode.Auth, authzExp);
            }
            catch (Exception exp) when (!Fx.IsFatal(exp))
            {
                AmqpTrace.Provider.AmqpLogError(this, "HandleSaslCommand", exp);
                this.CompleteNegotiation(SaslCode.Sys, exp);
            }
        }

        void IIoHandler.OnIoEvent(IoEvent ioEvent, long queueSize)
        {
        }

        void HandleSaslCommand(Performative command)
        {
            if (command.DescriptorCode == SaslMechanisms.Code)
            {
                this.OnSaslServerMechanisms((SaslMechanisms)command);
            }
            else if (command.DescriptorCode == SaslInit.Code)
            {
                this.OnSaslInit((SaslInit)command);
            }
            else if (command.DescriptorCode == SaslChallenge.Code)
            {
                this.saslHandler.OnChallenge((SaslChallenge)command);
            }
            else if (command.DescriptorCode == SaslResponse.Code)
            {
                this.saslHandler.OnResponse((SaslResponse)command);
            }
            else if (command.DescriptorCode == SaslOutcome.Code)
            {
                this.OnSaslOutcome((SaslOutcome)command);
            }
            else
            {
                throw new AmqpException(AmqpErrorCode.NotAllowed, command.ToString());
            }
        }

        /// <summary>
        /// Client receives the announced server mechanisms.
        /// </summary>
        void OnSaslServerMechanisms(SaslMechanisms mechanisms)
        {
            if (this.state != SaslState.WaitingForServerMechanisms)
            {
                throw new AmqpException(AmqpErrorCode.IllegalState, AmqpResources.GetString(AmqpResources.AmqpIllegalOperationState, "R:SASL-MECH", this.state));
            }

            string mechanismToUse = null;
            foreach (string mechanism in this.provider.Mechanisms)
            {
                if (mechanisms.SaslServerMechanisms.Contains(new AmqpSymbol(mechanism)))
                {
                    mechanismToUse = mechanism;
                    break;
                }

                if (mechanismToUse != null)
                {
                    break;
                }
            }

            if (mechanismToUse == null)
            {
                throw new AmqpException(
                    AmqpErrorCode.NotFound,
                    AmqpResources.GetString(AmqpResources.AmqpNotSupportMechanism, mechanisms.SaslServerMechanisms.ToString(), string.Join(",", this.provider.Mechanisms)));
            }

            this.state = SaslState.Negotiating;
            this.saslHandler = this.provider.GetHandler(mechanismToUse, true);
            SaslInit init = new SaslInit();
            init.Mechanism = mechanismToUse;
            this.saslHandler.Start(this, init, true);
        }

        /// <summary>
        /// Server receives the client init that may contain the initial response message.
        /// </summary>
        void OnSaslInit(SaslInit init)
        {
            if (this.state != SaslState.WaitingForInit)
            {
                throw new AmqpException(AmqpErrorCode.IllegalState, AmqpResources.GetString(AmqpResources.AmqpIllegalOperationState, "R:SASL-INIT", this.state));
            }

            this.state = SaslState.Negotiating;
            this.saslHandler = this.provider.GetHandler(init.Mechanism.Value, true);
            this.saslHandler.Start(this, init, false);
        }

        /// <summary>
        /// Client receives the sasl outcome from the server.
        /// </summary>
        void OnSaslOutcome(SaslOutcome outcome)
        {
            Exception exception = null;
            if (outcome.OutcomeCode.Value != SaslCode.Ok)
            {
                exception = new UnauthorizedAccessException(outcome.OutcomeCode.Value.ToString());
            }

            this.CompleteNegotiation(outcome.OutcomeCode.Value, exception);
        }
    }
}
