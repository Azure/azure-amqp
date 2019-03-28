// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Framing
{
    using System.Text;

    /// <summary>
    /// Defines the end performative.
    /// </summary>
    public sealed class End : Performative
    {
        /// <summary>Descriptor name.</summary>
        public static readonly string Name = "amqp:end:list";
        /// <summary>Descriptor code.</summary>
        public static readonly ulong Code = 0x0000000000000017;
        const int Fields = 1;

        /// <summary>
        /// Initializes the object.
        /// </summary>
        public End() : base(Name, Code)
        {
        }

        /// <summary>
        /// Gets or sets the "error" field.
        /// </summary>
        public Error Error { get; set; }

        internal override int FieldCount
        {
            get { return Fields; }
        }

        /// <summary>
        /// Returns a string that represents the object.
        /// </summary>
        /// <returns>The string representation.</returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder("end(");
            int count = 0;
            this.AddFieldToString(this.Error != null, sb, "error", this.Error, ref count);
            sb.Append(')');
            return sb.ToString();
        }

        internal override void OnEncode(ByteBuffer buffer)
        {
            AmqpCodec.EncodeSerializable(this.Error, buffer);
        }

        internal override void OnDecode(ByteBuffer buffer, int count)
        {
            if (count-- > 0)
            {
                this.Error = AmqpCodec.DecodeKnownType<Error>(buffer);
            }
        }

        internal override int OnValueSize()
        {
            int valueSize = 0;

            valueSize += AmqpCodec.GetSerializableEncodeSize(this.Error);

            return valueSize;
        }
    }
}
