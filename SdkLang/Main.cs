using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using SdkLang.Langs;
using SdkLang.Utils;

namespace SdkLang
{
    public static class Main
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
            UftCpp.InitIncludes();
        }
    }
}
