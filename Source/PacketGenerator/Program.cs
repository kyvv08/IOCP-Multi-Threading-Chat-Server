using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Xml;

namespace PacketGenerator;

public class MemberInfo
{
    public string Name { get; set; } = "";
    public string Type { get; set; } = "";
}

public class ListInfo
{
    public string Name { get; set; } = "";
    public List<MemberInfo> Members { get; set; } = new();
}

public class PacketInfo
{
    public string Name { get; set; } = "";
    public int Id { get; set; }
    public List<MemberInfo> Members { get; set; } = new();
    public List<ListInfo> Lists { get; set; } = new();
}

public class Program
{
    private static readonly Dictionary<string, string> TypeMapping = new()
    {
        { "bool", "bool" },
        { "int8", "int8" },
        { "int16", "int16" },
        { "int32", "int32" },
        { "int64", "int64" },
        { "uint8", "uint8" },
        { "uint16", "uint16" },
        { "uint32", "uint32" },
        { "uint64", "uint64" },
        { "float", "float" },
        { "double", "double" },
        { "string", "std::string" }
    };

    public static void Main(string[] args)
    {
        string pdlPath = args.Length > 0 ? args[0] : "PDL.xml";
        if (!File.Exists(pdlPath))
        {
            if (File.Exists("Source/PacketGenerator/PDL.xml"))
                pdlPath = "Source/PacketGenerator/PDL.xml";
            else if (File.Exists("../Source/PacketGenerator/PDL.xml"))
                pdlPath = "../Source/PacketGenerator/PDL.xml";
            else if (File.Exists("../../Source/PacketGenerator/PDL.xml"))
                pdlPath = "../../Source/PacketGenerator/PDL.xml";
        }

        Console.WriteLine($"[PacketGenerator] Loading PDL from: {Path.GetFullPath(pdlPath)}");

        XmlDocument doc = new XmlDocument();
        doc.Load(pdlPath);

        List<PacketInfo> packets = new List<PacketInfo>();
        XmlNodeList? packetNodes = doc.SelectNodes("//Packet");

        if (packetNodes != null)
        {
            foreach (XmlNode packetNode in packetNodes)
            {
                if (packetNode.Attributes == null) continue;

                PacketInfo pkt = new PacketInfo
                {
                    Name = packetNode.Attributes["name"]?.Value ?? "",
                    Id = int.Parse(packetNode.Attributes["id"]?.Value ?? "0")
                };

                foreach (XmlNode child in packetNode.ChildNodes)
                {
                    if (child.Name == "Member" && child.Attributes != null)
                    {
                        pkt.Members.Add(new MemberInfo
                        {
                            Name = child.Attributes["name"]?.Value ?? "",
                            Type = child.Attributes["type"]?.Value ?? ""
                        });
                    }
                    else if (child.Name == "List" && child.Attributes != null)
                    {
                        ListInfo listInfo = new ListInfo
                        {
                            Name = child.Attributes["name"]?.Value ?? ""
                        };

                        foreach (XmlNode listChild in child.ChildNodes)
                        {
                            if (listChild.Name == "Member" && listChild.Attributes != null)
                            {
                                listInfo.Members.Add(new MemberInfo
                                {
                                    Name = listChild.Attributes["name"]?.Value ?? "",
                                    Type = listChild.Attributes["type"]?.Value ?? ""
                                });
                            }
                        }

                        pkt.Lists.Add(listInfo);
                    }
                }

                packets.Add(pkt);
            }
        }

        // 1. Packet ID Enum
        StringBuilder enumSb = new StringBuilder();
        foreach (var pkt in packets)
        {
            enumSb.AppendLine(string.Format(PacketFormat.PacketIdMember, pkt.Name, pkt.Id));
        }

        // 2. Struct Definitions
        StringBuilder structsSb = new StringBuilder();
        foreach (var pkt in packets)
        {
            structsSb.AppendLine(GeneratePacketStruct(pkt));
        }

        // 3. Generate PacketProtocol.h (All Structs)
        string protocolContent = string.Format(PacketFormat.ProtocolHeaderFormat, enumSb.ToString(), structsSb.ToString());
        WriteToTargetDirs("PacketProtocol.h", protocolContent);

        // 4. Server Packet Handler (Handles C_...)
        GenerateHandlerFile(packets, "ServerPacketHandler", "C_", "ServerPacketHandler.h");

        // 5. Client Packet Handler (Handles S_...)
        GenerateHandlerFile(packets, "ClientPacketHandler", "S_", "ClientPacketHandler.h");

        Console.WriteLine("[PacketGenerator] Code Generation Completed Successfully!");
    }

