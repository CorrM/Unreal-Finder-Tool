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
    public class UftStringBuilder
    {
        public readonly StringBuilder BaseStr;

        public UftStringBuilder() : this(string.Empty) { }

        public UftStringBuilder(string str)
        {
            BaseStr = new StringBuilder(str);
        }

        public static UftStringBuilder operator +(UftStringBuilder sBuilder, string str)
        {
            sBuilder.BaseStr.Append(str);
            return sBuilder;
        }

        public static implicit operator string(UftStringBuilder builder)
        {
            return builder.ToString();
        }

        public override string ToString()
        {
            return BaseStr.ToString();
        }
    }

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
