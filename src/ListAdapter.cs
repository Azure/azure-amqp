// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Collections;
    using System.Collections.Generic;

    sealed class ListAdapter<TFrom, TTo> : IList<TTo>
    {
        readonly Func<TFrom, TTo> converter;
        IList<TFrom> source;

        public ListAdapter(Func<TFrom, TTo> converter)
        {
            this.converter = converter ?? throw new ArgumentNullException(nameof(converter));
        }

        IList<TFrom> Source =>
            this.source ?? throw new InvalidOperationException("The adapter is not attached.");

        public int Count => this.Source.Count;

        public bool IsReadOnly => true;

        public TTo this[int index]
        {
            get => this.converter(this.Source[index]);
            set => throw new NotSupportedException();
        }

        public void Attach(IList<TFrom> source)
        {
            if (source == null)
            {
                throw new ArgumentNullException(nameof(source));
            }

            if (this.source != null)
            {
                throw new InvalidOperationException("The adapter is already attached.");
            }

            this.source = source;
        }

        public void Detach()
        {
            this.source = null;
        }

        public bool Contains(TTo item) => this.IndexOf(item) >= 0;

        public int IndexOf(TTo item)
        {
            IList<TFrom> source = this.Source;
            for (int i = 0; i < source.Count; i++)
            {
                if (EqualityComparer<TTo>.Default.Equals(item, this.converter(source[i])))
                {
                    return i;
                }
            }

            return -1;
        }

        public void CopyTo(TTo[] array, int arrayIndex)
        {
            IList<TFrom> source = this.Source;
            int count = source.Count;
            if (array == null)
            {
                throw new ArgumentNullException(nameof(array));
            }

            if (arrayIndex < 0 || arrayIndex > array.Length - count)
            {
                throw new ArgumentOutOfRangeException(nameof(arrayIndex));
            }

            for (int i = 0; i < count; i++)
            {
                array[arrayIndex + i] = this.converter(source[i]);
            }
        }

        public IEnumerator<TTo> GetEnumerator() => new Enumerator(this.Source, this.converter);

        public void Add(TTo item) => throw new NotSupportedException();
        public void Clear() => throw new NotSupportedException();
        public void Insert(int index, TTo item) => throw new NotSupportedException();
        public bool Remove(TTo item) => throw new NotSupportedException();
        public void RemoveAt(int index) => throw new NotSupportedException();

        IEnumerator IEnumerable.GetEnumerator() => this.GetEnumerator();

        struct Enumerator : IEnumerator<TTo>
        {
            readonly Func<TFrom, TTo> converter;
            readonly IEnumerator<TFrom> enumerator;

            public Enumerator(IList<TFrom> source, Func<TFrom, TTo> converter)
            {
                this.converter = converter;
                this.enumerator = source.GetEnumerator();
            }

            public TTo Current => this.converter(this.enumerator.Current);
            object IEnumerator.Current => this.Current;
            public bool MoveNext() => this.enumerator.MoveNext();
            public void Reset() => this.enumerator.Reset();
            public void Dispose() => this.enumerator.Dispose();
        }
    }
}
