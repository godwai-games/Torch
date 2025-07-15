#include "GeoLayoutFactory.h"

#include "spdlog/spdlog.h"
#include "Companion.h"
#include "utils/Decompressor.h"
#include "utils/TorchUtils.h"
#include "types/RawBuffer.h"
#include <deque>

#define ALIGN8(val) (((val) + 7) & ~7)

namespace BK64 {

ExportResult GeoLayoutHeaderExporter::Export(std::ostream& write, std::shared_ptr<IParsedData> raw, std::string& entryName, YAML::Node& node, std::string* replacement) {
    const auto symbol = GetSafeNode(node, "symbol", entryName);

    if (Companion::Instance->IsOTRMode()) {
        write << "static const ALIGN_ASSET(2) char " << symbol << "[] = \"__OTR__" << (*replacement) << "\";\n\n";
        return std::nullopt;
    }

    return std::nullopt;
}

ExportResult GeoLayoutCodeExporter::Export(std::ostream& write, std::shared_ptr<IParsedData> raw, std::string& entryName, YAML::Node& node, std::string* replacement) {
    auto offset = GetSafeNode<uint32_t>(node, "offset");
    auto geo = std::static_pointer_cast<GeoLayoutData>(raw);

    return offset;
}

ExportResult BK64::GeoLayoutBinaryExporter::Export(std::ostream& write, std::shared_ptr<IParsedData> raw, std::string& entryName, YAML::Node& node, std::string* replacement) {
    auto writer = LUS::BinaryWriter();
    const auto geo = std::static_pointer_cast<GeoLayoutData>(raw);

    for(const auto& [opcode, cmdLength, arguments] : geo->mCmds) {

        writer.Write(static_cast<uint32_t>(opcode));
        writer.Write(cmdLength);

        for(auto& args : arguments) {
            switch(static_cast<GeoLayoutArgType>(args.index())) {
                case GeoLayoutArgType::U8: {
                    writer.Write(std::get<uint8_t>(args));
                    break;
                }
                case GeoLayoutArgType::S8: {
                    writer.Write(std::get<int8_t>(args));
                    break;
                }
                case GeoLayoutArgType::U16: {
                    writer.Write(std::get<uint16_t>(args));
                    break;
                }
                case GeoLayoutArgType::S16: {
                    writer.Write(std::get<int16_t>(args));
                    break;
                }
                case GeoLayoutArgType::U32: {
                    writer.Write(std::get<uint32_t>(args));
                    break;
                }
                case GeoLayoutArgType::S32: {
                    writer.Write(std::get<int32_t>(args));
                    break;
                }
                case GeoLayoutArgType::FLOAT: {
                    writer.Write(std::get<float>(args));
                    break;
                }
                default: {
                    break;
                }
            }
        }
    }

    std::vector<char> buffer = writer.ToVector();
    writer.Close();

    LUS::BinaryWriter output = LUS::BinaryWriter();
    WriteHeader(output, Torch::ResourceType::Blob, 0);

    output.Write(static_cast<uint32_t>(buffer.size()));
    output.Write(buffer.data(), buffer.size());
    output.Finish(write);
    output.Close();

    return std::nullopt;
}

ExportResult GeoLayoutModdingExporter::Export(std::ostream& write, std::shared_ptr<IParsedData> raw, std::string& entryName, YAML::Node& node, std::string* replacement) {
    auto geo = std::static_pointer_cast<GeoLayoutData>(raw);
    const auto symbol = GetSafeNode(node, "symbol", entryName);

    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << symbol;
    out << YAML::Value;
    out.SetIndent(2);
    out << YAML::BeginSeq;

    for(const auto& [opCode, cmdLength, arguments] : geo->mCmds) {
        uint32_t i = 0;
        bool addChild = false;
        out << YAML::BeginMap;

        switch(opCode) {
            case GeoLayoutOpCode::UnknownCmd0:
                out << YAML::Key << "UnknownCmd0";
                out << YAML::Value << YAML::BeginMap;
                out << YAML::Key << "childOffset" << YAML::Value << std::get<uint16_t>(arguments.at(i++));
                out << YAML::Key << "shouldRotatePitch" << YAML::Value << std::get<uint16_t>(arguments.at(i++));
                out << YAML::Key << "x" << YAML::Value << std::get<float>(arguments.at(i++));
                out << YAML::Key << "y" << YAML::Value << std::get<float>(arguments.at(i++));
                out << YAML::Key << "z" << YAML::Value << std::get<float>(arguments.at(i++));
                break;
            case GeoLayoutOpCode::Sort:
                out << YAML::Key << "Sort";
                out << YAML::Value << YAML::BeginMap;
                break;
            case GeoLayoutOpCode::Bone:
                out << YAML::Key << "Bone";
                out << YAML::Value << YAML::BeginMap;
                break;
            case GeoLayoutOpCode::LoadDL:
                out << YAML::Key << "LoadDL";
                out << YAML::Value << YAML::BeginMap;
                break;
            case GeoLayoutOpCode::Skinning:
                out << YAML::Key << "Skinning";
                out << YAML::Value << YAML::BeginMap;
                break;
            case GeoLayoutOpCode::Branch:
                out << YAML::Key << "Branch";
                out << YAML::Value << YAML::BeginMap;
                break;
            case GeoLayoutOpCode::UnknownCmd7:
                out << YAML::Key << "UnknownCmd7";
                out << YAML::Value << YAML::BeginMap;
                break;
            case GeoLayoutOpCode::LOD:
                out << YAML::Key << "LOD";
                out << YAML::Value << YAML::BeginMap;
                break;
            case GeoLayoutOpCode::ReferencePoint:
                out << YAML::Key << "ReferencePoint";
                out << YAML::Value << YAML::BeginMap;
                break;
            case GeoLayoutOpCode::Selector:
                out << YAML::Key << "Selector";
                out << YAML::Value << YAML::BeginMap;
                break;
            case GeoLayoutOpCode::DrawDistance:
                out << YAML::Key << "DrawDistance";
                out << YAML::Value << YAML::BeginMap;
                break;
            case GeoLayoutOpCode::UnknownCmdE:
                out << YAML::Key << "UnknownCmdE";
                out << YAML::Value << YAML::BeginMap;
                break;
            case GeoLayoutOpCode::UnknownCmdF:
                out << YAML::Key << "UnknownCmdF";
                out << YAML::Value << YAML::BeginMap;
                break;
            case GeoLayoutOpCode::UnknownCmd10:
                out << YAML::Key << "UnknownCmd10";
                out << YAML::Value << YAML::BeginMap;
                break;
            default:
                throw std::runtime_error("BK64::GeoLayoutModdingExporter: Unknown OpCode Found " + std::to_string(static_cast<uint32_t>(opCode)));
        }
        out << YAML::EndMap;
    }

    out << YAML::EndSeq;
    out << YAML::EndMap;

    write.write(out.c_str(), out.size());

    return std::nullopt;
}

std::optional<std::shared_ptr<IParsedData>> GeoLayoutFactory::parse(std::vector<uint8_t>& buffer, YAML::Node& node) {
    auto [_, segment] = Decompressor::AutoDecode(node, buffer);
    LUS::BinaryReader reader(segment.data, segment.size);
    reader.SetEndianness(Torch::Endianness::Big);
    const auto symbol = GetSafeNode<std::string>(node, "symbol");
    const auto offset = GetSafeNode<uint32_t>(node, "offset");

    std::vector<GeoLayoutCommand> cmds;

    std::deque<uint32_t> offsetStack;

    offsetStack.push_back(0);
    
    while (true) {
        uint32_t childCount = 0;
        std::vector<GeoLayoutArg> args;
        auto localOffset = offsetStack.back();
        reader.Seek(localOffset, LUS::SeekOffsetType::Start);
        auto opCode = reader.ReadUInt32();
        auto cmdLength = reader.ReadUInt32();

        offsetStack.back() += cmdLength;

        if (cmdLength == 0) {
            offsetStack.pop_back();
        }

        switch (static_cast<GeoLayoutOpCode>(opCode)) {
            case GeoLayoutOpCode::UnknownCmd0: {
                auto childOffset = reader.ReadUInt16();
                auto shouldRotatePitch = reader.ReadUInt16();
                auto x = reader.ReadFloat();
                auto y = reader.ReadFloat();
                auto z = reader.ReadFloat();

                args.emplace_back(childOffset);
                args.emplace_back(shouldRotatePitch);
                args.emplace_back(x);
                args.emplace_back(y);
                args.emplace_back(z);
                if (childOffset != 0) {
                    offsetStack.push_back(localOffset + childOffset);
                }
                break;
            }
            case GeoLayoutOpCode::Sort: {
                auto x1 = reader.ReadFloat();
                auto y1 = reader.ReadFloat();
                auto z1 = reader.ReadFloat();
                auto x2 = reader.ReadFloat();
                auto y2 = reader.ReadFloat();
                auto z2 = reader.ReadFloat();

                reader.ReadUByte(); // pad
                auto layoutOrder = reader.ReadUByte();
                auto firstChildOffset = reader.ReadUInt16();
                reader.ReadUInt16(); // pad
                auto secondChildOffset = reader.ReadUInt16();

                args.emplace_back(x1);
                args.emplace_back(y1);
                args.emplace_back(z1);
                args.emplace_back(x2);
                args.emplace_back(y2);
                args.emplace_back(z2);
                args.emplace_back(layoutOrder);
                args.emplace_back(firstChildOffset);
                args.emplace_back(secondChildOffset);

                if (firstChildOffset != 0) {
                    offsetStack.push_back(localOffset + firstChildOffset);
                }
                if (secondChildOffset != 0) {
                    offsetStack.push_back(localOffset + secondChildOffset);
                }
                break;
            }
            case GeoLayoutOpCode::Bone: {
                auto childOffset = reader.ReadUByte();
                auto boneId = reader.ReadUByte();
                auto unkBoneInfo = reader.ReadUInt16();

                args.emplace_back(childOffset);
                args.emplace_back(boneId);
                args.emplace_back(unkBoneInfo);

                if (childOffset != 0) {
                    offsetStack.push_back(localOffset + childOffset);
                }
                break;
            }
            case GeoLayoutOpCode::LoadDL: {
                auto dlIndex = reader.ReadUInt16();
                auto triCount = reader.ReadUInt16();
                args.emplace_back(dlIndex);
                args.emplace_back(triCount);
                break;
            }
            case GeoLayoutOpCode::Skinning: {
                auto dlOffsetPreviousBone = reader.ReadUInt16();

                args.emplace_back(dlOffsetPreviousBone);

                while (true) {
                    auto dlOffset = reader.ReadUInt16();
                    if (dlOffset == 0) {
                        break;
                    }
                    args.emplace_back(dlOffset);
                }
                break;
            }
            case GeoLayoutOpCode::Branch: {
                auto cmdTargetOffset = reader.ReadUInt32();

                args.emplace_back(cmdTargetOffset);
                break;
            }
            case GeoLayoutOpCode::UnknownCmd7: {
                reader.ReadUInt16(); // pad
                auto dlIndex = reader.ReadUInt16();

                args.emplace_back(dlIndex);
                break;
            }
            case GeoLayoutOpCode::LOD: {
                auto maxDistance = reader.ReadFloat();
                auto minDistance = reader.ReadFloat();
                auto x = reader.ReadFloat();
                auto y = reader.ReadFloat();
                auto z = reader.ReadFloat();
                auto childLayoutOffset = reader.ReadUInt32();
                args.emplace_back(maxDistance);
                args.emplace_back(minDistance);
                args.emplace_back(x);
                args.emplace_back(y);
                args.emplace_back(z);
                args.emplace_back(childLayoutOffset);
                if (childLayoutOffset != 0) {
                    offsetStack.push_back(localOffset + childLayoutOffset);
                }
                break;
            }
            case GeoLayoutOpCode::ReferencePoint: {
                auto referencePointIndex = reader.ReadUInt16();
                auto boneIndex = reader.ReadUInt16();
                auto boneOffsetX = reader.ReadFloat();
                auto boneOffsetY = reader.ReadFloat();
                auto boneOffsetZ = reader.ReadFloat();

                args.emplace_back(referencePointIndex);
                args.emplace_back(boneIndex);
                args.emplace_back(boneOffsetX);
                args.emplace_back(boneOffsetY);
                args.emplace_back(boneOffsetZ);
                break;
            }
            case GeoLayoutOpCode::Selector: {
                auto childCount = reader.ReadUInt16();
                auto selectorIndex = reader.ReadUInt16();

                args.emplace_back(childCount);
                args.emplace_back(selectorIndex);

                for (uint16_t i = 0; i < childCount; i++) {
                    auto childOffset = reader.ReadUInt32();

                    args.emplace_back(childOffset);
                    if (childOffset != 0) {
                        offsetStack.push_back(localOffset + childOffset);
                    }
                }
                break;
            }
            case GeoLayoutOpCode::DrawDistance: {
                auto negX = reader.ReadInt16();
                auto negY = reader.ReadInt16();
                auto negZ = reader.ReadInt16();
                auto posX = reader.ReadInt16();
                auto posY = reader.ReadInt16();
                auto posZ = reader.ReadInt16();
                auto unk14 = reader.ReadInt16();
                auto unk16 = reader.ReadInt16();

                args.emplace_back(negX);
                args.emplace_back(negY);
                args.emplace_back(negZ);
                args.emplace_back(posX);
                args.emplace_back(posY);
                args.emplace_back(posZ);
                args.emplace_back(unk14);
                args.emplace_back(unk16);
                break;
            }
            case GeoLayoutOpCode::UnknownCmdE: {
                auto coords1X = reader.ReadInt16();
                auto coords1Y = reader.ReadInt16();
                auto coords1Z = reader.ReadInt16();
                auto coords2X = reader.ReadInt16();
                auto coords2Y = reader.ReadInt16();
                auto coords2Z = reader.ReadInt16();

                args.emplace_back(coords1X);
                args.emplace_back(coords1Y);
                args.emplace_back(coords1Z);
                args.emplace_back(coords2X);
                args.emplace_back(coords2Y);
                args.emplace_back(coords2Z);
                break;
            }
            case GeoLayoutOpCode::UnknownCmdF: {
                auto childOffset = reader.ReadUInt16();
                auto unkA = reader.ReadUByte();
                auto unkB = reader.ReadUByte();
                for (int32_t i = 0; i < 12; i++) {
                    auto unkCBuf = reader.ReadUByte();
                    args.emplace_back(unkCBuf);
                }
                if (childOffset != 0) {
                    offsetStack.push_back(localOffset + childOffset);
                }
                break;
            }
            case GeoLayoutOpCode::UnknownCmd10: {
                auto wrapMode = reader.ReadInt32();
                args.emplace_back(wrapMode);
                break;
            }
            default:
                throw std::runtime_error("BK64::GeoLayoutFactory: Unknown OpCode Found " + std::to_string(opCode));
        }
        cmds.emplace_back(static_cast<GeoLayoutOpCode>(opCode), cmdLength, args);

        if (offsetStack.size() == 0) {
            break;
        }
    }

    return std::make_shared<GeoLayoutData>(cmds);
}

} // namespace BK64
