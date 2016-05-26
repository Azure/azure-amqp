// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#if WINDOWS_UWP
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using Windows.Networking;
using Windows.Networking.Sockets;
#endif

#if DNXCORE

// This interface doesn't exist in DNXCORE50, define it manually
namespace System
{
    public interface ICloneable
    {
        object Clone();
    }
}

namespace System.Collections.Generic
{
    using System.Collections.ObjectModel;
    using Microsoft.Azure.Amqp;

    /// <summary>Provides a collection whose items are types that serve as keys.</summary>
    /// <typeparam name="TItem">The item types contained in the collection that also serve as the keys for the collection.</typeparam>
    public class KeyedByTypeCollection<TItem> : KeyedCollection<Type, TItem>
    {
        /// <summary>Initializes a new instance of the <see cref="T:System.Collections.Generic.KeyedByTypeCollection`1" /> class.  </summary>
        public KeyedByTypeCollection() : base(null, 4)
        {
        }

        /// <summary>Initializes a new instance of the <see cref="T:System.Collections.Generic.KeyedByTypeCollection`1" /> class for a specified enumeration of objects.</summary>
        /// <param name="items">The <see cref="T:System.Collections.Generic.IEnumerable`1" /> of generic type <see cref="T:System.Object" /> used to initialize the collection.</param>
        /// <exception cref="T:System.ArgumentNullException">
        ///   <paramref name="items" /> is null.</exception>
        public KeyedByTypeCollection(IEnumerable<TItem> items) : base(null, 4)
        {
            if (items == null)
            {
                throw Fx.Exception.ArgumentNull("items");
            }
            foreach (TItem current in items)
            {
                base.Add(current);
            }
        }

        /// <summary>Returns the first item in the collection of a specified type.</summary>
        /// <returns>The object of type <paramref name="T" /> if it is a reference type and the value of type <paramref name="T" /> if it is a value type. The default value of the type is returned if no object of type <paramref name="T" /> is contained in the collection: null if it is a reference type and 0 if it is a value type.</returns>
        /// <typeparam name="T">The type of item in the collection to find.</typeparam>
        public T Find<T>()
        {
            return this.Find<T>(false);
        }

        /// <summary>Removes an object of a specified type from the collection.</summary>
        /// <returns>The object removed from the collection.</returns>
        /// <typeparam name="T">The type of item in the collection to remove.</typeparam>
        public T Remove<T>()
        {
            return this.Find<T>(true);
        }

        T Find<T>(bool remove)
        {
            for (int i = 0; i < base.Count; i++)
            {
                TItem tItem = base[i];
                if (tItem is T)
                {
                    if (remove)
                    {
                        base.Remove(tItem);
                    }
                    return (T)((object)tItem);
                }
            }
            return default(T);
        }

        /// <summary>Returns a collection of objects of type <paramref name="T" /> that are contained in the <see cref="T:System.Collections.Generic.KeyedByTypeCollection`1" />.</summary>
        /// <returns>A <see cref="T:System.Collections.ObjectModel.Collection`1" /> of type <paramref name="T" /> that contains the objects of type <paramref name="T" /> from the original collection.</returns>
        /// <typeparam name="T">The type of item in the collection to find.</typeparam>
        public Collection<T> FindAll<T>()
        {
            return this.FindAll<T>(false);
        }

        /// <summary>Removes all of the elements of a specified type from the collection.</summary>
        /// <returns>The <see cref="T:System.Collections.ObjectModel.Collection`1" /> that contains the objects of type <paramref name="T" /> from the original collection.</returns>
        /// <typeparam name="T">The type of item in the collection to remove.</typeparam>
        public Collection<T> RemoveAll<T>()
        {
            return this.FindAll<T>(true);
        }

        Collection<T> FindAll<T>(bool remove)
        {
            Collection<T> collection = new Collection<T>();
            foreach (TItem current in this)
            {
                if (current is T)
                {
                    collection.Add((T)((object)current));
                }
            }
            if (remove)
            {
                foreach (T current2 in collection)
                {
                    base.Remove((TItem)((object)current2));
                }
            }
            return collection;
        }

        /// <summary>Gets the type of an item contained in the collection.</summary>
        /// <returns>The type of the specified <paramref name="item" /> in the collection.</returns>
        /// <param name="item">The item in the collection whose type is to be retrieved.</param>
        /// <exception cref="T:System.ArgumentNullException">
        ///   <paramref name="item" /> is null.</exception>
        protected override Type GetKeyForItem(TItem item)
        {
            if (item == null)
            {
                throw Fx.Exception.ArgumentNull("item");
            }
            return item.GetType();
        }

        /// <summary>Inserts an element into the collection at a specific location.</summary>
        /// <param name="index">The zero-based index at which <paramref name="item" /> should be inserted. </param>
        /// <param name="item">The object to insert into the collection.</param>
        /// <exception cref="T:System.ArgumentNullException">
        ///   <paramref name="item" /> is null.</exception>
        protected override void InsertItem(int index, TItem item)
        {
            if (item == null)
            {
                throw Fx.Exception.ArgumentNull("item");
            }
            if (base.Contains(item.GetType()))
            {
                string message = string.Format(
                    "The value could not be added to the collection, as the collection already contains an item of the same type: '{0}'. This collection only supports one instance of each type.",
                    item.GetType().FullName);
                throw Fx.Exception.Argument("item", message);
            }
            base.InsertItem(index, item);
        }

        /// <summary>Replaces the item at the specified index with a new object.</summary>
        /// <param name="index">The zero-based index of the <paramref name="item" /> to be replaced.</param>
        /// <param name="item">The object to add to the collection.</param>
        /// <exception cref="T:System.ArgumentNullException">
        ///   <paramref name="item" /> is null.</exception>
        protected override void SetItem(int index, TItem item)
        {
            if (item == null)
            {
                throw Fx.Exception.ArgumentNull("item");
            }
            base.SetItem(index, item);
        }
    }
}
#if WINDOWS_UWP

namespace System.Threading
{
    //
    // Summary:
    //     Represents a callback method to be executed by a thread pool thread.
    //
    // Parameters:
    //   state:
    //     An object containing information to be used by the callback method.
    public delegate void WaitCallback(object state);
}

#endif


#if WINDOWS_UWP

class Win32
{
    [DllImport("kernel32.dll")]
    public static extern int GetCurrentProcessId();
}

#endif

#endif // DNXCORE

namespace Diagnostics
{
    static class CurrentProcess
    {
        public static int ID
        {
            get
            {
#if WINDOWS_UWP
                return Win32.GetCurrentProcessId();
#else
                return System.Diagnostics.Process.GetCurrentProcess().Id;
#endif
            }
        }
    }
}

