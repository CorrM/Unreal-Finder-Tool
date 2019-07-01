using SdkLang;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using net.r_eg.Conari;
using SdkLang.Utils;

namespace SdkLang.Langs
{
    public class BasicHeader : IncludeFile<UftCpp>
    {
        public override string FileName() => "Basic.h";
        public List<string> Pragmas() => new List<string>() { "warning(disable: 4267)" };
        public List<string> Include() => new List<string>() { "<vector>", "<locale>", "<set>" };

        public override void Process()
        {
            // Read File
            var fileStr = ReadThisFile(Main.IncludePath);

            // Replace Main stuff
            fileStr.Replace("/*!!INCLUDE_PLACEHOLDER!!*/", TargetLang.GetFileHeader(Pragmas(), Include(), true));
            fileStr.Replace("/*!!FOOTER_PLACEHOLDER!!*/", TargetLang.GetFileFooter());

            var jStruct = UtilsFunctions.GetStruct("FUObjectItem");
            string fUObjectItemStr = string.Empty;

            // Replace Major Stuff
            foreach (var mem in jStruct.Members)
            {
                fUObjectItemStr += mem.Type.All(char.IsDigit)
                    ? $"\tunsigned char {mem.Name} [{mem.Type}];\n"
                    : $"\t{mem.Type} {mem.Name};\n";
            }

            fileStr.Replace("/*!!DEFINE_PLACEHOLDER!!*/", Main.GenInfo.IsGObjectsChunks ? "#define GOBJECTS_CHUNKS" : "");
            fileStr.Replace("/*!!POINTER_SIZE_PLACEHOLDER!!*/", Main.GenInfo.PointerSize.ToString());
            fileStr.Replace("/*!!FUObjectItem_MEMBERS_PLACEHOLDER!!*/", fUObjectItemStr);

            // Write File
            CopyToSdk(fileStr);
        }
    }

    public class UftCpp : UtfLang
    {
        public enum FileContentType
        {
            Structs,
            Classes,
            Functions,
            FunctionParameters
        };
        public string GetFileHeader(List<string> pragmas, List<string> includes, bool isHeaderFile)
        {
            var genInfo = Main.GenInfo;
            var sb = new StringBuilder();

            // Pragmas
            if (isHeaderFile)
            {
                sb.Append("#pragma once\n");
                if (pragmas.Count > 0)
                    foreach (string i in pragmas) { sb.Append("#pragma " + i + "\n"); }
                sb.Append("\n");
            }

            if (genInfo.IsExternal)
                sb.Append("#include \"../Memory.h\"\n");

            // Includes
            if (includes.Count > 0)
                foreach (string i in includes) { sb.Append("#include " + i + "\n"); }
            sb.Append("\n");

            // 
            sb.Append($"// Name: {genInfo.GameName}, Version: {genInfo.GameVersion}\n\n");
            sb.Append($"#ifdef _MSC_VER\n\t#pragma pack(push, 0x{genInfo.MemberAlignment:X2})\n#endif\n\n");
            sb.Append($"namespace {genInfo.NamespaceName}\n{{\n");

            return sb.ToString();
        }
        public string GetFileHeader(List<string> includes, bool isHeaderFile)
        {
            return GetFileHeader(new List<string>(), includes, isHeaderFile);
        }
        public string GetFileHeader(bool isHeaderFile)
        {
            return GetFileHeader(new List<string>(), new List<string>(), isHeaderFile);
        }
        public string GetFileFooter()
        {
            return "}\n\n#ifdef _MSC_VER\n\t#pragma pack(pop)\n#endif\n";
        }
        public string GetSectionHeader(string name)
        {
            return 
                $"//---------------------------------------------------------------------------\n" +
                $"// {name}\n" +
                $"//---------------------------------------------------------------------------\n\n";
        }
        public string GenerateFileName(FileContentType type, string packageName)
        {
            switch (type)
            {
                case FileContentType.Structs:
                    return $"{packageName}_structs.h";
                case FileContentType.Classes:
                    return $"{packageName}_classes.h";
                case FileContentType.Functions:
                    return $"{packageName}_functions.cpp";
                case FileContentType.FunctionParameters:
                    return $"{packageName}_parameters.h";
                default:
                    throw new Exception("WHAT IS THIS TYPE .?!!");
            }
        }

