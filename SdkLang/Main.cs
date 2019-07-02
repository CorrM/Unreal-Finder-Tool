using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using net.r_eg.Conari.Types;
using SdkLang.Langs;
using SdkLang.Utils;

namespace SdkLang
{
    public static class Main
    {
        public static string IncludePath { get; set; }
        public static SdkGenInfo GenInfo { get; set; }

        [DllExport]
        public static void SdkLangInit(IntPtr genInfo)
        {
            GenInfo = new SdkGenInfo((Native.GenInfo)new UnmanagedStructure(genInfo, typeof(Native.GenInfo)).Managed);
            IncludePath = GenInfo.LangPath + (GenInfo.IsExternal ? @"\External" : @"\Internal");

            // new UftCpp().Init();
        }
    }
}
