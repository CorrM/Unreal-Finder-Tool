using SdkLang;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using net.r_eg.Conari;
using SdkLang.Utils;

namespace SdkLang.Langs
{
    public class BasicHeader : IncludeFile
    {
        public override string FileName() => "Basic.h";
        public List<string> Pragmas() => new List<string>() { "warning(disable: 4267)" };
        public List<string> Include() => new List<string>() { "<vector>", "<locale>", "<set>" };

        public override void Init()
        {
            // Read File
            var fileStr = ReadThisFile(Main.IncludePath);

            // Replace Main stuff
            fileStr.Replace("/*!!INCLUDE_PLACEHOLDER!!*/", PrintHelper.GetFileHeader(Pragmas(), Include(), true));
            fileStr.Replace("/*!!FOOTER_PLACEHOLDER!!*/", PrintHelper.GetFileFooter());

            var jStruct = PrintHelper.GetStruct("FUObjectItem");
            string fUObjectItemStr = string.Empty;

            // Replace Major Stuff
            foreach (var mem in jStruct.Members)
            {
                fUObjectItemStr += mem.Type.All(char.IsDigit)
                    ? $"\tunsigned char {mem.Name} [{mem.Type}];\n"
                    : $"\t{mem.Type} {mem.Name};\n";
            }

            fileStr.Replace("/*!!DEFINE_PLACEHOLDER!!*/", Main.GenInfo.IsGObjectsChunks ? "#define GOBJECTS_CHUNKS" : "");
            fileStr.Replace("/*!!POINTER_SIZE_PLACEHOLDER!!*/", Main.GenInfo.PointerSize.ToString());
            fileStr.Replace("/*!!FUObjectItem_MEMBERS_PLACEHOLDER!!*/", fUObjectItemStr);

            // Write File
            CopyToSdk(Main.SdkPath, fileStr);
        }
    }

    public static class UftCpp
    {
        public static void InitIncludes()
        {
            new BasicHeader().Init();
        }
    }
}
