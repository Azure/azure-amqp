namespace Test.Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;
    using System.Runtime.Serialization;
    using global::Microsoft.Azure.Amqp;
    using global::Microsoft.Azure.Amqp.Encoding;
    using global::Microsoft.Azure.Amqp.Serialization;

    [AmqpContract(Name = "person", Code = 0)]
    [KnownType(typeof(Student))]
    [KnownType(typeof(Teacher))]
    class Person
    {
        public Person() { }

        public Person(string name)
        {
            this.Name = name;
        }

        [AmqpMember(Order = 1)]
        public string Name
        {
            get;
            private set;
        }

        [AmqpMember(Order = 2)]
        public int Age
        {
            get;
            set;
        }

        [AmqpMember(Order = 3)]
        public DateTime? DateOfBirth;

        public IDictionary<string, object> Properties
        {
            get
            {
                if (this.properties == null)
                {
                    this.properties = new Dictionary<string, object>();
                }

                return this.properties;
            }
        }

        [AmqpMember(Order = 8)]
        Dictionary<string, object> properties;

        [System.Runtime.Serialization.OnDeserialized]
        void OnDesrialized()
        {
            this.Age = this.Age + 1;
        }
    }

    [AmqpContract(Name = "student", Code = 1)]
    class Student : Person
    {
        public Student() : base(null) { }

        public Student(string name)
            : base(name)
        {
        }

        [AmqpMember(Name = "address", Order = 4)]
        public Address Address;

        [AmqpMember(Name = "grades", Order = 10)]
        public List<int> Grades { get; set; }
    }

    [AmqpContract(Name = "teacher", Code = 2)]
    class Teacher : Person
    {
        public Teacher() { }

        public Teacher(string name)
            : base(name)
        {
            this.Id = EmployeeId.New();
        }

        [AmqpMember(Name = "sallary", Order = 4)]
        public int Sallary;

        [AmqpMember(Order = 10)]
        public EmployeeId Id
        {
            get;
            private set;
        }

        [AmqpMember(Order = 11)]
        public Dictionary<int, string> Classes
        {
            get;
            set;
        }

        [System.Runtime.Serialization.OnDeserialized]
        void OnDesrialized()
        {
            this.Sallary *= 2;
        }
    }

    [AmqpContract(Name = "address", Code = 3)]
    class Address
    {
        [AmqpMember]
        public string FullAddress;
    }

    class EmployeeId : IAmqpSerializable
    {
        Guid uuid;

        EmployeeId(Guid uuid)
        {
            this.uuid = uuid;
        }

        public EmployeeId() { }

        public int EncodeSize
        {
            get { return FixedWidth.UuidEncoded; }
        }

        public void Encode(ByteBuffer buffer)
        {
            UuidEncoding.Encode(this.uuid, buffer);
        }

        public void Decode(ByteBuffer buffer)
        {
            this.uuid = UuidEncoding.Decode(buffer, 0).Value;
        }

        public static EmployeeId New()
        {
            return new EmployeeId(Guid.NewGuid());
        }

        public override bool Equals(object obj)
        {
            return obj is EmployeeId && ((EmployeeId)obj).uuid == this.uuid;
        }

        public override int GetHashCode()
        {
            return this.uuid.GetHashCode();
        }
    }
}
