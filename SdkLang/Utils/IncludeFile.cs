using System;
using System.Collections.Generic;
using System.Text;
using System.IO;

namespace SdkLang.Utils
{
    public abstract class IncludeFile<TLang> where TLang : UftLang
    {
        public TLang TargetLang { get; }
        public string SdkPath { get; }

        public abstract string FileName { get; set; }

        protected IncludeFile() : this((TLang)Main.Lang, Main.GenInfo.SdkPath)
        {
        }

        protected IncludeFile(TLang targetLang, string sdkPath)
        {
            TargetLang = targetLang;
            SdkPath = sdkPath;
        }
        public abstract void Process(string includePath);

        public void CreateFile()
        {
            File.CreateText($@"{SdkPath}\{FileName}").Close();
        }
        public static void CreateFile(string sdkPah, string fileName)
        {
            File.CreateText($@"{sdkPah}\{fileName}").Close();
        }
        public UftStringBuilder ReadThisFile(string includePath)
        {
            return new UftStringBuilder(File.ReadAllText($@"{includePath}\{FileName}"));
        }
        public void CopyToSdk(UftStringBuilder fileStr)
        {
            File.WriteAllText($@"{SdkPath}\{FileName}", fileStr.ToString());
        }
        public void CopyToSdk(string fileStr)
        {
            File.WriteAllText($@"{SdkPath}\{FileName}", fileStr);
        }
        public void AppendToSdk(string text)
        {
            File.AppendAllText($@"{SdkPath}\{FileName}", text);
        }
        public static void AppendToSdk(string sdkPath, string fileName, string text)
        {
            File.AppendAllText($@"{sdkPath}\{fileName}", text);
        }
        public static void WriteToSdk(string sdkPath, string fileName, string text)
        {
            File.WriteAllText($@"{sdkPath}\{fileName}", text);
        }
    }
}
