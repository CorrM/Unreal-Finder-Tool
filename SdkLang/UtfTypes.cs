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
        public string GameName, GameVersion, NamespaceName;
        public int MemberAlignment, PointerSize;
        public bool IsExternal, IsGObjectsChunks, ShouldConvertStaticMethods, ShouldUseStrings;
        public bool ShouldXorStrings, ShouldGenerateFunctionParametersFile;
    }

    public struct JsonVar
    {
        public string Name, Type;
        public size_t Size, Offset;

        public JsonVar(Native.JsonVar nJsonVar)
        {
            Name = nJsonVar.Name.Str;
            Offset = nJsonVar.Offset;
            Size = nJsonVar.Size;
            Type = nJsonVar.Type.Str;

        }
    }
    public struct JsonStruct
    {
        public string Name, Super;
        public List<JsonVar> Members;

        public JsonStruct(Native.JsonStruct nStruct)
        {
            Name = nStruct.StructName.Str;
            Super = nStruct.StructSuper.Str;

            Members = new CTypes.UftArrayPtr(nStruct.Members.Ptr, nStruct.Members.Count, nStruct.Members.ItemSize).ToStructList<Native.JsonVar>()
                .Select(nVar => new JsonVar(nVar)).ToList();
        }
    }
    public struct SdkPredefinedMethod
    {
        public string Signature, Body;
        public Native.PredefinedMethod.Type MethodType;

        public SdkPredefinedMethod(Native.PredefinedMethod nPredefined)
        {
            Signature = nPredefined.Signature.Str;
            Body = nPredefined.Body.Str;
            MethodType = nPredefined.MethodType;
        }
    }
    public struct SdkConstant
    {
        public string Name, Value;

        public SdkConstant(Native.Constant nConstant)
        {
            Name = nConstant.Name.Str;
            Value = nConstant.Value.Str;
        }
    }
    public struct SdkEnum
    {
        public string Name;
        public string FullName;
        public List<string> Values;

        public SdkEnum(Native.Enum nEnum)
        {
            Name = nEnum.Name.Str;
            FullName = nEnum.FullName.Str;
            Values = new CTypes.UftStringArrayPtr(nEnum.Values.Ptr, nEnum.Values.Count).ToList();
        }
    }
    public struct SdkMember
    {
        public string Name, Type, FlagsString, Comment;
        public bool IsStatic;
        public size_t Offset, Size, Flags;

        public SdkMember(Native.Member nMember)
        {
            Name = nMember.Name.Str;
            Type = nMember.Type.Str;
            FlagsString = nMember.FlagsString.Str;
            Comment = nMember.Comment.Str;
            IsStatic = nMember.IsStatic;
            Offset = nMember.Offset;
            Size = nMember.Size;
            Flags = nMember.Flags;
        }
    }
    public struct SdkScriptStruct
    {
        public string Name, Type, FullName, NameCpp, NameCppFull;
        public size_t Size, InheritedSize;

        public List<SdkMember> Members;
        public List<SdkPredefinedMethod> PredefinedMethods;

        public SdkScriptStruct(Native.ScriptStruct nScriptStruct)
        {
            Name = nScriptStruct.Name.Str;
            Type = nScriptStruct.Type.Str;
            FullName = nScriptStruct.FullName.Str;
            NameCpp = nScriptStruct.NameCpp.Str;
            NameCppFull = nScriptStruct.NameCppFull.Str;

            Size = nScriptStruct.Size;
            InheritedSize = nScriptStruct.InheritedSize;

            Members = new CTypes.UftArrayPtr(nScriptStruct.Members.Ptr, nScriptStruct.Members.Count, nScriptStruct.Members.ItemSize)
                .ToStructList<Native.Member>()
                .Select(nVar => new SdkMember(nVar)).ToList();

            PredefinedMethods = new CTypes.UftArrayPtr(nScriptStruct.PredefinedMethods.Ptr, nScriptStruct.PredefinedMethods.Count, nScriptStruct.PredefinedMethods.ItemSize)
                .ToStructList<Native.PredefinedMethod>()
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
                CppType = nParameter.CppType.Str;
                Name = nParameter.Name.Str;
                FlagsString = nParameter.FlagsString.Str;
            }
        }

        public size_t Index;
        public string Name, FullName, FlagsString;
        public List<SdkParameter> Parameters;
        public bool IsNative, IsStatic;

        public SdkMethod(Native.Method nMethod)
        {
            Index = nMethod.Index;
            Name = nMethod.Name.Str;
            FullName = nMethod.FullName.Str;
            FlagsString = nMethod.FlagsString.Str;

            Parameters = new CTypes.UftArrayPtr(nMethod.Parameters.Ptr, nMethod.Parameters.Count, nMethod.Parameters.ItemSize)
                .ToStructList<Native.Method.Parameter>()
                .Select(nVar => new SdkParameter(nVar)).ToList();

            IsNative = nMethod.IsNative;
            IsStatic = nMethod.IsStatic;
        }
    }
    public struct SdkClass
    {
        // ScriptStruct
        public string Name, Type, FullName, NameCpp, NameCppFull;

        public size_t Size;
        public size_t InheritedSize;

        public List<SdkMember> Members;
        public List<SdkPredefinedMethod> PredefinedMethods;

        // Class
        public List<string> VirtualFunctions;
        public List<SdkMethod> Methods;

        public SdkClass(Native.Class nClass)
        {
            Name = nClass.Name.Str;
            Type = nClass.Type.Str;
            FullName = nClass.FullName.Str;
            NameCpp = nClass.NameCpp.Str;
            NameCppFull = nClass.NameCppFull.Str;

            Size = nClass.Size;
            InheritedSize = nClass.InheritedSize;

            Members = new CTypes.UftArrayPtr(nClass.Members.Ptr, nClass.Members.Count, nClass.Members.ItemSize)
                .ToStructList<Native.Member>()
                .Select(nVar => new SdkMember(nVar)).ToList();

            PredefinedMethods = new CTypes.UftArrayPtr(nClass.PredefinedMethods.Ptr, nClass.PredefinedMethods.Count, nClass.PredefinedMethods.ItemSize)
                .ToStructList<Native.PredefinedMethod>()
                .Select(nVar => new SdkPredefinedMethod(nVar)).ToList();

            VirtualFunctions = new CTypes.UftStringArrayPtr(nClass.VirtualFunctions.Ptr, nClass.VirtualFunctions.Count).ToList();

            Methods = new CTypes.UftArrayPtr(nClass.Methods.Ptr, nClass.Methods.Count, nClass.Methods.ItemSize)
                .ToStructList<Native.Method>()
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
            Name = nPackage.Name.Str;

            Constants = new CTypes.UftArrayPtr(nPackage.Constants.Ptr, nPackage.Constants.Count, nPackage.Constants.ItemSize)
                .ToStructList<Native.Constant>()
                .Select(nVar => new SdkConstant(nVar)).ToList();

            Classes = new CTypes.UftArrayPtr(nPackage.Classes.Ptr, nPackage.Classes.Count, nPackage.Classes.ItemSize)
                .ToStructList<Native.Class>()
                .Select(nVar => new SdkClass(nVar)).ToList();

            ScriptStructs = new CTypes.UftArrayPtr(nPackage.ScriptStructs.Ptr, nPackage.ScriptStructs.Count, nPackage.ScriptStructs.ItemSize)
                .ToStructList<Native.ScriptStruct>()
                .Select(nVar => new SdkScriptStruct(nVar)).ToList();

            Enums = new CTypes.UftArrayPtr(nPackage.Enums.Ptr, nPackage.Enums.Count, nPackage.Enums.ItemSize)
                .ToStructList<Native.Enum>()
                .Select(nVar => new SdkEnum(nVar)).ToList();
        }
    }
    public struct SdkUStruct
    {
        public string Name, FullName, CppName;
        public size_t PropertySize;

        public SdkUStruct(Native.UStruct nStruct)
        {
            Name = nStruct.Name.Str;
            FullName = nStruct.FullName.Str;
            CppName = nStruct.CppName.Str;

            PropertySize = nStruct.PropertySize;
        }
    }
}
