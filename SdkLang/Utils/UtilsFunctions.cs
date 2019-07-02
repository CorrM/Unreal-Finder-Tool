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
    public static class UtilsFunctions
    {
        public static JsonStruct GetStruct(string structName)
        {
            using (var l = new ConariL(Main.GenInfo.UftPath))
            {
                var d = l.DLR;
                IntPtr structPtr = d.GetStructPtr<IntPtr>(new UnmanagedString(structName, UnmanagedString.SType.Ansi).Pointer);
                l.BeforeUnload += (sender, e) => { d.FreeStructPtr(structPtr); };

                var us = new UnmanagedStructure(structPtr, typeof(Native.JsonStruct));
                var jStruct = (Native.JsonStruct)us.Managed;

                return new JsonStruct(jStruct);
            }
        }

    }
}
