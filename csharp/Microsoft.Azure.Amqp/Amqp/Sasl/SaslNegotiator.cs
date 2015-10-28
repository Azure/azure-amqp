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
    using Microsoft.Azure.Amqp.Tracing;

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

        public SaslNegotiator(SaslTransport transport, SaslTransportProvider provider, bool isInitiator)
        {
            this.transport = transport;
            this.provider = provider;
            this.isInitiator = isInitiator;
            this.state = SaslState.Start;
        }

        public bool Start()
        {
            this.reader = new AsyncIO.FrameBufferReader(this, transport);
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

        public void ReadFrame()
        {
            try
            {
                this.reader.ReadFrame();
            }
            catch (Exception exception)
            {
                if (Fx.IsFatal(exception))
                {
                    throw;
                }

                this.HandleException("ReadFrame", exception);
            }
        }

        public void WriteFrame(Performative command, bool needReply)
        {
            try
            {
#if DEBUG
                Frame frame = new Frame(FrameType.Sasl) { Command = command };
                frame.Trace(true);
                AmqpTrace.Provider.AmqpLogOperationVerbose(this, TraceOperation.Send, frame);
#endif

                ByteBuffer buffer = Frame.EncodeCommand(FrameType.Sasl, 0, command, 0);
                TransportAsyncCallbackArgs args = new TransportAsyncCallbackArgs();
                args.SetBuffer(buffer);
                args.CompletedCallback = onWriteFrameComplete;
                args.UserToken = this;
                args.UserToken2 = needReply;
                this.writer.WriteBuffer(args);
            }
            catch (Exception exception)
            {
                if (Fx.IsFatal(exception))
                {
                    throw;
                }

                this.HandleException("WriteFrame", exception);
            }
        }

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

        public override string ToString()
        {
            return "sasl-negotiator";
        }

        static void OnWriteFrameComplete(TransportAsyncCallbackArgs args)
        {
            var thisPtr = (SaslNegotiator)args.UserToken;
            if (args.Exception != null)
            {
                thisPtr.HandleException("OnWriteFrameComplete", args.Exception);
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
            AmqpTrace.Provider.AmqpLogError(this, action, exception.Message);
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

        void IIoHandler.OnReceiveBuffer(ByteBuffer buffer)
        {
            Frame frame = new Frame();
            try
            {
                frame.Decode(buffer);
#if DEBUG
                frame.Trace(false);
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
            catch (Exception exp)
            {
                if (Fx.IsFatal(exp))
                {
                    throw;
                }

                AmqpTrace.Provider.AmqpLogError(this, "SaslDecode", exp.Message);
                this.CompleteNegotiation(SaslCode.Sys, exp);
                return;
            }

            try
            {
                this.HandleSaslCommand(frame.Command);
            }
            catch (UnauthorizedAccessException authzExp)
            {
                AmqpTrace.Provider.AmqpLogError(this, "Authorize", authzExp.Message);
                this.CompleteNegotiation(SaslCode.Auth, authzExp);
            }
            catch (Exception exp)
            {
                if (Fx.IsFatal(exp))
                {
                    throw;
                }

                AmqpTrace.Provider.AmqpLogError(this, "HandleSaslCommand", exp.Message);
                this.CompleteNegotiation(SaslCode.Sys, exp);
            }
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
