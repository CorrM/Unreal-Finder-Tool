using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using net.r_eg.Conari.Extension;

namespace SdkLang.Utils
{
    public class CTypes
    {
        [DebuggerDisplay("{Str} [ Length: {Length} ]")]
        public struct UftCharPtr
        {
            public readonly IntPtr Ptr;

            public UftCharPtr(IntPtr ptr)
            {
                Ptr = ptr;
            }

            public byte[] Raw => this.Ptr.GetStringBytes(this.Length);

            public int Length => this.Ptr.GetStringLength();
            public string Str => Marshal.PtrToStringAnsi(Ptr);

            public override string ToString()
            {
                return (Ptr == IntPtr.Zero ? base.ToString() : Marshal.PtrToStringAnsi(Ptr)) ?? "";
            }
        }

        [DebuggerDisplay("{ToString()} [ Length: {Length} ]")]
        public struct UftWCharPtr
        {
            public readonly IntPtr Ptr;

            public UftWCharPtr(IntPtr ptr)
            {
                Ptr = ptr;
            }

            public byte[] Raw => this.Ptr.GetStringBytes(this.Length);

            public int Length => this.Ptr.GetStringLength(2U);

            public override string ToString()
            {
                return (Ptr == IntPtr.Zero ? base.ToString() : Marshal.PtrToStringUni(Ptr)) ?? "null";
            }
        }

        public struct UftArrayPtr
        {
            public readonly IntPtr Ptr;
            private readonly IntPtr[] _data;
            private readonly int _itemSize;

            public UftArrayPtr(IntPtr arrayPtr, int itemCount, int itemSize)
            {
                Ptr = arrayPtr;
                Count = itemCount;
                _itemSize = itemSize;
                _data = new IntPtr[Count];

                for (int i = 0; i < Count; i++)
                {
                    int offset = i * itemSize;
                    _data[i] = new IntPtr(arrayPtr.ToInt64() + offset);
                }
            }

            public IntPtr this[int index] => GetItemPtr(index);

            public IntPtr GetItemPtr(int index)
            {
                return _data[index];
            }

            public int Count { get; }
        }

    }
}
