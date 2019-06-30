using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using net.r_eg.Conari;
using net.r_eg.Conari.Core;
using net.r_eg.Conari.Native;

namespace CppLang
{
    public static class PrintHelper
    {
        public static string GetFileHeader(List<string> pragmas, List<string> includes, bool isHeaderFile)
        {
            var genInfo = UftCppLang.GenInfo;
            var sb = new StringBuilder();

            // Pragmas
            if (isHeaderFile)
            {
                sb.Append("#pragma once\n");
                if (pragmas.Count > 0)
                    foreach(string i in pragmas) { sb.Append("#pragma " + i + "\n"); }
                sb.Append("\n");
            }

            if (genInfo.IsExternal)
                sb.Append("#include \"../Memory.h\"\n");

            // Includes
            if (includes.Count > 0)
                foreach (string i in includes) { sb.Append("#include " + i + "\n"); }
            sb.Append("\n");

            // 
            sb.Append($"// Name: {genInfo.GameName}, Version: {genInfo.GameVersion}\n\n");
            sb.Append($"#ifdef _MSC_VER\n\t#pragma pack(push, 0x{genInfo.MemberAlignment:X2})\n#endif\n\n");
            sb.Append($"namespace {genInfo.NamespaceName}\n{{\n");

            return sb.ToString();
        }

        public static string GetFileFooter()
        {
            return "}\n\n#ifdef _MSC_VER\n\t#pragma pack(pop)\n#endif\n";
        }

        public static JsonStruct GetStruct(string structName)
        {
            MessageBox.Show(typeof(ConariL).FullName);
            //using (var l = new ConariL("dfgfghgfhghffghgfhfghd"))
            //{
            //    var d = l.DLR;
            //    IntPtr structPtr = d.GetStructPtr<IntPtr>(structName);
            //    l.BeforeUnload += (sender, e) => { d.FreeStructA(); };

            //    CTypes.UftCharPtr name = structPtr.Native()
            //        .t<CTypes.UftCharPtr>("StructName")
            //        .Raw.Type.DLR.StructName;
            //    return new JsonStruct() { Name = name };
            //}
            return new JsonStruct() { Name = "gfdg" };
        }
    }
}
