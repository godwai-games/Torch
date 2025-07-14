#include "ModelFactory.h"

#include "spdlog/spdlog.h"
#include "Companion.h"
#include "utils/Decompressor.h"
#include "utils/TorchUtils.h"
#include "types/RawBuffer.h"

#define BK64_MODEL_HEADER 0xB
#define SEGMENT_OFFSET(segment, offset) ((((segment) & 0xFF) << 0x18) | ((offset) & 0x00FFFFFF))
#define TEXTURE_HEADER_SIZE 0x8
#define TEXTURE_METADATA_SIZE 0x10
#define GFX_HEADER_SIZE 0x8
#define GFX_CMD_SIZE 0x8
#define VTX_HEADER_SIZE 0x18

namespace BK64 {

static const std::unordered_map<std::string, uint8_t> gF3DTable = {
    { "G_VTX", 0x04 },
    { "G_DL", 0x06 },
    { "G_MTX", 0x1 },
    { "G_ENDDL", 0xB8 },
    { "G_SETTIMG", 0xFD },
    { "G_MOVEMEM", 0x03 },
    { "G_MV_L0", 0x86 },
    { "G_MV_L1", 0x88 },
    { "G_MV_LIGHT", 0xA },
    { "G_TRI2", 0xB1 },
    { "G_QUAD", -1 }
};

static const std::unordered_map<std::string, uint8_t> gF3DExTable = {
    { "G_VTX", 0x04 },
    { "G_DL", 0x06 },
    { "G_MTX", 0x1 },
    { "G_ENDDL", 0xB8 },
    { "G_SETTIMG", 0xFD },
    { "G_MOVEMEM", 0x03 },
    { "G_MV_L0", 0x86 },
    { "G_MV_L1", 0x88 },
    { "G_MV_LIGHT", 0xA },
    { "G_TRI2", 0xB1 },
    { "G_QUAD", 0xB5 }
};

static const std::unordered_map<std::string, uint8_t> gF3DEx2Table = {
    { "G_VTX", 0x01 },
    { "G_DL", 0xDE },
    { "G_MTX", 0xDA },
    { "G_ENDDL", 0xDF },
    { "G_SETTIMG", 0xFD },
    { "G_MOVEMEM", 0xDC },
    { "G_MV_L0", 0x86 },
    { "G_MV_L1", 0x88 },
    { "G_MV_LIGHT", 0xA },
    { "G_TRI2", 0x06 },
    { "G_QUAD", 0x07 }
};

static const std::unordered_map<GBIVersion, std::unordered_map<std::string, uint8_t>> gGBITable = {
    { GBIVersion::f3d, gF3DTable },
    { GBIVersion::f3dex, gF3DExTable },
    { GBIVersion::f3dex2, gF3DEx2Table },
};

#define GBI(cmd) gGBITable.at(Companion::Instance->GetGBIVersion()).at(#cmd)

ExportResult ModelHeaderExporter::Export(std::ostream& write, std::shared_ptr<IParsedData> raw, std::string& entryName, YAML::Node& node, std::string* replacement) {
    const auto symbol = GetSafeNode(node, "symbol", entryName);

    if (Companion::Instance->IsOTRMode()) {
        write << "static const ALIGN_ASSET(2) char " << symbol << "[] = \"__OTR__" << (*replacement) << "\";\n\n";
        return std::nullopt;
    }

    return std::nullopt;
}

ExportResult ModelCodeExporter::Export(std::ostream& write, std::shared_ptr<IParsedData> raw, std::string& entryName, YAML::Node& node, std::string* replacement) {
    auto offset = GetSafeNode<uint32_t>(node, "offset");
    auto model = std::static_pointer_cast<ModelData>(raw);

    return offset;
}

ExportResult BK64::ModelBinaryExporter::Export(std::ostream& write, std::shared_ptr<IParsedData> raw, std::string& entryName, YAML::Node& node, std::string* replacement) {
    auto writer = LUS::BinaryWriter();
    const auto model = std::static_pointer_cast<ModelData>(raw);

    
    return std::nullopt;
}

std::optional<std::shared_ptr<IParsedData>> ModelFactory::parse(std::vector<uint8_t>& buffer, YAML::Node& node) {
    auto [_, segment] = Decompressor::AutoDecode(node, buffer);
    LUS::BinaryReader reader(segment.data, segment.size);
    reader.SetEndianness(Torch::Endianness::Big);
    const auto symbol = GetSafeNode<std::string>(node, "symbol");
    const auto modelOffset = GetSafeNode<uint32_t>(node, "offset"); // Should always be 0 in reality
    const auto modelOffsetEnd = modelOffset + segment.size;
    const auto fileOffset = Companion::Instance->GetCurrentVRAM().value().offset;

    if (reader.ReadInt32() != BK64_MODEL_HEADER) {
        SPDLOG_ERROR("Invalid Header For BK64 Model {}", symbol);
        return std::nullopt;
    }

    
    auto geoLayoutOffset = reader.ReadUInt32();
    auto textureSetupOffset = reader.ReadUInt16();
    auto geoType = reader.ReadUInt16();
    auto displayListSetupOffset = reader.ReadUInt32();
    auto vertexSetupOffset = reader.ReadUInt32();
    auto unkHitboxInfo = reader.ReadUInt32(); // pad?
    auto animationSetupOffset = reader.ReadUInt32();
    auto collisionSetupOffset = reader.ReadUInt32();
    auto effectsSetupEndOffset = reader.ReadUInt32();
    auto effectsSetupOffset = reader.ReadUInt32();
    reader.ReadUInt32(); // pad
    auto animatedTextureOffset = reader.ReadUInt32();
    auto triCount = reader.ReadUInt16();
    auto vertCount = reader.ReadUInt16();

    Companion::Instance->SetCompressedSegment(1, fileOffset, modelOffset + vertexSetupOffset + VTX_HEADER_SIZE);
    Companion::Instance->SetCompressedSegment(3, fileOffset, modelOffset + displayListSetupOffset + GFX_HEADER_SIZE);
    
    if (geoLayoutOffset != 0 && false) {
        YAML::Node geoLayout;
        geoLayout["type"] = "BK64:GEO_LAYOUT";
        geoLayout["offset"] = geoLayoutOffset;
        geoLayout["offset_end"] = (uint32_t)segment.size;
        geoLayout["symbol"] = symbol + "_GEO";
        Companion::Instance->AddAsset(geoLayout);
    }

    if (textureSetupOffset != 0) {

        reader.Seek(textureSetupOffset, LUS::SeekOffsetType::Start);

        auto textureDataSize = reader.ReadUInt32();
        auto textureCount = reader.ReadUInt16();
        reader.ReadUInt16(); // pad

        Companion::Instance->SetCompressedSegment(2, fileOffset, modelOffset + textureSetupOffset + TEXTURE_HEADER_SIZE + textureCount * TEXTURE_METADATA_SIZE);
        
        for (uint16_t i = 0; i < textureCount; i++) {
            auto textureDataOffset = reader.ReadUInt32();
            auto textureType = reader.ReadUInt16();
            reader.ReadUInt16(); // pad
            uint32_t width = reader.ReadUByte();
            uint32_t height = reader.ReadUByte();
            reader.ReadUInt16(); // pad
            reader.ReadUInt32(); // pad

            std::string format;
            std::string ctype;
            uint32_t tlutSize = 0;
            
            YAML::Node texture;
            switch (textureType) {
                case 0x1: {
                    uint32_t tlutOffset = modelOffset + textureSetupOffset + TEXTURE_HEADER_SIZE + textureCount * TEXTURE_METADATA_SIZE + textureDataOffset;
                    YAML::Node tlut;
                    tlut["type"] = "TEXTURE";
                    tlut["format"] = "TLUT";
                    tlut["ctype"] = "u16";
                    tlut["colors"] = 0x10;
                    tlut["offset"] = tlutOffset;
                    tlut["symbol"] = symbol + "_TLUT_" + std::to_string(i);
                    Companion::Instance->AddAsset(tlut);
                    texture["format"] = "CI4";
                    texture["ctype"] = "u8";
                    texture["tlut"] = tlutOffset;
                    tlutSize = 0x10;
                    break;
                }
                case 0x2: {
                    uint32_t tlutOffset = modelOffset + textureSetupOffset + TEXTURE_HEADER_SIZE + textureCount * TEXTURE_METADATA_SIZE + textureDataOffset;
                    YAML::Node tlut;
                    tlut["type"] = "TEXTURE";
                    tlut["format"] = "TLUT";
                    tlut["ctype"] = "u16";
                    tlut["colors"] = 0x100;
                    tlut["offset"] = tlutOffset;
                    tlut["symbol"] = symbol + "_TLUT_" + std::to_string(i);
                    Companion::Instance->AddAsset(tlut);
                    texture["format"] = "CI8";
                    texture["ctype"] = "u8";
                    texture["tlut"] = tlutOffset;
                    tlutSize = 0x100;
                    break;
                }
                case 0x4:
                    texture["format"] = "RGBA16";
                    texture["ctype"] = "u16";
                    break;
                case 0x8:
                    texture["format"] = "RGBA32";
                    texture["ctype"] = "u32";
                    break;
                case 0x10:
                    texture["format"] = "IA8";
                    texture["ctype"] = "u8";
                    break;
                default:
                    throw std::runtime_error("BK64::ModelFactory: Invalid Texture Format Found " + std::to_string(textureType));
            }

            uint32_t textureOffset = modelOffset + textureSetupOffset + TEXTURE_HEADER_SIZE + textureCount * TEXTURE_METADATA_SIZE + textureDataOffset + tlutSize * sizeof(int16_t);
            texture["type"] = "TEXTURE";
            texture["width"] = width;
            texture["height"] = height;
            texture["offset"] = textureOffset;
            texture["symbol"] = symbol + "_TEX_" + std::to_string(i);
            Companion::Instance->AddAsset(texture);
        }
    }

    if (vertexSetupOffset != 0) {
        reader.Seek(vertexSetupOffset, LUS::SeekOffsetType::Start);

        auto minCoordsX = reader.ReadUInt16();
        auto minCoordsY = reader.ReadUInt16();
        auto minCoordsZ = reader.ReadUInt16();
        auto maxCoordsX = reader.ReadUInt16();
        auto maxCoordsY = reader.ReadUInt16();
        auto maxCoordsZ = reader.ReadUInt16();
        auto centerCoordsX = reader.ReadUInt16();
        auto centerCoordsY = reader.ReadUInt16();
        auto centerCoordsZ = reader.ReadUInt16();

        auto doubleVtxCount = reader.ReadUInt16();
        auto largestDistToOrigin = reader.ReadUInt16();

        YAML::Node vtx;
        vtx["type"] = "VTX";
        vtx["count"] = doubleVtxCount / 2;
        vtx["offset"] = modelOffset + vertexSetupOffset + VTX_HEADER_SIZE;
        vtx["symbol"] = symbol + "_VTX";
        Companion::Instance->AddAsset(vtx);
    }

    if (displayListSetupOffset != 0) {
        reader.Seek(displayListSetupOffset, LUS::SeekOffsetType::Start);
        auto dlCount = reader.ReadUInt32();
        reader.ReadUInt32(); // checksum?

        std::vector<uint32_t> dlOffsets;
        uint32_t dlOffset = 0;
        dlOffsets.emplace_back(dlOffset);
        while (dlOffset < dlCount * GFX_CMD_SIZE) {
            auto w0 = reader.ReadUInt32();
            reader.ReadUInt32();
            dlOffset += GFX_CMD_SIZE;
            uint8_t opCode = w0 >> 24;
            
            if (opCode == GBI(G_ENDDL) && dlOffset != dlCount * GFX_CMD_SIZE) {
                dlOffsets.emplace_back(dlOffset);
            }
        }

        uint32_t count = 0;
        for (const auto& extractOffset : dlOffsets) {
            YAML::Node gfxNode;

            gfxNode["type"] = "GFX";
            gfxNode["offset"] = modelOffset + displayListSetupOffset + GFX_HEADER_SIZE + extractOffset;
            gfxNode["symbol"] = symbol + "_GFX_" + std::to_string(count);
            Companion::Instance->AddAsset(gfxNode);
            count++;
        }
    }



    return std::make_shared<ModelData>();
}

} // namespace BK64
