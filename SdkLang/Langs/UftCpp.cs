using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using SdkLang.Utils;

namespace SdkLang.Langs
{
    public class BasicHeader : IncludeFile<UftCpp>
    {
        private readonly List<string> _pragmas = new List<string>() { "warning(disable: 4267)" };
        private readonly List<string> _include = new List<string>() { "<vector>", "<locale>", "<set>" };
        public override string FileName { get; set; } = "Basic.h";

        public override void Process(string includePath)
        {
            // Read File
            var fileStr = ReadThisFile(includePath);

            // Replace Main stuff
            fileStr.BaseStr.Replace("/*!!INCLUDE!!*/", TargetLang.GetFileHeader(_pragmas, _include, true));
            fileStr.BaseStr.Replace("/*!!FOOTER!!*/", TargetLang.GetFileFooter());

            var jStruct = UtilsFunctions.GetStruct("FUObjectItem");
            string fUObjectItemStr = string.Empty;

            // Replace
            foreach (var mem in jStruct.Members)
            {
                fUObjectItemStr += mem.Type.All(char.IsDigit)
                    ? $"\tunsigned char {mem.Name} [{mem.Type}];{Environment.NewLine}"
                    : $"\t{mem.Type} {mem.Name};{Environment.NewLine}";
            }

            fileStr.BaseStr.Replace("/*!!DEFINE!!*/", Main.GenInfo.IsGObjectsChunks ? "#define GOBJECTS_CHUNKS" : "");
            fileStr.BaseStr.Replace("/*!!POINTER_SIZE!!*/", Main.GenInfo.PointerSize.ToString());
            fileStr.BaseStr.Replace("/*!!FUObjectItem_MEMBERS!!*/", fUObjectItemStr);

            // Write File
            CopyToSdk(fileStr);

            // Append To SdkHeader file
            AppendToSdk(Path.GetDirectoryName(Main.GenInfo.SdkPath), "SDK.h", $"{Environment.NewLine}#include \"SDK/{FileName}\"{Environment.NewLine}");
        }
    }
    public class BasicCpp : IncludeFile<UftCpp>
    {
        private readonly List<string> _include = new List<string>() { "\"../SDK.h\"", "<Windows.h>" };
        public override string FileName { get; set; } = "Basic.cpp";

        public override void Process(string includePath)
        {
            // Read File
            var fileStr = ReadThisFile(includePath);

            // Replace Main stuff
            fileStr.BaseStr.Replace("/*!!INCLUDE!!*/", TargetLang.GetFileHeader(_include, false));
            fileStr.BaseStr.Replace("/*!!FOOTER!!*/", TargetLang.GetFileFooter());

            // Replace
            fileStr.BaseStr.Replace("/*!!DEFINE!!*/", "");
            if (!string.IsNullOrWhiteSpace(Main.GenInfo.ModuleName))
            {
                long gObjectsOffset = Main.GenInfo.GObjectsAddress.ToInt64() - Main.GenInfo.ModuleBase.ToInt64();
                long gNamesOffset = Main.GenInfo.GNameAddress.ToInt64() - Main.GenInfo.ModuleBase.ToInt64();
                fileStr.BaseStr.Replace("/*!!AUTO_INIT_SDK!!*/", $"InitSdk(\"{Main.GenInfo.ModuleName}\", 0x{gObjectsOffset:X}, 0x{gNamesOffset:X});");
            }
            else
            {
                fileStr.BaseStr.Replace("/*!!AUTO_INIT_SDK!!*/", "throw std::exception(\"Don't use this func.\");");
            }

            // Write File
            CopyToSdk(fileStr);
        }
    }

