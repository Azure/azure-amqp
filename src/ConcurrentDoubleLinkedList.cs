// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#nullable enable

namespace Microsoft.Azure.Amqp;

using System.Collections;
using System.Collections.Generic;
using System.Threading;

sealed class ConcurrentDoubleLinkedList<T> : IEnumerable<T>
{
    private int _count; // this counter is not meant to be accurate
    readonly Node head;   // sentinel
    readonly Node tail;   // sentinel
    readonly object removeLock = new();

    public ConcurrentDoubleLinkedList()
    {
        head = new Node(default!);
        tail = new Node(default!);
        head.next = tail;
        tail.prev = head;
    }

    public void AddFirst(T value)
    {
        var newNode = new Node(value);
        while (true)
        {
            // head.next is never actually null (always at least tail)
            var next = Volatile.Read(ref head.next)!;

            newNode.prev = head;
            newNode.next = next;

            // try to splice newNode in between head and next:
            if (Interlocked.CompareExchange(ref head.next, newNode, next) == next)
            {
                // now link back:
                // we know next.prev is still head (no one else removed it),
                // but even if another thread raced here, it's safe to overwrite it
                Volatile.Write(ref next.prev, newNode);
                Interlocked.Increment(ref _count);
                return;
            }
            // otherwise someone else inserted; retry
        }
    }

    public void AddLast(T value)
    {
        var newNode = new Node(value);
        while (true)
        {
            // tail.prev is never actually null (always at least head)
            var prev = Volatile.Read(ref tail.prev)!;

            newNode.prev = prev;
            newNode.next = tail;

            if (Interlocked.CompareExchange(ref prev.next, newNode, tail) == tail)
            {
                Volatile.Write(ref tail.prev, newNode);
                Interlocked.Increment(ref _count);
                return;
            }
            // contention: another insert/unlink in flight; retry
        }
    }

    public bool Remove(T value)
    {
        var comparer = EqualityComparer<T>.Default;

        var cur = Volatile.Read(ref head.next);
        while (cur is not null && cur != tail)
        {
            if (!cur.IsRemoved && comparer.Equals(cur.Value, value))
            {
                return RemoveNode(cur);
            }

            cur = Volatile.Read(ref cur.next);
        }

        return false;
    }

    private bool RemoveNode(Node node)
    {
        // never remove the sentinels
        if (ReferenceEquals(node, head) || ReferenceEquals(node, tail))
        {
            return false;
        }

        // phase 1: mark as removed
        if (!node.TryMarkRemoved())
        {
            return false;
        }

        // phase 2: unlink under brief lock
        lock (removeLock)
        {
            var p = node.prev!;
            var n = node.next!;

            Volatile.Write(ref p.next, n);
            Volatile.Write(ref n.prev, p);

            Volatile.Write(ref node.prev, null);
            Volatile.Write(ref node.next, null);
        }

        Interlocked.Decrement(ref _count);
        return true;
    }

    private List<Node> SnapshotNodesForEnumeration()
    {
        int approxCount = Volatile.Read(ref _count);
        var snapshot = new List<Node>(approxCount);

        var cur = Volatile.Read(ref head.next);
        while (cur is not null && cur != tail)
        {
            if (!cur.IsRemoved)
            {
                snapshot.Add(cur);
            }

            cur = Volatile.Read(ref cur.next);
        }

        return snapshot;
    }

    public IEnumerator<T> GetEnumerator()
    {
        foreach (var node in SnapshotNodesForEnumeration())
        {
            yield return node.Value;
        }
    }

    IEnumerator IEnumerable.GetEnumerator() => GetEnumerator();

    sealed class Node
    {
        internal Node? prev, next;
        int removed;
        public T Value { get; }

        internal Node(T value)
        {
            Value = value;
        }

        internal bool IsRemoved => Volatile.Read(ref removed) != 0;
        internal bool TryMarkRemoved() => Interlocked.Exchange(ref removed, 1) == 0;
    }
}