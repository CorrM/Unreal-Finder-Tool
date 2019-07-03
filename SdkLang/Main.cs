using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using net.r_eg.Conari.Types;
using SdkLang.Langs;
using SdkLang.Utils;

namespace SdkLang
{
    public static class Main
    {
        public static Dictionary<string, UftLang> SupportedLangs = new Dictionary<string, UftLang>()
        {
            { "Cpp", new UftCpp() }
        };

        public static string IncludePath { get; private set; }
        public static UftLang Lang { get; set; }
        public static SdkGenInfo GenInfo { get; set; }

        private static SdkPackage GetPackageFromPtr(IntPtr nPackage)
        {
            var us = new UnmanagedStructure(nPackage, typeof(Native.Package));
            return new SdkPackage((Native.Package)us.Managed);
        }

        [DllExport]
        public static bool UftLangInit(IntPtr genInfo)
        {
            GenInfo = new SdkGenInfo((Native.GenInfo)new UnmanagedStructure(genInfo, typeof(Native.GenInfo)).Managed);
            IncludePath = GenInfo.LangPath + (GenInfo.IsExternal ? @"\External" : @"\Internal");

            // Check if this lang is supported
            if (!SupportedLangs.ContainsKey(GenInfo.SdkLang))
                return false;

            Lang = SupportedLangs[GenInfo.SdkLang];
            Lang.Init();

            return true;
        }
        [DllExport]
        public static void UftLangSaveStructs(IntPtr nPackage)
        {
            Lang.SaveStructs(GetPackageFromPtr(nPackage));
        }
        [DllExport]
        public static void UftLangSaveClasses(IntPtr nPackage)
        {
            Lang.SaveClasses(GetPackageFromPtr(nPackage));
        }
        [DllExport]
        public static void UftLangSaveFunctions(IntPtr nPackage)
        {
            Lang.SaveFunctions(GetPackageFromPtr(nPackage));
        }
        [DllExport]
        public static void UftLangSaveFunctionParameters(IntPtr nPackage)
        {
            Lang.SaveFunctionParameters(GetPackageFromPtr(nPackage));
        }
    }
}
