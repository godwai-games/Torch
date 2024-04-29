#include "LimbFactory.h"
#include "spdlog/spdlog.h"

#include "Companion.h"
#include "utils/Decompressor.h"
#include "utils/TorchUtils.h"
#include <regex>

ExportResult Z64::LimbHeaderExporter::Export(std::ostream &write, std::shared_ptr<IParsedData> raw, std::string& entryName, YAML::Node &node, std::string* replacement) {
    const auto symbol = GetSafeNode(node, "symbol", entryName);

    if(Companion::Instance->IsOTRMode()){
        write << "static const char " << symbol << "[] = \"__OTR__" << (*replacement) << "\";\n\n";
        return std::nullopt;
    }

    write << "extern StandardLimb " << symbol << ";\n";
    return std::nullopt;
}

ExportResult Z64::LimbCodeExporter::Export(std::ostream &write, std::shared_ptr<IParsedData> raw, std::string& entryName, YAML::Node &node, std::string* replacement ) {
    const auto symbol = GetSafeNode(node, "symbol", entryName);
    const auto offset = GetSafeNode<uint32_t>(node, "offset");
    auto limbData = std::static_pointer_cast<Z64::LimbData>(raw);

    write << "StandardLimb " << symbol << "= {\n";
    std::stringstream child;
    std::stringstream sibling;

    if (limbData->mChild == LIMB_DONE) {
        child << "LIMB_DONE";
    } else {
        child << "0x" << std::hex << (uint32_t)limbData->mChild;
    }
    if (limbData->mSibling == LIMB_DONE) {
        sibling << "LIMB_DONE";
    } else {
        sibling << "0x" << std::hex << (uint32_t)limbData->mSibling;
    }

    write << limbData->mJointPos << ", " << child.str() << ", " << sibling.str() << ",\n";

    auto dec = Companion::Instance->GetNodeByAddr(limbData->mDList);
    if (dec.has_value()) {
        auto dlNode = std::get<1>(dec.value());
        write << GetSafeNode<std::string>(dlNode, "symbol");
    } else {
        SPDLOG_WARN("Cannot find node for ptr 0x{:X}", limbData->mDList);
        write << std::hex << "0x" << limbData->mDList << std::dec;
    }

    write << "\n};\n";

    size_t size = 0xC;

    return offset + size;
}

ExportResult Z64::LimbBinaryExporter::Export(std::ostream &write, std::shared_ptr<IParsedData> raw, std::string& entryName, YAML::Node &node, std::string* replacement ) {
    auto writer = LUS::BinaryWriter();
    auto limbData = std::static_pointer_cast<Z64::LimbData>(raw);

    WriteHeader(writer, LUS::ResourceType::ZLimb, 0);


    writer.Finish(write);
    return std::nullopt;
}

std::optional<std::shared_ptr<IParsedData>> Z64::LimbFactory::parse(std::vector<uint8_t>& buffer, YAML::Node& node) {
    auto [_, segment] = Decompressor::AutoDecode(node, buffer);
    LUS::BinaryReader reader(segment.data, segment.size);
    reader.SetEndianness(LUS::Endianness::Big);

    auto jointPosX = reader.ReadInt16();
    auto jointPosY = reader.ReadInt16();
    auto jointPosZ = reader.ReadInt16();

    Vec3s jointPos = Vec3s(jointPosX, jointPosY, jointPosZ);

    auto child = reader.ReadUByte();
    auto sibling = reader.ReadUByte();

    auto dList = reader.ReadUInt32();
    // TODO: auto gen

    return std::make_shared<Z64::LimbData>(jointPos, child, sibling, dList);
}
