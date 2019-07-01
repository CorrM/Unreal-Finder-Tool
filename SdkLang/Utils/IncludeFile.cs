using System;
using System.Collections.Generic;
using System.Text;
using System.IO;

namespace SdkLang.Utils
{
    public abstract class IncludeFile
    {
        public abstract string FileName();
        public abstract void Init(UtfLang targetLang);


        public StringBuilder ReadThisFile(string includePath)
        {
            return new StringBuilder(File.ReadAllText($@"{includePath}\{FileName()}"));
        }
        public void CopyToSdk(string sdkPath, StringBuilder fileStr)
        {
            File.WriteAllText($@"{sdkPath}\{FileName()}", fileStr.ToString());
        }
        public void CopyToSdk(string sdkPath, string fileStr)
        {
            File.WriteAllText($@"{sdkPath}\{FileName()}", fileStr);
        }
        public void AppendToSdk(string sdkPath, string text)
        {
            File.AppendAllText($@"{sdkPath}\{FileName()}", text);
        }
        public static void AppendToSdk(string sdkPath, string fileName, string text)
        {
            File.AppendAllText($@"{sdkPath}\{fileName}", text);
        }
    }
}
