using System;
using System.Runtime.InteropServices;

namespace SdkLang
{
    public class Native
    {
        [StructLayout(LayoutKind.Sequential)]
        public struct StructArray
        {
            public IntPtr Ptr; // Pointer to Pointer [in cpp will be (Type** ptr)]
            public IntPtr Count;
            public IntPtr ItemSize;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct StringArray
        {
            public IntPtr Ptr;
            public IntPtr Count;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct GenInfo
        {
            [MarshalAs(UnmanagedType.LPStr)]
            public string UftPath;
            [MarshalAs(UnmanagedType.LPStr)]
            public string SdkPath;
            [MarshalAs(UnmanagedType.LPStr)]
            public string LangPath;
            [MarshalAs(UnmanagedType.LPStr)]
            public string SdkLang;

            [MarshalAs(UnmanagedType.LPStr)]
            public string GameName;
            [MarshalAs(UnmanagedType.LPStr)]
            public string GameVersion;
            [MarshalAs(UnmanagedType.LPStr)]
            public string NamespaceName;
            [MarshalAs(UnmanagedType.LPStr)]
            public string ModuleName;

            public IntPtr MemberAlignment;
            public IntPtr PointerSize;
            public IntPtr ModuleBase;
            public IntPtr GNameAddress;
            public IntPtr GObjectsAddress;

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
            [MarshalAs(UnmanagedType.LPStr)]
            public string Name;
            [MarshalAs(UnmanagedType.LPStr)]
            public string Type;
            public IntPtr Size, Offset;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct JsonStruct
        {
            [MarshalAs(UnmanagedType.LPStr)]
            public string StructName;
            [MarshalAs(UnmanagedType.LPStr)]
            public string StructSuper;
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

            [MarshalAs(UnmanagedType.LPStr)]
            public string Signature;
            [MarshalAs(UnmanagedType.LPStr)]
            public string Body;
            public Type MethodType;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct Constant
        {
            [MarshalAs(UnmanagedType.LPStr)]
            public string Name;
            [MarshalAs(UnmanagedType.LPStr)]
            public string Value;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct Enum
        {
            [MarshalAs(UnmanagedType.LPStr)]
            public string Name;
            [MarshalAs(UnmanagedType.LPStr)]
            public string FullName;
            public StringArray Values;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct Member
        {
            [MarshalAs(UnmanagedType.LPStr)]
            public string Name;
            [MarshalAs(UnmanagedType.LPStr)]
            public string Type;
            public bool IsStatic;
            public IntPtr Offset;
            public IntPtr Size;
            public IntPtr Flags;
            [MarshalAs(UnmanagedType.LPStr)]
            public string FlagsString;
            [MarshalAs(UnmanagedType.LPStr)]
            public string Comment;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct ScriptStruct
        {
            [MarshalAs(UnmanagedType.LPStr)]
            public string Name;
            [MarshalAs(UnmanagedType.LPStr)]
            public string FullName;
            [MarshalAs(UnmanagedType.LPStr)]
            public string NameCpp;
            [MarshalAs(UnmanagedType.LPStr)]
            public string NameCppFull;

            public IntPtr Size;
            public IntPtr InheritedSize;

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
                [MarshalAs(UnmanagedType.LPStr)]
                public string CppType;
                [MarshalAs(UnmanagedType.LPStr)]
                public string Name;
                [MarshalAs(UnmanagedType.LPStr)]
                public string FlagsString;
            }

            public IntPtr Index;
            [MarshalAs(UnmanagedType.LPStr)]
            public string Name;
            [MarshalAs(UnmanagedType.LPStr)]
            public string FullName;
            public StructArray Parameters; // Parameter
            [MarshalAs(UnmanagedType.LPStr)]
            public string FlagsString;
            public bool IsNative;
            public bool IsStatic;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct Class
        {
            // ScriptStruct
            [MarshalAs(UnmanagedType.LPStr)]
            public string Name;
            [MarshalAs(UnmanagedType.LPStr)]
            public string FullName;
            [MarshalAs(UnmanagedType.LPStr)]
            public string NameCpp;
            [MarshalAs(UnmanagedType.LPStr)]
            public string NameCppFull;

            public IntPtr Size;
            public IntPtr InheritedSize;

            public StructArray Members; // Member
            public StructArray PredefinedMethods; // PredefinedMethod

            // Class
            public StringArray VirtualFunctions;
            public StructArray Methods; // Method
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct Package
        {
            [MarshalAs(UnmanagedType.LPStr)]
            public string Name;
            public StructArray Constants; // Constant
            public StructArray Classes; // Class
            public StructArray ScriptStructs; // ScriptStruct
            public StructArray Enums; // Enum
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct UStruct
        {
            [MarshalAs(UnmanagedType.LPStr)]
            public string Name;
            [MarshalAs(UnmanagedType.LPStr)]
            public string FullName;
            [MarshalAs(UnmanagedType.LPStr)]
            public string CppName;

            public IntPtr PropertySize;
        }
    }
}