    public class UftCpp : UftLang
    {
        #region FileStruct
        public enum FileContentType
        {
            Structs,
            Classes,
            Functions,
            FunctionParameters
        }
        public string GetFileHeader(List<string> pragmas, List<string> includes, bool isHeaderFile)
        {
            var genInfo = Main.GenInfo;
            var sb = new UftStringBuilder();

            // Pragmas
            if (isHeaderFile)
            {
                sb.BaseStr.Append($"#pragma once{Environment.NewLine}");
                if (pragmas.Count > 0)
                    foreach (string i in pragmas) { sb.BaseStr.Append($"#pragma " + i + $"{Environment.NewLine}"); }
                sb.BaseStr.Append($"{Environment.NewLine}");
            }

            if (genInfo.IsExternal)
                sb.BaseStr.Append($"#include \"../Memory.h\"{Environment.NewLine}");

            // Includes
            if (includes.Count > 0)
                foreach (string i in includes) { sb.BaseStr.Append("#include " + i + $"{Environment.NewLine}"); }
            sb.BaseStr.Append($"{Environment.NewLine}");

            // 
            sb.BaseStr.Append($"// Name: {genInfo.GameName.Trim()}, Version: {genInfo.GameVersion}{Environment.NewLine}{Environment.NewLine}");
            sb.BaseStr.Append($"#ifdef _MSC_VER{Environment.NewLine}\t#pragma pack(push, 0x{genInfo.MemberAlignment.ToInt64():X2}){Environment.NewLine}#endif{Environment.NewLine}{Environment.NewLine}");
            sb.BaseStr.Append($"namespace {genInfo.NamespaceName}{Environment.NewLine}{{{Environment.NewLine}");

            return sb;
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
            return $"}}{Environment.NewLine}{Environment.NewLine}#ifdef _MSC_VER{Environment.NewLine}\t#pragma pack(pop){Environment.NewLine}#endif{Environment.NewLine}";
        }
        public string GetSectionHeader(string name)
        {
            return
                $"//---------------------------------------------------------------------------{Environment.NewLine}" +
                $"// {name}{Environment.NewLine}" +
                $"//---------------------------------------------------------------------------{Environment.NewLine}{Environment.NewLine}";
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
        #endregion

        #region BuildMethod
        public string MakeValidName(string name)
        {
            name = name.Replace(' ', '_')
                .Replace('?', '_')
                .Replace('+', '_')
                .Replace('-', '_')
                .Replace(':', '_')
                .Replace('/', '_')
                .Replace('^', '_')
                .Replace('(', '_')
                .Replace(')', '_')
                .Replace('[', '_')
                .Replace(']', '_')
                .Replace('<', '_')
                .Replace('>', '_')
                .Replace('&', '_')
                .Replace('.', '_')
                .Replace('#', '_')
                .Replace('\\', '_')
                .Replace('"', '_')
                .Replace('%', '_');

            if (string.IsNullOrEmpty(name)) return name;

            if (char.IsDigit(name[0]))
                name = '_' + name;

            return name;
        }
        public string BuildMethodSignature(SdkMethod m, SdkClass c, bool inHeader)
        {
            var text = new UftStringBuilder();

            // static
            if (m.IsStatic && inHeader && Main.GenInfo.ShouldConvertStaticMethods)
                text += "$static ";

            // Return Type
            var retn = m.Parameters.Where(item => item.ParamType == Native.Method.Parameter.Type.Return).ToList();
            text += (retn.Any() ? retn.First().CppType : $"void") + $" ";

            // inHeader
            if (!inHeader)
                text += $"{c.NameCpp}::";
            if (m.IsStatic && Main.GenInfo.ShouldConvertStaticMethods)
                text += $"STATIC_";
            text += m.Name;

            // Parameters
            text += $"(";
            if (m.Parameters.Count > 0)
            {
                var paramList = m.Parameters
                    .Where(p => p.ParamType != Native.Method.Parameter.Type.Return)
                    .OrderBy(p => p.ParamType)
                    .Select(p => (p.PassByReference ? $"const " : "") + p.CppType + (p.PassByReference ? $"& " :
                                     p.ParamType == Native.Method.Parameter.Type.Out ? $"* " : $" ") + p.Name).ToList();
                if (paramList.Count > 0)
                    text += paramList.Aggregate((cur, next) => cur + ", " + next);
            }
            text += $")";

            return text;
        }
        public string BuildMethodBody(SdkClass c, SdkMethod m)
        {
            UftStringBuilder text = new UftStringBuilder();

            // Function Pointer
            text += $"{{{Environment.NewLine}\tstatic auto fn";
            if (Main.GenInfo.ShouldUseStrings)
            {
                text += $" = UObject::FindObject<UFunction>(";

                if (Main.GenInfo.ShouldXorStrings)
                    text += $"_xor_(\"{m.FullName}\")";
                else
                    text += $"\"{m.FullName}\"";

                text += $");{Environment.NewLine}{Environment.NewLine}";
            }
            else
            {
                text += $" = UObject::GetObjectCasted<UFunction>({m.Index});{Environment.NewLine}{Environment.NewLine}";
            }

            // Parameters
            if (Main.GenInfo.ShouldGenerateFunctionParametersFile)
            {
                text += $"\t{c.NameCpp}_{m.Name}_Params params;{Environment.NewLine}";
            }
            else
            {
                text += $"\tstruct{Environment.NewLine}\t{{{Environment.NewLine}";
                foreach (var param in m.Parameters)
                    text += $"\t\t{param.CppType,-30} {param.Name};{Environment.NewLine}";
                text += $"\t}} params;{Environment.NewLine}";
            }

            var retn = m.Parameters.Where(item => item.ParamType == Native.Method.Parameter.Type.Default).ToList();
            if (retn.Any())
            {
                foreach (var param in retn)
                    text += $"\tparams.{param.Name} = {param.Name};{Environment.NewLine}";
            }
            text += $"{Environment.NewLine}";

            //Function Call
            text += $"\tauto flags = fn->FunctionFlags;{Environment.NewLine}";
            if (m.IsNative)
                text += $"\tfn->FunctionFlags |= 0x{UEFunctionFlags.Native:X};{Environment.NewLine}";
            text += $"{Environment.NewLine}";

            if (m.IsStatic && !Main.GenInfo.ShouldConvertStaticMethods)
            {
                text += $"\tstatic auto defaultObj = StaticClass()->CreateDefaultObject();{Environment.NewLine}";
                text += $"\tdefaultObj->ProcessEvent(fn, &params);{Environment.NewLine}{Environment.NewLine}";
            }
            else
            {
                text += $"\tUObject::ProcessEvent(fn, &params);{Environment.NewLine}{Environment.NewLine}";
            }
            text += $"\tfn->FunctionFlags = flags;{Environment.NewLine}";

            //Out Parameters
            var rOut = m.Parameters.Where(item => item.ParamType == Native.Method.Parameter.Type.Out).ToList();
            if (rOut.Any())
            {
                text += $"{Environment.NewLine}";
                foreach (var param in rOut)
                    text += $"\tif ({param.Name} != nullptr){Environment.NewLine}" +
                            $"\t\t*{param.Name} = params.{param.Name};{Environment.NewLine}";
            }
            text += $"{Environment.NewLine}";

	        //Return Value
            var ret = m.Parameters.Where(item => item.ParamType == Native.Method.Parameter.Type.Return).ToList();
            if (ret.Any())
                text += $"{Environment.NewLine}\treturn params.{ret.First().Name};{Environment.NewLine}";

            text += $"}}{Environment.NewLine}";

            return text;
        }
        #endregion

        #region Print
        public void PrintConstant(string fileName, SdkConstant c)
        {
            IncludeFile<UftCpp>.AppendToSdk(Main.GenInfo.SdkPath, fileName, $"#define CONST_{c.Name,-50} {c.Value}{Environment.NewLine}");
        }
        public void PrintEnum(string fileName, SdkEnum e)
        {
            UftStringBuilder text = new UftStringBuilder($"// {e.FullName}{Environment.NewLine}enum class {e.Name} : uint8_t{Environment.NewLine}{{{Environment.NewLine}");

            for (int i = 0; i < e.Values.Count; i++)
                text += $"\t{e.Values[i],-30} = {i},{Environment.NewLine}";

            text += $"{Environment.NewLine}}};{Environment.NewLine}{Environment.NewLine}";

            IncludeFile<UftCpp>.AppendToSdk(Main.GenInfo.SdkPath, fileName, text);
        }
        public void PrintStruct(string fileName, SdkScriptStruct ss)
        {
            var text = new UftStringBuilder($"// {ss.FullName}{Environment.NewLine}// ");

            if (ss.InheritedSize.ToInt32() > 0)
                text += $"0x{(ss.Size.ToInt32() - ss.InheritedSize.ToInt32()):X4} (0x{ss.Size.ToInt64():X4} - 0x{ss.InheritedSize.ToInt64():X4}){Environment.NewLine}";
            else
                text += $"0x{(long)ss.Size:X4}{Environment.NewLine}";

            text += $"{ss.NameCppFull}{Environment.NewLine}{{{Environment.NewLine}";

            //Member
            foreach (var m in ss.Members)
            {
                text += 
                    $"\t{(m.IsStatic ? "static " + m.Type : m.Type),-50} {m.Name + ";",-58} // 0x{(long)m.Offset:X4}(0x{(long)m.Size:X4})" +
                    (!string.IsNullOrEmpty(m.Comment) ? " " + m.Comment : "") +
                    (!string.IsNullOrEmpty(m.FlagsString) ? " (" + m.FlagsString + ")" : "") +
                    $"{Environment.NewLine}";
            }
            text += $"{Environment.NewLine}";

            //Predefined Methods
            if (ss.PredefinedMethods.Count > 0)
            {
                text += $"{Environment.NewLine}";
                foreach (var m in ss.PredefinedMethods)
                {
                    if (m.MethodType == Native.PredefinedMethod.Type.Inline)
                        text += m.Body;
                    else
                        text += $"\t{m.Signature};";
                    text += $"{Environment.NewLine}{Environment.NewLine}";
                }
            }
            text += $"}};{Environment.NewLine}";

            IncludeFile<UftCpp>.AppendToSdk(Main.GenInfo.SdkPath, fileName, text);
        }
        public void PrintClass(string fileName, SdkClass c)
        {
            var text = new UftStringBuilder($"// {c.FullName}{Environment.NewLine}// ");

            if (c.InheritedSize.ToInt32() > 0)
                text += $"0x{c.Size.ToInt32() - c.InheritedSize.ToInt32():X4} ({(long)c.Size:X4} - 0x{(long)c.InheritedSize:X4}){Environment.NewLine}";
            else
                text += $"0x{(long)c.Size:X4}{Environment.NewLine}";

            text += $"{c.NameCppFull}{Environment.NewLine}{{{Environment.NewLine}public:{Environment.NewLine}";

            // Member
            foreach (var m in c.Members)
            {
                text +=
                    $"\t{(m.IsStatic ? "static " + m.Type : m.Type),-50} {m.Name,-58}; // 0x{(long)m.Offset:X4}(0x{(long)m.Size:X4})" +
                    (!string.IsNullOrEmpty(m.Comment) ? " " + m.Comment : "") +
                    (!string.IsNullOrEmpty(m.FlagsString) ? " (" + m.FlagsString + ")" : "") +
                    $"{Environment.NewLine}";
            }
            text += $"{Environment.NewLine}";

            // Predefined Methods
            if (c.PredefinedMethods.Count > 0)
            {
                text += $"{Environment.NewLine}";
                foreach (var m in c.PredefinedMethods)
                {
                    if (m.MethodType == Native.PredefinedMethod.Type.Inline)
                        text += m.Body;
                    else
                        text += $"\t{m.Signature};";

                    text += $"{Environment.NewLine}{Environment.NewLine}";
                }
            }

            // Methods
            if (c.PredefinedMethods.Count > 0)
            {
                text += $"{Environment.NewLine}";
                foreach (var m in c.Methods)
                {
                    text += $"\t{BuildMethodSignature(m, new SdkClass(),  true)};{Environment.NewLine}";
                }
            }

            text += $"}};{Environment.NewLine}{Environment.NewLine}";

            IncludeFile<UftCpp>.AppendToSdk(Main.GenInfo.SdkPath, fileName, text);
        }
        #endregion

        #region SavePackage
        public override void SaveStructs(SdkPackage package)
        {
            // Create file
            string fileName = GenerateFileName(FileContentType.Structs, package.Name);

            // Init File
            IncludeFile<UftCpp>.CreateFile(Main.GenInfo.SdkPath, fileName);
            IncludeFile<UftCpp>.AppendToSdk(Main.GenInfo.SdkPath, fileName, GetFileHeader(true));

            if (package.Constants.Count > 0)
            {
                IncludeFile<UftCpp>.AppendToSdk(Main.GenInfo.SdkPath, fileName, GetSectionHeader("Constants"));
                foreach (var c in package.Constants)
                    PrintConstant(fileName, c);
            }

            if (package.Enums.Count > 0)
            {
                IncludeFile<UftCpp>.AppendToSdk(Main.GenInfo.SdkPath, fileName, GetSectionHeader("Enums"));
                foreach (var e in package.Enums)
                    PrintEnum(fileName, e);
            }

            if (package.ScriptStructs.Count > 0)
            {
                IncludeFile<UftCpp>.AppendToSdk(Main.GenInfo.SdkPath, fileName, GetSectionHeader("Script Structs"));
                foreach (var ss in package.ScriptStructs)
                    PrintStruct(fileName, ss);
            }

            IncludeFile<UftCpp>.AppendToSdk(Main.GenInfo.SdkPath, fileName, GetFileFooter());
        }
        public override void SaveClasses(SdkPackage package)
        {
            // Create file
            string fileName = GenerateFileName(FileContentType.Classes, package.Name);

            // Init File
            IncludeFile<UftCpp>.CreateFile(Main.GenInfo.SdkPath, fileName);
            IncludeFile<UftCpp>.AppendToSdk(Main.GenInfo.SdkPath, fileName, GetFileHeader(true));

            if (package.Classes.Count <= 0) return;

            IncludeFile<UftCpp>.AppendToSdk(Main.GenInfo.SdkPath, fileName, GetSectionHeader("Classes"));
            foreach (var c in package.Classes)
                PrintClass(fileName, c);

            IncludeFile<UftCpp>.AppendToSdk(Main.GenInfo.SdkPath, fileName, GetFileFooter());
        }
        public override void SaveFunctions(SdkPackage package)
        {
            if (Main.GenInfo.IsExternal)
                return;

            // Create Function Parameters File
            if (Main.GenInfo.ShouldGenerateFunctionParametersFile)
                SaveFunctionParameters(package);

            // ////////////////////////

            // Create Functions file
            string fileName = GenerateFileName(FileContentType.Functions, package.Name);

            // Init Functions File
            IncludeFile<UftCpp>.CreateFile(Main.GenInfo.SdkPath, fileName);
            IncludeFile<UftCpp>.AppendToSdk(Main.GenInfo.SdkPath, fileName, GetFileHeader(new List<string>() { "\"../SDK.h\"" }, false));

            var text = new UftStringBuilder(GetSectionHeader("Functions"));
            foreach (var s in package.ScriptStructs)
            {
                foreach (var m in s.PredefinedMethods)
                {
                    if (m.MethodType != Native.PredefinedMethod.Type.Inline)
                        text += $"{m.Body}{Environment.NewLine}{Environment.NewLine}";
                }
            }

            foreach (var c in package.Classes)
            {
                foreach (var m in c.PredefinedMethods)
                {
                    if (m.MethodType != Native.PredefinedMethod.Type.Inline)
                        text += $"{m.Body}{Environment.NewLine}{Environment.NewLine}";
                }

                foreach (var m in c.Methods)
                {
                    //Method Info
                    text += $"// {m.FullName}{Environment.NewLine}" + $"// ({m.FlagsString}){Environment.NewLine}";

                    if (m.Parameters.Count > 0)
                    {
                        text += $"// Parameters:{Environment.NewLine}";
                        foreach (var param in m.Parameters)
                            text += $"// {param.CppType,-30} {param.Name,-30} ({param.FlagsString}){Environment.NewLine}";
                    }

                    text += $"{Environment.NewLine}";
                    text += BuildMethodSignature(m, c, false) + $"{Environment.NewLine}";
                    text += BuildMethodBody(c, m) + $"{Environment.NewLine}{Environment.NewLine}";
                }
            }

            text += GetFileFooter();

            // Write the file
            IncludeFile<UftCpp>.AppendToSdk(Main.GenInfo.SdkPath, fileName, text);
        }
        public override void SaveFunctionParameters(SdkPackage package)
        {
            // Create file
            string fileName = GenerateFileName(FileContentType.FunctionParameters, package.Name);

            // Init File
            IncludeFile<UftCpp>.CreateFile(Main.GenInfo.SdkPath, fileName);
            IncludeFile<UftCpp>.AppendToSdk(Main.GenInfo.SdkPath, fileName, GetFileHeader(new List<string>() { "\"../SDK.h\"" }, true));

            // Section
            var text = new UftStringBuilder(GetSectionHeader("Parameters"));

            // Method Params
            foreach (var c in package.Classes)
            {
                foreach (var m in c.Methods)
                {
                    text += $"// {m.FullName}{Environment.NewLine}" +
                            $"struct {c.NameCpp}_{m.Name}_Params{Environment.NewLine}{{{Environment.NewLine}";

                    foreach (var param in m.Parameters)
                        text += $"\t{param.CppType,-50} {param.Name + ";",-58} // ({param.FlagsString}){Environment.NewLine}";
                    text += $"}};{Environment.NewLine}{Environment.NewLine}";
                }
            }

            text += GetFileFooter();

            // Write the file
            IncludeFile<UftCpp>.AppendToSdk(Main.GenInfo.SdkPath, fileName, text);
        }
        public override void SdkAfterFinish(List<SdkPackage> packages, List<SdkUStruct> missing)
        {
            // Copy Include File
            new BasicHeader().Process(Main.IncludePath);
            new BasicCpp().Process(Main.IncludePath);

            var text = new UftStringBuilder();
            text += $"// ------------------------------------------------{Environment.NewLine}";
            text += $"// Sdk Generated By ( Unreal Finder Tool By CorrM ){Environment.NewLine}";
            text += $"// ------------------------------------------------{Environment.NewLine}";
            text += $"{Environment.NewLine}";
            text += $"#pragma once{Environment.NewLine}{Environment.NewLine}";
            text += $"// Name: {Main.GenInfo.GameName.Trim()}, Version: {Main.GenInfo.GameVersion}{Environment.NewLine}{Environment.NewLine}";
            text += "{Environment.NewLine}";
            text += $"#include <set>{Environment.NewLine}";
            text += $"#include <string>{Environment.NewLine}";

            // Check for missing structs
            if (missing.Count > 0)
            {
                string missingText = string.Empty;

                // Init File
                IncludeFile<UftCpp>.CreateFile(Path.GetDirectoryName(Main.GenInfo.SdkPath), "MISSING.h");

                foreach (var s in missing)
                {
                    IncludeFile<UftCpp>.AppendToSdk(Path.GetDirectoryName(Main.GenInfo.SdkPath), "MISSING.h", GetFileHeader(true));

                    missingText += $"// {s.FullName}{Environment.NewLine}// ";
                    missingText += $"0x{s.PropertySize.ToInt64():X4}{Environment.NewLine}";

                    missingText += $"struct {MakeValidName(s.CppName)}{Environment.NewLine}{{{Environment.NewLine}";
                    missingText += $"\tunsigned char UnknownData[0x{s.PropertySize.ToInt64():X}];{Environment.NewLine}}};{Environment.NewLine}{Environment.NewLine}";
                }

                missingText += GetFileFooter();
                IncludeFile<UftCpp>.WriteToSdk(Path.GetDirectoryName(Main.GenInfo.SdkPath), "MISSING.h", missingText);

                // Append To Sdk Header
                text += "{Environment.NewLine}#include \"SDK/MISSING.h\"{Environment.NewLine}";
            }

            text += "{Environment.NewLine}";
            foreach (var package in packages)
            {
                text += $"#include \"SDK/{GenerateFileName(FileContentType.Structs, package.Name)}\"{Environment.NewLine}";
                text += $"#include \"SDK/{GenerateFileName(FileContentType.Classes, package.Name)}\"{Environment.NewLine}";

                if (Main.GenInfo.ShouldGenerateFunctionParametersFile)
                    text += $"#include \"SDK/{GenerateFileName(FileContentType.FunctionParameters, package.Name)}\"{Environment.NewLine}";
            }

            // Write SDK.h
            IncludeFile<UftCpp>.AppendToSdk(Path.GetDirectoryName(Main.GenInfo.SdkPath), "SDK.h", text);
        }
        #endregion
    }
}
