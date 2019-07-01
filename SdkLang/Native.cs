using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using net.r_eg.Conari.Types;
using SdkLang.Utils;

namespace SdkLang
{
    public class Native
    {
        [StructLayout(LayoutKind.Sequential)]
        public struct StructArray
        {
            public IntPtr Ptr;
            public size_t Count;
            public size_t ItemSize;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct StringArray
        {
            public IntPtr Ptr;
            public size_t Count;
        };

        [StructLayout(LayoutKind.Sequential)]
        public struct JsonVar
        {
            public CTypes.UftCharPtr Name;
            public CTypes.UftCharPtr Type;
            public size_t Size, Offset;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct JsonStruct
        {
            public CTypes.UftCharPtr StructName;
            public CTypes.UftCharPtr StructSuper;
            public StructArray Members;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct PredefinedMethod
        {
            public enum Type
            {
                Default,
                Inline
            }

            public CTypes.UftCharPtr Signature;
            public CTypes.UftCharPtr Body;
            public Type MethodType;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct Constant
        {
            public CTypes.UftCharPtr Name;
            public CTypes.UftCharPtr Value;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct Enum
        {
            public CTypes.UftCharPtr Name;
            public CTypes.UftCharPtr FullName;
            public StringArray Values;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct Member
        {
            public CTypes.UftCharPtr Name;
            public CTypes.UftCharPtr FullName;
            public bool IsStatic;
            public size_t Offset;
            public size_t Size;
            public size_t Flags;
            public CTypes.UftCharPtr FlagsString;
            public CTypes.UftCharPtr Comment;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct ScriptStruct
        {
            public CTypes.UftCharPtr Name;
            public CTypes.UftCharPtr Type;
            public CTypes.UftCharPtr FullName;
            public CTypes.UftCharPtr NameCpp;
            public CTypes.UftCharPtr NameCppFull;

            public size_t Size;
            public size_t InheritedSize;

            public StructArray Members; // Member
            public StructArray PredefinedMethods; // PredefinedMethod
        }

        public struct Method
        {
            public struct Parameter
            {
                public enum Type
                {
                    Default,
                    Out,
                    Return
                }

                public Type ParamType;
                public bool PassByReference;
                public CTypes.UftCharPtr CppType;
                public CTypes.UftCharPtr Name;
                public CTypes.UftCharPtr FlagsString;
            }

            public size_t Index;
            public CTypes.UftCharPtr Name;
            public CTypes.UftCharPtr FullName;
            public StructArray Parameters; // Parameter
            public CTypes.UftCharPtr FlagsString;
            public bool IsNative;
            public bool IsStatic;
        }

        public struct Class
        {
            // ScriptStruct
            public CTypes.UftCharPtr Name;
            public CTypes.UftCharPtr Type;
            public CTypes.UftCharPtr FullName;
            public CTypes.UftCharPtr NameCpp;
            public CTypes.UftCharPtr NameCppFull;

            public size_t Size;
            public size_t InheritedSize;

            public StructArray Members; // Member
            public StructArray PredefinedMethods; // PredefinedMethod

            // Class
            public StringArray VirtualFunctions;
            public StructArray Methods; // Method
        }

        public struct Package
        {
            public CTypes.UftCharPtr Name;
            public StructArray Constants; // Constant
            public StructArray Classes; // Class
            public StructArray ScriptStructs; // ScriptStruct
            public StructArray Enums; // Enum
        }
    }
}
