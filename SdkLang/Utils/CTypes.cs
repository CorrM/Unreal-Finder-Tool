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
        public class UftCharPtr
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
        public class UftWCharPtr
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
        public class UftArrayPtr
        {
            public IntPtr Ptr;
            private readonly IntPtr[] _data;

            public bool IsStructArray { get; private set; }
            public int ItemSize { get; }
            public int Count { get; }

            public UftArrayPtr(IntPtr arrayPtr, IntPtr itemCount, IntPtr itemSize) 
                : this(arrayPtr, itemCount.ToInt32(), itemSize.ToInt32())
            {

            }
            public UftArrayPtr(IntPtr arrayPtr, int itemCount, int itemSize)
            {
                Ptr = arrayPtr;
                Count = itemCount;
                ItemSize = itemSize;
                _data = new IntPtr[Count];

                Init(arrayPtr, false);
            }

            private void Init(IntPtr arrayPtr, bool isStructArray)
            {
                Ptr = arrayPtr;
                IsStructArray = isStructArray;
                for (int i = 0; i < Count; i++)
                {
                    if (isStructArray)
                    {
                        int offset = i * IntPtr.Size;
                        _data[i] = Marshal.ReadIntPtr(Ptr + offset);
                    }
                    else
                    {
                        int offset = i * ItemSize;
                        _data[i] = new IntPtr((IntPtr.Size == 8 ? arrayPtr.ToInt64() : arrayPtr.ToInt32()) + offset);
                    }
                }
            }
            public IntPtr this[int index] => GetItemPtr(index);
            public IntPtr GetItemPtr(int index)
            {
                return _data[index];
            }
            public List<TNativeType> ToPtrStructList<TNativeType>()
            {
                if (Ptr == IntPtr.Zero)
                    return new List<TNativeType>();

                Init(Ptr, true);

                return _data
                    .Where(ptr => ptr != IntPtr.Zero)
                    .Select(ptr => (TNativeType)new UnmanagedStructure(ptr, typeof(TNativeType)).Managed)
                    .ToList();
            }
        }
        public class UftStringArrayPtr
        {
            public readonly IntPtr Ptr;
            private readonly string[] _data;

            public int Count { get; }

            public UftStringArrayPtr(IntPtr arrayPtr, int itemCount, bool uniCode = false)
            {
                Ptr = arrayPtr;
                Count = itemCount;
                _data = new string[Count];

                for (int i = 0; i < Count; i++)
                {
                    int offset = i * IntPtr.Size;
                    if (uniCode)
                        _data[i] = Marshal.PtrToStringUni(Marshal.ReadIntPtr(Ptr + offset));
                    else
                        _data[i] = Marshal.PtrToStringAnsi(Marshal.ReadIntPtr(Ptr + offset));
                }
            }
            public UftStringArrayPtr(Native.StringArray strArray, bool uniCode = false) 
                : this(strArray.Ptr, strArray.Count.ToInt32(), uniCode)
            {
            }

            public UftStringArrayPtr(IntPtr arrayPtr, IntPtr itemCount, bool uniCode = false)
                : this(arrayPtr, itemCount.ToInt32(), uniCode)
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
