using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using net.r_eg.Conari;
using net.r_eg.Conari.Core;
using net.r_eg.Conari.Native;
using net.r_eg.Conari.Types;

namespace CppLang
{
    public class BasicHeader : IncludeFile
    {
        public override string FileName() => "Basic.h";
        public override List<string> Pragmas() => new List<string>() { "warning(disable: 4267)" };
        public override List<string> Include() => new List<string>() { "<vector>", "<locale>", "<set>" };

        public override void Init()
        {
            // Read File
            var fileStr = ReadThisFile(UftCppLang.IncludePath);

            // Replace Main stuff
            fileStr.Replace("/*!!INCLUDE_PLACEHOLDER!!*/", PrintHelper.GetFileHeader(Pragmas(), Include(), true));
            fileStr.Replace("/*!!FOOTER_PLACEHOLDER!!*/", PrintHelper.GetFileFooter());

            // Replace Major Stuff

        }
    }

    public static class UftCppLang
    {
        public static string UftPath { get; set; }
        public static string SdkPath { get; set; }
        public static string LangPath { get; set; }
        public static string IncludePath { get; set; }
        public static SdkGenInfo GenInfo { get; set; }

        [DllExport]
        public static void Init(CTypes.UftCharPtr uftPath, CTypes.UftCharPtr sdkPath, CTypes.UftCharPtr langPath, bool isExternal)
        {
            UftPath = uftPath.ToString();
            SdkPath = sdkPath.ToString();
            LangPath = langPath.ToString();

            GenInfo = new SdkGenInfo
            {
                IsExternal = isExternal
            };

            IncludePath = LangPath + (GenInfo.IsExternal ? @"\External" : @"\Internal");
            InitIncludes();
        }

        private static void InitIncludes()
        {
            try
            {
                var gg = PrintHelper.GetStruct("UObject");
                MessageBox.Show(gg.Name);
            }
            catch (FileNotFoundException e)
            {
                MessageBox.Show(e.FileName);
            }
        }
    }
}
