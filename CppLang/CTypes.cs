using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using net.r_eg.Conari.Extension;

namespace CppLang
{
    public class CTypes
    {
        [DebuggerDisplay("{(string)this} [ {\"0x\" + ptr.ToString(\"X\")} Length: {Length} ]")]
        public struct UftCharPtr
        {
            private readonly IntPtr _ptr;

            public UftCharPtr(IntPtr ptr)
            {
                _ptr = ptr;
            }

            public byte[] Raw => this._ptr.GetStringBytes(this.Length);

            public int Length => this._ptr.GetStringLength(2U);

            public override string ToString()
            {
                return (_ptr == IntPtr.Zero ? base.ToString() : Marshal.PtrToStringAnsi(_ptr)) ?? "";
            }

            public static implicit operator string(UftCharPtr val)
            {
                return val._ptr == IntPtr.Zero ? null : Marshal.PtrToStringAnsi(val._ptr);
            }
        }
    }
}
