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
        public static UftLang Lang { get; set; }
        public static SdkGenInfo GenInfo { get; set; }

        [DllExport]
        public static void UftLangInit(IntPtr genInfo)
        {
            // MessageBox.Show(Marshal.SizeOf(typeof(Native.GenInfo)).ToString());
            GenInfo = new SdkGenInfo((Native.GenInfo)new UnmanagedStructure(genInfo, typeof(Native.GenInfo)).Managed);
            IncludePath = GenInfo.LangPath + (GenInfo.IsExternal ? @"\External" : @"\Internal");

            if (GenInfo.SdkLang == "Cpp")
                Lang = new UftCpp();

            Lang.Init();
        }

        [DllExport]
        public static void UftLangSaveStructs(IntPtr nPackage)
        {
            try
            {
                var us = new UnmanagedStructure(nPackage, typeof(Native.Package));
                SdkPackage package = new SdkPackage((Native.Package)us.Managed);
                Lang.SaveStructs(package);
            }
            catch (Exception e)
            {
                Console.WriteLine(e);
                throw;
            }
        }
    }
}
