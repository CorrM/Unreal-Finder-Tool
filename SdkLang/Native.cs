using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using SdkLang.Utils;

namespace SdkLang
{
    public class Native
    {
        [StructLayout(LayoutKind.Sequential)]
        public struct JsonVar
        {
            public CTypes.UftCharPtr Name;
            public CTypes.UftCharPtr Type;
            public int Size, Offset;
        };

        [StructLayout(LayoutKind.Sequential)]
        public struct JsonStruct
        {
            public CTypes.UftCharPtr StructName;
            public CTypes.UftCharPtr StructSuper;
            public IntPtr Members; // NativeJsonVar*
            public int MemberSize;
            public int MembersCount;
        };

    }
}
