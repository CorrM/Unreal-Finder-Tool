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
    public struct SdkGenInfo
    {
        public string UftPath, SdkPath, LangPath, SdkLang;

        public string GameName, GameVersion, NamespaceName;
        public long MemberAlignment, PointerSize;
        public bool IsExternal, IsGObjectsChunks, ShouldConvertStaticMethods, ShouldUseStrings;
        public bool ShouldXorStrings, ShouldGenerateFunctionParametersFile;

        public SdkGenInfo(Native.GenInfo nGenInfo)
        {
            UftPath = nGenInfo.UftPath;
            SdkPath = nGenInfo.SdkPath;
            LangPath = nGenInfo.LangPath;

            SdkLang = nGenInfo.SdkLang;

            GameName = nGenInfo.GameName;
            GameVersion = nGenInfo.GameVersion;
            NamespaceName = nGenInfo.NamespaceName;

            MemberAlignment = nGenInfo.MemberAlignment.ToInt64();
            PointerSize = nGenInfo.PointerSize.ToInt64();
            IsExternal = nGenInfo.IsExternal;
            IsGObjectsChunks = nGenInfo.IsGObjectsChunks;
            ShouldConvertStaticMethods = nGenInfo.ShouldConvertStaticMethods;
            ShouldUseStrings = nGenInfo.ShouldUseStrings;
            ShouldXorStrings = nGenInfo.ShouldXorStrings;
            ShouldGenerateFunctionParametersFile = nGenInfo.ShouldGenerateFunctionParametersFile;
        }
    }
    public struct JsonVar
    {
        public string Name, Type;
        public IntPtr Size, Offset;

        public JsonVar(Native.JsonVar nJsonVar)
        {
            Name = nJsonVar.Name;
            Offset = nJsonVar.Offset;
            Size = nJsonVar.Size;
            Type = nJsonVar.Type;

        }
    }
    public struct JsonStruct
    {
        public string Name, Super;
        public List<JsonVar> Members;

        public JsonStruct(Native.JsonStruct nStruct)
        {
            Name = nStruct.StructName;
            Super = nStruct.StructSuper;

            Members = new CTypes.UftArrayPtr(nStruct.Members.Ptr, nStruct.Members.Count.ToInt32(), nStruct.Members.ItemSize.ToInt32()).ToPtrStructList<Native.JsonVar>()
                .Select(nVar => new JsonVar(nVar)).ToList();
        }
    }
    public struct SdkPredefinedMethod
    {
        public string Signature, Body;
        public Native.PredefinedMethod.Type MethodType;

        public SdkPredefinedMethod(Native.PredefinedMethod nPredefined)
        {
            Signature = nPredefined.Signature;
            Body = nPredefined.Body;
            MethodType = nPredefined.MethodType;
        }
    }
    public struct SdkConstant
    {
        public string Name, Value;

        public SdkConstant(Native.Constant nConstant)
        {
            Name = nConstant.Name;
            Value = nConstant.Value;
        }
    }
    public struct SdkEnum
    {
        public string Name;
        public string FullName;
        public List<string> Values;

        public SdkEnum(Native.Enum nEnum)
        {
            Name = nEnum.Name;
            FullName = nEnum.FullName;
            Values = new CTypes.UftStringArrayPtr(nEnum.Values.Ptr, nEnum.Values.Count.ToInt32()).ToList();
        }
    }
    public struct SdkMember
    {
        public string Name, Type, FlagsString, Comment;
        public bool IsStatic;
        public IntPtr Offset, Size, Flags;

        public SdkMember(Native.Member nMember)
        {
            Name = nMember.Name;
            Type = nMember.Type;
            FlagsString = nMember.FlagsString;
            Comment = nMember.Comment;
            IsStatic = nMember.IsStatic;
            Offset = nMember.Offset;
            Size = nMember.Size;
            Flags = nMember.Flags;
        }
    }
    public struct SdkScriptStruct
    {
        public string Name, FullName, NameCpp, NameCppFull;
        public IntPtr Size, InheritedSize;

        public List<SdkMember> Members;
        public List<SdkPredefinedMethod> PredefinedMethods;

        public SdkScriptStruct(Native.ScriptStruct nScriptStruct)
        {
            Name = nScriptStruct.Name;
            FullName = nScriptStruct.FullName;
            NameCpp = nScriptStruct.NameCpp;
            NameCppFull = nScriptStruct.NameCppFull;

            Size = nScriptStruct.Size;
            InheritedSize = nScriptStruct.InheritedSize;

            Members = new CTypes.UftArrayPtr(nScriptStruct.Members.Ptr, nScriptStruct.Members.Count.ToInt32(), nScriptStruct.Members.ItemSize.ToInt32())
                .ToPtrStructList<Native.Member>()
                .Select(nVar => new SdkMember(nVar)).ToList();

            PredefinedMethods = new CTypes.UftArrayPtr(nScriptStruct.PredefinedMethods.Ptr, nScriptStruct.PredefinedMethods.Count.ToInt32(), nScriptStruct.PredefinedMethods.ItemSize.ToInt32())
                .ToPtrStructList<Native.PredefinedMethod>()
                .Select(nVar => new SdkPredefinedMethod(nVar)).ToList();
        }
    }
    public struct SdkMethod
    {
        public struct SdkParameter
        {
            public Native.Method.Parameter.Type ParamType;
            public bool PassByReference;
            public string CppType, Name, FlagsString;

            public SdkParameter(Native.Method.Parameter nParameter)
            {
                ParamType = nParameter.ParamType;
                PassByReference = nParameter.PassByReference;
                CppType = nParameter.CppType;
                Name = nParameter.Name;
                FlagsString = nParameter.FlagsString;
            }
        }

        public IntPtr Index;
        public string Name, FullName, FlagsString;
        public List<SdkParameter> Parameters;
        public bool IsNative, IsStatic;

        public SdkMethod(Native.Method nMethod)
        {
            Index = nMethod.Index;
            Name = nMethod.Name;
            FullName = nMethod.FullName;
            FlagsString = nMethod.FlagsString;

            Parameters = new CTypes.UftArrayPtr(nMethod.Parameters.Ptr, nMethod.Parameters.Count.ToInt32(), nMethod.Parameters.ItemSize.ToInt32())
                .ToPtrStructList<Native.Method.Parameter>()
                .Select(nVar => new SdkParameter(nVar)).ToList();

            IsNative = nMethod.IsNative;
            IsStatic = nMethod.IsStatic;
        }
    }
    public struct SdkClass
    {
        // ScriptStruct
        public string Name, FullName, NameCpp, NameCppFull;

        public IntPtr Size;
        public IntPtr InheritedSize;

        public List<SdkMember> Members;
        public List<SdkPredefinedMethod> PredefinedMethods;

        // Class
        public List<string> VirtualFunctions;
        public List<SdkMethod> Methods;

        public SdkClass(Native.Class nClass)
        {
            Name = nClass.Name;
            FullName = nClass.FullName;
            NameCpp = nClass.NameCpp;
            NameCppFull = nClass.NameCppFull;

            Size = nClass.Size;
            InheritedSize = nClass.InheritedSize;

            Members = new CTypes.UftArrayPtr(nClass.Members.Ptr, nClass.Members.Count, nClass.Members.ItemSize)
                .ToPtrStructList<Native.Member>()
                .Select(nVar => new SdkMember(nVar)).ToList();

            PredefinedMethods = new CTypes.UftArrayPtr(nClass.PredefinedMethods.Ptr, nClass.PredefinedMethods.Count, nClass.PredefinedMethods.ItemSize)
                .ToPtrStructList<Native.PredefinedMethod>()
                .Select(nVar => new SdkPredefinedMethod(nVar)).ToList();

            VirtualFunctions = new CTypes.UftStringArrayPtr(nClass.VirtualFunctions.Ptr, nClass.VirtualFunctions.Count).ToList();

            Methods = new CTypes.UftArrayPtr(nClass.Methods.Ptr, nClass.Methods.Count, nClass.Methods.ItemSize)
                .ToPtrStructList<Native.Method>()
                .Select(nVar => new SdkMethod(nVar)).ToList();
        }
    }
    public struct SdkPackage
    {
        public string Name;
        public List<SdkConstant> Constants;
        public List<SdkClass> Classes;
        public List<SdkScriptStruct> ScriptStructs;
        public List<SdkEnum> Enums;

        public SdkPackage(Native.Package nPackage)
        {
            Name = nPackage.Name;

            Constants = new CTypes.UftArrayPtr(nPackage.Constants.Ptr, nPackage.Constants.Count, nPackage.Constants.ItemSize)
                .ToPtrStructList<Native.Constant>()
                .Select(nVar => new SdkConstant(nVar))
                .ToList();

            Classes = new CTypes.UftArrayPtr(nPackage.Classes.Ptr, nPackage.Classes.Count, nPackage.Classes.ItemSize)
                .ToPtrStructList<Native.Class>()
                .Select(nVar => new SdkClass(nVar))
                .ToList();

            ScriptStructs = new CTypes.UftArrayPtr(nPackage.ScriptStructs.Ptr, nPackage.ScriptStructs.Count, nPackage.ScriptStructs.ItemSize)
                .ToPtrStructList<Native.ScriptStruct>()
                .Select(nVar => new SdkScriptStruct(nVar))
                .ToList();

            Enums = new CTypes.UftArrayPtr(nPackage.Enums.Ptr, nPackage.Enums.Count, nPackage.Enums.ItemSize)
                .ToPtrStructList<Native.Enum>()
                .Select(nVar => new SdkEnum(nVar))
                .ToList();
        }
    }
    public struct SdkUStruct
    {
        public string Name, FullName, CppName;
        public IntPtr PropertySize;

        public SdkUStruct(Native.UStruct nStruct)
        {
            Name = nStruct.Name;
            FullName = nStruct.FullName;
            CppName = nStruct.CppName;

            PropertySize = nStruct.PropertySize;
        }
    }
}
