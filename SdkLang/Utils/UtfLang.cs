using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using net.r_eg.Conari;
using net.r_eg.Conari.Types;

namespace SdkLang.Utils
{
    public abstract class UtfLang
    {
        public abstract void SaveStructs(SdkPackage package);
        public abstract void SaveClasses(SdkPackage package);
        public abstract void SaveFunctions(SdkPackage package);
        public abstract void SaveFunctionParameters(SdkPackage package);
        public abstract void SdkAfterFinish(List<SdkPackage> packages, List<SdkUStruct> missing);

    }
}