        #region BuildMethod
        public string BuildMethodSignature(SdkMethod m, SdkClass c, bool inHeader)
        {
            string text = string.Empty;

            // static
            if (m.IsStatic && inHeader && Main.GenInfo.ShouldConvertStaticMethods)
                text += "static ";

            // Return Type
            var retn = m.Parameters.Where(item => item.ParamType == Native.Method.Parameter.Type.Return).ToList();
            text += (retn.Any() ? retn.First().CppType : "void") + " ";

            // inHeader
            if (!inHeader)
                text += $"{c.NameCpp}::";
            if (m.IsStatic && Main.GenInfo.ShouldConvertStaticMethods)
                text += "STATIC_";
            text += m.Name;

            // Parameters
            text += "(";
            text += m.Parameters
                .Where(p => p.ParamType != Native.Method.Parameter.Type.Return)
                .OrderBy(p => p.ParamType)
                .Select(p => (p.PassByReference ? "const " : "") + p.CppType + (p.PassByReference ? "& " : p.ParamType == Native.Method.Parameter.Type.Out ? "* " : " ") + p.Name)
                .Aggregate((cur, next) => cur + ", " + next);
            text += ")";

            return text;
        }
        public string BuildMethodBody(SdkClass c, SdkMethod m)
        {
            string text = string.Empty;

            // Function Pointer
            text += "{\n\tstatic auto fn";
            if (Main.GenInfo.ShouldUseStrings)
            {
                text += " = UObject::FindObject<UFunction>(";

                if (Main.GenInfo.ShouldXorStrings)
                    text += $"_xor_(\"{m.FullName}\")";
                else
                    text += "\"{m.FullName}\"";

                text += ");\n\n";
            }
            else
            {
                text += $" = UObject::GetObjectCasted<UFunction>({m.Index});\n\n";
            }

            // Parameters
            if (Main.GenInfo.ShouldGenerateFunctionParametersFile)
            {
                text += $"\t{c.NameCpp}_{m.Name}_Params params;\n";
            }
            else
            {
                text += "\tstruct\n\t{\n";
                foreach (var param in m.Parameters)
                    text += $"\t\t{param.CppType,-30} {param.Name};\n";
                text += "\t} params;\n";
            }

            var retn = m.Parameters.Where(item => item.ParamType == Native.Method.Parameter.Type.Default).ToList();
            if (retn.Any())
            {
                foreach (var param in retn)
                    text += $"\tparams.{param.Name} = {param.Name};\n";
            }
            text += "\n";

            //Function Call
            text += "\tauto flags = fn->FunctionFlags;\n";
            if (m.IsNative)
                text += $"\tfn->FunctionFlags |= 0x{UEFunctionFlags.Native:X};\n";
            text += "\n";

            if (m.IsStatic && !Main.GenInfo.ShouldConvertStaticMethods)
            {
                text += "\tstatic auto defaultObj = StaticClass()->CreateDefaultObject();\n";
                text += "\tdefaultObj->ProcessEvent(fn, &params);\n\n";
            }
            else
            {
                text += "\tUObject::ProcessEvent(fn, &params);\n\n";
            }
            text += "\tfn->FunctionFlags = flags;\n";

            //Out Parameters
            var rOut = m.Parameters.Where(item => item.ParamType == Native.Method.Parameter.Type.Out).ToList();
            if (rOut.Any())
            {
                text += "\n";
                foreach (var param in rOut)
                    text += $"\tif ({param.Name} != nullptr)\n" +
                            $"\t\t*{param.Name} = params.{param.Name};\n";
            }
            text += "\n";

	        //Return Value
            var ret = m.Parameters.Where(item => item.ParamType == Native.Method.Parameter.Type.Return).ToList();
            if (ret.Any())
            {
                foreach (var param in ret)
                    text += $"\n\treturn params.{ret.First().Name};\n";
            }
            text += "}\n";

            return text;
        }
        #endregion

