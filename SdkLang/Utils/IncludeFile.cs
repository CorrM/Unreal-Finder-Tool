using System;
using System.Collections.Generic;
using System.Text;
using System.IO;

namespace SdkLang.Utils
{
    public abstract class IncludeFile
    {
        public abstract string FileName();
        public abstract void Init();

        public StringBuilder ReadThisFile(string includePath)
        {
            return new StringBuilder(File.ReadAllText($@"{includePath}\{FileName()}"));
        }
        public void CopyToSdk(string sdkPath, StringBuilder fileStr)
        {
            File.WriteAllText($@"{sdkPath}\{FileName()}", fileStr.ToString());
        }
        private void AddToSdkHeader()
        {

        }
    }
}
