// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using System;
    using System.Runtime.Serialization;
    using System.Text;
    using System.Transactions;
    using Microsoft.Azure.Amqp.Encoding;

    [Serializable]
    public sealed class Error : DescribedList, ISerializable
    {
        public static readonly string Name = "amqp:error:list";
        public static readonly ulong Code = 0x000000000000001d;
        const int Fields = 3;
        const int MaxSizeInInfoMap = 8 * 1024;

        public Error() : base(Name, Code)
        {
        }

        Error(SerializationInfo info, StreamingContext context)
            : base(Name, Code)
        {
            this.Condition = (string)info.GetValue("Condition", typeof(string));
            this.Description = (string)info.GetValue("Description", typeof(string));
        }

        public AmqpSymbol Condition { get; set; }

        public string Description { get; set; }

        public Fields Info { get; set; }

        protected override int FieldCount
        {
            get { return Fields; }
        }

        // This list should have non-SB related exceptions. The contract that handles SB exceptions
        // are in ExceptionHelper
        public static Error FromException(Exception exception, bool includeDetail = true)
        {
            AmqpException amqpException = exception as AmqpException;
            if (amqpException != null)
            {
                return amqpException.Error;
            }

            Error error = new Error();
            if (exception is UnauthorizedAccessException)
            {
                error.Condition = AmqpErrorCode.UnauthorizedAccess;
            }
            else if (exception is InvalidOperationException)
            {
                error.Condition = AmqpErrorCode.NotAllowed;
            }
            else if (exception is TransactionAbortedException)
            {
                error.Condition = AmqpErrorCode.TransactionRollback;
            }
            else if (exception is NotImplementedException)
            {
                error.Condition = AmqpErrorCode.NotImplemented;
            }
            else
            {
                error.Condition = AmqpErrorCode.InternalError;
            }

            error.Description = exception.Message;
            if (includeDetail)
            {
                error.Info = new Fields();

                // Limit the size of the exception string as it may exceed the conneciton max frame size
                string exceptionString = exception.ToString();
                if (exceptionString.Length > MaxSizeInInfoMap)
                {
                    exceptionString = exceptionString.Substring(0, MaxSizeInInfoMap);
                }

                error.Info.Add("exception", exceptionString);
            }

            return error;
        }
        
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder("error(");
            int count = 0;
            this.AddFieldToString(this.Condition.Value != null, sb, "condition", this.Condition, ref count);
            this.AddFieldToString(this.Description != null, sb, "description", this.Description, ref count);
            this.AddFieldToString(this.Info != null, sb, "info", this.Info, ref count);
            sb.Append(')');
            return sb.ToString();
        }

        void ISerializable.GetObjectData(SerializationInfo info, StreamingContext context)
        {
            // The inner types aren't actually serializable, instead we serialize
            // Condition and Description as strings
            info.AddValue("Condition", this.Condition.Value);
            info.AddValue("Description", this.Description);
        }

        protected override void EnsureRequired()
        {
            if (this.Condition.Value == null)
            {
                throw AmqpEncoding.GetEncodingException(AmqpResources.GetString(AmqpResources.AmqpRequiredFieldNotSet, "condition", Name));
            }
        }

        protected override void OnEncode(ByteBuffer buffer)
        {
            AmqpCodec.EncodeSymbol(this.Condition, buffer);
            AmqpCodec.EncodeString(this.Description, buffer);
            AmqpCodec.EncodeMap(this.Info, buffer);
        }

        protected override void OnDecode(ByteBuffer buffer, int count)
        {
            if (count-- > 0)
            {
                this.Condition = AmqpCodec.DecodeSymbol(buffer);
            }

            if (count-- > 0)
            {
                this.Description = AmqpCodec.DecodeString(buffer);
            }

            if (count-- > 0)
            {
                this.Info = AmqpCodec.DecodeMap<Fields>(buffer);
            }
        }

        protected override int OnValueSize()
        {
            int valueSize = 0;

            valueSize = AmqpCodec.GetSymbolEncodeSize(this.Condition);
            valueSize += AmqpCodec.GetStringEncodeSize(this.Description);
            valueSize += AmqpCodec.GetMapEncodeSize(this.Info);

            return valueSize;
        }
    }
}