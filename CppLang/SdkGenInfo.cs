using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace CppLang
{
    public struct JsonVar
    {
        public string Name, Type;
        public int Size, Offset;
    }
    public struct JsonStruct
    {
        public string Name, Super;
        public int Size;
        public List<JsonVar> Members;
    }
    public struct SdkGenInfo
    {
        public string GameName, GameVersion, NamespaceName;
        public int MemberAlignment;
        public bool IsExternal;
    }
}
