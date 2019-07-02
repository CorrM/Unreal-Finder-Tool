using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using net.r_eg.Conari.Types;
using SdkLang.Utils;
using static SdkLang.Utils.CTypes;

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
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct GenInfo
        {
            public UftCharPtr UftPath;
            public UftCharPtr SdkPath;
            public UftCharPtr LangPath;

            public UftCharPtr SdkLang;

            public UftCharPtr GameName;
            public UftCharPtr GameVersion;
            public UftCharPtr NamespaceName;

            public size_t MemberAlignment;
            public size_t PointerSize;
            public bool IsExternal;
            public bool IsGObjectsChunks;
            public bool ShouldConvertStaticMethods;
            public bool ShouldUseStrings;
            public bool ShouldXorStrings;
            public bool ShouldGenerateFunctionParametersFile;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct JsonVar
        {
            public UftCharPtr Name;
            public UftCharPtr Type;
            public size_t Size, Offset;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct JsonStruct
        {
            public UftCharPtr StructName;
            public UftCharPtr StructSuper;
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

            public UftCharPtr Signature;
            public UftCharPtr Body;
            public Type MethodType;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct Constant
        {
            public UftCharPtr Name;
            public UftCharPtr Value;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct Enum
        {
            public UftCharPtr Name;
            public UftCharPtr FullName;
            public StringArray Values;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct Member
        {
            public UftCharPtr Name;
            public UftCharPtr Type;
            public bool IsStatic;
            public size_t Offset;
            public size_t Size;
            public size_t Flags;
            public UftCharPtr FlagsString;
            public UftCharPtr Comment;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct ScriptStruct
        {
            public UftCharPtr Name;
            public UftCharPtr Type;
            public UftCharPtr FullName;
            public UftCharPtr NameCpp;
            public UftCharPtr NameCppFull;

            public size_t Size;
            public size_t InheritedSize;

            public StructArray Members; // Member
            public StructArray PredefinedMethods; // PredefinedMethod
        }

        [StructLayout(LayoutKind.Sequential)]
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
                public UftCharPtr CppType;
                public UftCharPtr Name;
                public UftCharPtr FlagsString;
            }

            public size_t Index;
            public UftCharPtr Name;
            public UftCharPtr FullName;
            public StructArray Parameters; // Parameter
            public UftCharPtr FlagsString;
            public bool IsNative;
            public bool IsStatic;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct Class
        {
            // ScriptStruct
            public UftCharPtr Name;
            public UftCharPtr Type;
            public UftCharPtr FullName;
            public UftCharPtr NameCpp;
            public UftCharPtr NameCppFull;

            public size_t Size;
            public size_t InheritedSize;

            public StructArray Members; // Member
            public StructArray PredefinedMethods; // PredefinedMethod

            // Class
            public StringArray VirtualFunctions;
            public StructArray Methods; // Method
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct Package
        {
            public UftCharPtr Name;
            public StructArray Constants; // Constant
            public StructArray Classes; // Class
            public StructArray ScriptStructs; // ScriptStruct
            public StructArray Enums; // Enum
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct UStruct
        {
            public UftCharPtr Name;
            public UftCharPtr FullName;
            public UftCharPtr CppName;

            public size_t PropertySize;
        }
    }
}
