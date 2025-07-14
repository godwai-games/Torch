#include "GeoLayoutFactory.h"

#include "spdlog/spdlog.h"
#include "Companion.h"
#include "utils/Decompressor.h"
#include "utils/TorchUtils.h"
#include "types/RawBuffer.h"

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

    for(auto& [opcode, arguments] : geo->mCmds) {

        writer.Write(static_cast<uint32_t>(opcode));

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

std::optional<std::shared_ptr<IParsedData>> GeoLayoutFactory::parse(std::vector<uint8_t>& buffer, YAML::Node& node) {
    auto [_, segment] = Decompressor::AutoDecode(node, buffer);
    LUS::BinaryReader reader(segment.data, segment.size);
    reader.SetEndianness(Torch::Endianness::Big);
    const auto symbol = GetSafeNode<std::string>(node, "symbol");
    const auto offset = GetSafeNode<std::string>(node, "offset");
    const auto endOffset = GetSafeNode<std::string>(node, "end_offset");
    std::vector<GeoLayoutCommand> cmds;

    auto rollingOffset = offset;
    uint32_t extraCommandsToProcess = 0;
    
    while (true) {
        GeoLayoutCommand cmd;
        std::vector<GeoLayoutArg> args;
        auto opCode = reader.ReadUInt32();

        switch (static_cast<GeoLayoutOpCode>(opCode)) {
            case GeoLayoutOpCode::Sort: {
                auto cmdLength = reader.ReadUInt32();
                float x1 = reader.ReadFloat();
                float y1 = reader.ReadFloat();
                float z1 = reader.ReadFloat();
                float x2 = reader.ReadFloat();
                float y2 = reader.ReadFloat();
                float z2 = reader.ReadFloat();

                reader.ReadUByte(); // pad
                auto layoutOrder = reader.ReadUByte();
                auto firstChildOffset = reader.ReadUInt16();
                reader.ReadUInt16(); // pad
                auto secondChildOffset = reader.ReadUInt16();
                args.emplace_back(cmdLength);
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
                    extraCommandsToProcess++;
                }
                if (secondChildOffset != 0) {
                    extraCommandsToProcess++;
                }

                rollingOffset += cmdLength;
                break;
            }
            case GeoLayoutOpCode::Bone: {
                reader.ReadUInt16(); // pad
                auto dlIndex = reader.ReadUInt16();
                auto cmdLength = reader.ReadUByte();
                auto boneId = reader.ReadUByte();
                auto unkBoneInfo = reader.ReadUInt16();

                args.emplace_back(dlIndex);
                args.emplace_back(cmdLength);
                args.emplace_back(boneId);
                args.emplace_back(unkBoneInfo);
                if (cmdLength == 0x10) {
                    reader.ReadUInt32(); // pad
                }

                rollingOffset += cmdLength;
                break;
            }
            case GeoLayoutOpCode::LoadDL: {
                auto cmdLength = reader.ReadUInt32();
                auto dlIndex = reader.ReadUInt16();
                auto triCount = reader.ReadUInt16();
                args.emplace_back(cmdLength);
                args.emplace_back(dlIndex);
                args.emplace_back(triCount);
                if (cmdLength == 0x10) {
                    reader.ReadUInt32(); // pad
                }

                rollingOffset += cmdLength;
                break;
            }
            case GeoLayoutOpCode::Skinning: {
                auto cmdLength = reader.ReadUInt32();
                auto dlOffsetPreviousBone = reader.ReadUInt16();

                args.emplace_back(cmdLength);
                args.emplace_back(dlOffsetPreviousBone);

                while (true) {
                    auto dlOffset = reader.ReadUInt16();
                    if (dlOffset == 0) {
                        break;
                    }
                    args.emplace_back(dlOffset);
                }

                rollingOffset += cmdLength;
                break;
            }
            case GeoLayoutOpCode::Branch: {
                auto cmdLength = reader.ReadUInt32();
                auto cmdTargetOffset = reader.ReadUInt32();
                reader.ReadUInt32(); // pad

                args.emplace_back(cmdLength);
                args.emplace_back(cmdTargetOffset);
                rollingOffset += cmdLength;
                break;
            }
            case GeoLayoutOpCode::LOD: {
                auto cmdLength = reader.ReadUInt32();
                auto maxDistance = reader.ReadFloat();
                auto minDistance = reader.ReadFloat();
                auto x = reader.ReadFloat();
                auto y = reader.ReadFloat();
                auto z = reader.ReadFloat();
                auto childLayoutOffset = reader.ReadUInt32();
                if (childLayoutOffset != 0) {
                    extraCommandsToProcess++;
                }
                args.emplace_back(cmdLength);
                args.emplace_back(maxDistance);
                args.emplace_back(minDistance);
                args.emplace_back(x);
                args.emplace_back(y);
                args.emplace_back(z);
                args.emplace_back(childLayoutOffset);
                rollingOffset += cmdLength;
                break;
            }
            case GeoLayoutOpCode::ReferencePoint: {
                auto cmdLength = reader.ReadUInt32();
                auto referencePointIndex = reader.ReadUInt16();
                auto boneIndex = reader.ReadUInt16();
                auto boneOffsetX = reader.ReadFloat();
                auto boneOffsetY = reader.ReadFloat();
                auto boneOffsetZ = reader.ReadFloat();
                args.emplace_back(cmdLength);
                args.emplace_back(referencePointIndex);
                args.emplace_back(boneIndex);
                args.emplace_back(boneOffsetX);
                args.emplace_back(boneOffsetY);
                args.emplace_back(boneOffsetZ);
                rollingOffset += cmdLength;
                break;
            }
            case GeoLayoutOpCode::Selector: {
                auto cmdLength = reader.ReadUInt32();
                auto childCount = reader.ReadUInt16();
                auto selectorIndex = reader.ReadUInt16();

                args.emplace_back(cmdLength);
                args.emplace_back(childCount);
                args.emplace_back(selectorIndex);

                for (uint16_t i = 0; i < childCount; i++) {
                    auto childOffset = reader.ReadUInt32();

                    args.emplace_back(childOffset);
                }

                extraCommandsToProcess += childCount;
                rollingOffset += cmdLength;
                break;
            }
            case GeoLayoutOpCode::DrawDistance: {
                auto cmdLength = reader.ReadUInt32();
                auto negX = reader.ReadInt16();
                auto negY = reader.ReadInt16();
                auto negZ = reader.ReadInt16();
                auto posX = reader.ReadInt16();
                auto posY = reader.ReadInt16();
                auto posZ = reader.ReadInt16();
                auto unk14 = reader.ReadInt16();
                auto unk16 = reader.ReadInt16();


                args.emplace_back(cmdLength);
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
            case GeoLayoutOpCode::UnknownCmd: {
                auto cmdLength = reader.ReadUInt32();
                auto coords1X = reader.ReadInt16();
                auto coords1Y = reader.ReadInt16();
                auto coords1Z = reader.ReadInt16();
                auto coords2X = reader.ReadInt16();
                auto coords2Y = reader.ReadInt16();
                auto coords2Z = reader.ReadInt16();
                reader.ReadUInt32(); // pad

                args.emplace_back(cmdLength);
                args.emplace_back(coords1X);
                args.emplace_back(coords1Y);
                args.emplace_back(coords1Z);
                args.emplace_back(coords2X);
                args.emplace_back(coords2Y);
                args.emplace_back(coords2Z);
                break;
            }
            default:
                throw std::runtime_error("BK64::GeoLayoutFactory: Unknown OpCode Found " + std::to_string(opCode));
        }

        cmds.emplace_back(static_cast<GeoLayoutOpCode>(opCode), args);

        if (extraCommandsToProcess > 0) {
            extraCommandsToProcess--;
        } else {
            if (rollingOffset >= endOffset) {
                break;
            }
        }
    }

    return std::make_shared<GeoLayoutData>(cmds);
}

} // namespace BK64