        #region Print
        public void PrintConstant(string fileName, SdkConstant c)
        {
            IncludeFile<UftCpp>.AppendToSdk(Main.SdkPath, fileName, $"#define CONST_{c.Name,-50} {c.Value}\n");
        }
        public void PrintEnum(string fileName, SdkEnum e)
        {
            string text = $"// {e.FullName}\nenum class {e.Name} : uint8_t\n{{\n";

            for (int i = 0; i < e.Values.Count; i++)
                text += $"\t{e.Values[i],-30} = {i},\n";

            text += $"\n}};\n\n";
            IncludeFile<UftCpp>.AppendToSdk(Main.SdkPath, fileName, text);
        }
        public void PrintStruct(string fileName, SdkScriptStruct ss)
        {
            string text = $"// {ss.FullName}\n// ";

            if (ss.InheritedSize > 0)
                text += $"0x{(long)ss.Size - ss.InheritedSize:X4} ({(long)ss.Size:X4} - 0x{(long)ss.InheritedSize:X4})\n";
            else
                text += $"0x{(long)ss.Size:X4}\n";

            text += $"{ss.NameCppFull}\n{{\n";

            //Member
            foreach (var m in ss.Members)
            {
                text += 
                    $"\t{(m.IsStatic ? "static " + m.Type : m.Type),-50} {m.Name,-58}; // 0x{(long)m.Offset:X4}(0x{(long)m.Size:X4})" +
                    (!string.IsNullOrEmpty(m.Comment) ? " " + m.Comment : "") +
                    (!string.IsNullOrEmpty(m.FlagsString) ? " (" + m.FlagsString + ")" : "") +
                    "\n";
            }
            text += "\n";

            //Predefined Methods
            if (ss.PredefinedMethods.Count > 0)
            {
                text += "\n";
                foreach (var m in ss.PredefinedMethods)
                {
                    if (m.MethodType == Native.PredefinedMethod.Type.Inline)
                        text += m.Body;
                    else
                        text += $"\t{m.Signature};";
                    text += "\n\n";
                }
            }
            text += "};\n";

            IncludeFile<UftCpp>.AppendToSdk(Main.SdkPath, fileName, text);
        }
        public void PrintClass(string fileName, SdkClass c)
        {
            string text = $"// {c.FullName}\n// ";

            if (c.InheritedSize > 0)
                text += $"0x{(long)c.Size - c.InheritedSize:X4} ({(long)c.Size:X4} - 0x{(long)c.InheritedSize:X4})\n";
            else
                text += $"0x{(long)c.Size:X4}\n";

            text += $"{c.NameCppFull}\n{{\npublic:\n";

            // Member
            foreach (var m in c.Members)
            {
                text +=
                    $"\t{(m.IsStatic ? "static " + m.Type : m.Type),-50} {m.Name,-58}; // 0x{(long)m.Offset:X4}(0x{(long)m.Size:X4})" +
                    (!string.IsNullOrEmpty(m.Comment) ? " " + m.Comment : "") +
                    (!string.IsNullOrEmpty(m.FlagsString) ? " (" + m.FlagsString + ")" : "") +
                    "\n";
            }
            text += "\n";

            // Predefined Methods
            if (c.PredefinedMethods.Count > 0)
            {
                text += "\n";
                foreach (var m in c.PredefinedMethods)
                {
                    if (m.MethodType == Native.PredefinedMethod.Type.Inline)
                        text += m.Body;
                    else
                        text += $"\t{m.Signature};";

                    text += "\n\n";
                }
            }

            // Methods
            if (c.PredefinedMethods.Count > 0)
            {
                text += "\n";
                foreach (var m in c.Methods)
                {
                    text += $"\t{BuildMethodSignature(m, new SdkClass(),  true)};\n";
                }
            }

            text += "};\n\n";

            IncludeFile<UftCpp>.AppendToSdk(Main.SdkPath, fileName, text);
        }
        #endregion

        #region SavePackage
        public void SaveStructs(SdkPackage package)
        {
            string fileName = GenerateFileName(FileContentType.Structs, package.Name);
            IncludeFile<UftCpp>.CreateFile(Main.SdkPath, fileName);
            IncludeFile<UftCpp>.AppendToSdk(Main.SdkPath, fileName, GetFileHeader(true));

            if (package.Constants.Count > 0)
            {
                IncludeFile<UftCpp>.AppendToSdk(Main.SdkPath, fileName, GetSectionHeader("Constants"));
                foreach (var c in package.Constants)
                    PrintConstant(fileName, c);
            }

            if (package.Enums.Count > 0)
            {
                IncludeFile<UftCpp>.AppendToSdk(Main.SdkPath, fileName, GetSectionHeader("Enums"));
                foreach (var e in package.Enums)
                    PrintEnum(fileName, e);
            }

            if (package.ScriptStructs.Count > 0)
            {
                IncludeFile<UftCpp>.AppendToSdk(Main.SdkPath, fileName, GetSectionHeader("Script Structs"));
                foreach (var ss in package.ScriptStructs)
                    PrintStruct(fileName, ss);
            }
        }
        public string SaveSdkHeader()
        {
            string ret =
                $"// ------------------------------------------------\n" +
                $"// Sdk Generated By ( Unreal Finder Tool By CorrM )\n" +
                $"// ------------------------------------------------\n" +
                $"#pragma once\n\n" +
                $"// Name: {Main.GenInfo.GameName}, Version: {Main.GenInfo.GameVersion}\n\n" +
                $"#include <set>\n" +
                $"#include <string>\n";

            return ret;
        }
        #endregion

        public void Init()
        {
            new BasicHeader().Init(this);
        }
    }
}
