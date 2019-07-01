using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using net.r_eg.Conari.Extension;
using net.r_eg.Conari.Types;

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

            public int Count { get; }

            public UftArrayPtr(IntPtr arrayPtr, int itemCount, int itemSize)
            {
                Ptr = arrayPtr;
                Count = itemCount;
                _data = new IntPtr[Count];

                for (int i = 0; i < Count; i++)
                {
                    int offset = i * itemSize;
                    _data[i] = new IntPtr((IntPtr.Size == 8 ? arrayPtr.ToInt64() : arrayPtr.ToInt32()) + offset);
                }
            }
            public IntPtr this[int index] => GetItemPtr(index);
            public IntPtr GetItemPtr(int index)
            {
                return _data[index];
            }
            public List<TNativeType> ToStructList<TNativeType>()
            {
                return _data.Select(item => (TNativeType)new UnmanagedStructure(item, typeof(TNativeType)).Managed).ToList();
            }
        }
        public struct UftStringArrayPtr
        {
            public readonly IntPtr Ptr;
            private readonly string[] _data;

            public int Count { get; }

            public UftStringArrayPtr(IntPtr arrayPtr, int itemCount, bool uniCode = false)
            {
                Ptr = arrayPtr;
                Count = itemCount;
                _data = new string[Count];

                IntPtr curPtr = arrayPtr;
                for (int i = 0; i < Count; i++)
                {
                    _data[i] = Marshal.PtrToStringAnsi(curPtr);
                    curPtr += _data[i].Length; // + 1
                }
            }
            public UftStringArrayPtr(Native.StringArray strArray, bool uniCode = false) 
                : this(strArray.Ptr, strArray.Count, uniCode)
            {
            }

            public string this[int index] => GetItem(index);
            public string GetItem(int index)
            {
                return _data[index];
            }
            public List<string> ToList()
            {
                return new List<string>(_data);
            }
        }
    }
}
