// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using System;
    using System.Runtime.Serialization;
    using System.Text;
    using Microsoft.Azure.Amqp.Encoding;

    /// <summary>
    /// Defines the AMQP error.
    /// </summary>
    [Serializable]
    public sealed class Error : DescribedList, ISerializable
    {
        /// <summary>Descriptor name.</summary>
        public const string Name = "amqp:error:list";
        /// <summary>Descriptor code.</summary>
        public const ulong Code = 0x000000000000001d;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public Error() : base(Name, Code, 3)
        {
        }

        Error(SerializationInfo info, StreamingContext context)
            : this()
        {
            this.Condition = (string)info.GetValue("Condition", typeof(string));
            this.Description = (string)info.GetValue("Description", typeof(string));
        }

        /// <summary>
        /// Gets or sets the "condition" field.
        /// </summary>
        public AmqpSymbol Condition { get; set; }

        /// <summary>
        /// Gets or sets the "description" field.
        /// </summary>
        public string Description { get; set; }

        /// <summary>
        /// Gets or sets the "info" field.
        /// </summary>
        public Fields Info { get; set; }

        internal static Error FromException(Exception exception, bool includeErrorDetails = false)
        {
            AmqpException amqpException = exception as AmqpException;
            if (amqpException != null)
            {
                return amqpException.Error;
            }

            Error error = new Error();
            error.Description = exception.Message;
            if (exception is UnauthorizedAccessException)
            {
                error.Condition = AmqpErrorCode.UnauthorizedAccess;
            }
            else if (exception is InvalidOperationException)
            {
                error.Condition = AmqpErrorCode.NotAllowed;
            }
            else if (exception is System.Transactions.TransactionAbortedException)
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
                error.Description = includeErrorDetails ?
                    exception.Message :
                    AmqpResources.GetString(AmqpResources.AmqpErrorOccurred, AmqpErrorCode.InternalError);
            }

#if DEBUG
            error.Info = new Fields();

            const int MaxSizeInInfoMap = 8 * 1024;
            // Limit the size of the exception string as it may exceed the connection max frame size
            string exceptionString = exception.ToString();
            if (exceptionString.Length > MaxSizeInInfoMap)
            {
                exceptionString = exceptionString.Substring(0, MaxSizeInInfoMap);
            }

            error.Info.Add("exception", exceptionString);
#endif

            return error;
        }

        /// <summary>
        /// Returns a string that represents the object.
        /// </summary>
        /// <returns>The string representation.</returns>
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

        internal override void EnsureRequired()
        {
            if (this.Condition.Value == null)
            {
                throw AmqpEncoding.GetEncodingException(AmqpResources.GetString(AmqpResources.AmqpRequiredFieldNotSet, "condition", Name));
            }
        }

        /// <summary>
        /// Encodes the fields into the buffer.
        /// </summary>
        /// <param name="buffer">The buffer to write.</param>
        protected override void OnEncode(ByteBuffer buffer)
        {
            AmqpCodec.EncodeSymbol(this.Condition, buffer);
            AmqpCodec.EncodeString(this.Description, buffer);
            AmqpCodec.EncodeMap(this.Info, buffer);
        }

        /// <summary>
        /// Decodes the fields from the buffer.
        /// </summary>
        /// <param name="buffer">The buffer.</param>
        /// <param name="count">The number of fields.</param>
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

        /// <summary>
        /// Returns the total encode size of all fields.
        /// </summary>
        /// <returns>The total encode size.</returns>
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