    private static string GeneratePacketStruct(PacketInfo pkt)
    {
        StringBuilder memberDecl = new StringBuilder();
        StringBuilder readCode = new StringBuilder();
        StringBuilder writeCode = new StringBuilder();

        // Lists Struct Definitions
        foreach (var list in pkt.Lists)
        {
            string structName = FirstCharToUpper(list.Name) + "Info";
            memberDecl.AppendLine($"\t\tstruct {structName}");
            memberDecl.AppendLine("\t\t{");
            foreach (var m in list.Members)
            {
                memberDecl.AppendLine($"\t\t\t{TypeMapping[m.Type]} {m.Name}{{}};");
            }
            memberDecl.AppendLine("\t\t};");
            memberDecl.AppendLine($"\t\tstd::vector<{structName}> {list.Name};");
        }

        // Normal Members
        foreach (var m in pkt.Members)
        {
            if (m.Type == "string")
                memberDecl.AppendLine($"\t\tstd::string {m.Name};");
            else
                memberDecl.AppendLine($"\t\t{TypeMapping[m.Type]} {m.Name}{{}};");
        }

        // Read Code
        foreach (var m in pkt.Members)
        {
            if (m.Type == "string")
                readCode.AppendLine($"\t\t\tif (reader.ReadString({m.Name}) == false) return false;");
            else
                readCode.AppendLine($"\t\t\tif (reader.Read({m.Name}) == false) return false;");
        }

        foreach (var list in pkt.Lists)
        {
            string countVar = $"{list.Name}Count";
            readCode.AppendLine($"\t\t\tuint16 {countVar} = 0;");
            readCode.AppendLine($"\t\t\tif (reader.Read({countVar}) == false) return false;");
            readCode.AppendLine($"\t\t\t{list.Name}.resize({countVar});");
            readCode.AppendLine($"\t\t\tfor (uint16 i = 0; i < {countVar}; ++i)");
            readCode.AppendLine("\t\t\t{");
            foreach (var lm in list.Members)
            {
                if (lm.Type == "string")
                    readCode.AppendLine($"\t\t\t\tif (reader.ReadString({list.Name}[i].{lm.Name}) == false) return false;");
                else
                    readCode.AppendLine($"\t\t\t\tif (reader.Read({list.Name}[i].{lm.Name}) == false) return false;");
            }
            readCode.AppendLine("\t\t\t}");
        }

        // Write Code
        foreach (var m in pkt.Members)
        {
            if (m.Type == "string")
                writeCode.AppendLine($"\t\t\tif (writer.WriteString({m.Name}) == false) return nullptr;");
            else
                writeCode.AppendLine($"\t\t\tif (writer.Write({m.Name}) == false) return nullptr;");
        }

        foreach (var list in pkt.Lists)
        {
            string countVar = $"{list.Name}Count";
            writeCode.AppendLine($"\t\t\tuint16 {countVar} = static_cast<uint16>({list.Name}.size());");
            writeCode.AppendLine($"\t\t\tif (writer.Write({countVar}) == false) return nullptr;");
            writeCode.AppendLine($"\t\t\tfor (const auto& item : {list.Name})");
            writeCode.AppendLine("\t\t\t{");
            foreach (var lm in list.Members)
            {
                if (lm.Type == "string")
                    writeCode.AppendLine($"\t\t\t\tif (writer.WriteString(item.{lm.Name}) == false) return nullptr;");
                else
                    writeCode.AppendLine($"\t\t\t\tif (writer.Write(item.{lm.Name}) == false) return nullptr;");
            }
            writeCode.AppendLine("\t\t\t}");
        }

        return string.Format(PacketFormat.PacketStruct, pkt.Name, memberDecl.ToString(), readCode.ToString(), writeCode.ToString());
    }

    private static void GenerateHandlerFile(List<PacketInfo> packets, string className, string prefixFilter, string outputFileName)
    {
        StringBuilder handlerDecls = new StringBuilder();
        StringBuilder dispatchCases = new StringBuilder();

        foreach (var pkt in packets)
        {
            if (pkt.Name.StartsWith(prefixFilter))
            {
                handlerDecls.AppendLine(string.Format(PacketFormat.HandlerDeclaration, pkt.Name));
                dispatchCases.AppendLine(string.Format(PacketFormat.DispatchCase, pkt.Name));
            }
        }

        string fullContent = string.Format(PacketFormat.HandlerHeaderFormat, outputFileName, handlerDecls.ToString(), dispatchCases.ToString(), className);
        WriteToTargetDirs(outputFileName, fullContent);
    }

    private static void WriteToTargetDirs(string fileName, string content)
    {
        List<string> targetDirs = new List<string>
        {
            Path.Combine("Source", "Common", "Protocol")
        };

        foreach (var dir in targetDirs)
        {
            Directory.CreateDirectory(dir);
            string fullPath = Path.Combine(dir, fileName);
            File.WriteAllText(fullPath, content, Encoding.UTF8);
            Console.WriteLine($"  -> Generated: {fullPath}");
        }
    }

    private static string FirstCharToUpper(string input)
    {
        if (string.IsNullOrEmpty(input)) return input;
        return char.ToUpper(input[0]) + input.Substring(1);
    }
}
