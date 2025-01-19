#include "PlayerAnimationFactory.h"

#include "utils/Decompressor.h"
#include "spdlog/spdlog.h"
#include "Companion.h"

#define PLAYER_LIMB_COUNT 22

ExportResult OOT::PlayerAnimationHeaderExporter::Export(std::ostream &write, std::shared_ptr<IParsedData> raw, std::string& entryName, YAML::Node &node, std::string* replacement) {
    const auto symbol = GetSafeNode(node, "symbol", entryName);

    if(Companion::Instance->IsOTRMode()){
        write << "static const ALIGN_ASSET(2) char " << symbol << "[] = \"__OTR__" << (*replacement) << "\";\n\n";
        return std::nullopt;
    }

    return std::nullopt;
}

ExportResult OOT::PlayerAnimationBinaryExporter::Export(std::ostream &write, std::shared_ptr<IParsedData> raw, std::string& entryName, YAML::Node &node, std::string* replacement) {
    auto writer = LUS::BinaryWriter();
    const auto playerAnimData = std::static_pointer_cast<PlayerAnimationData>(raw);


    WriteHeader(writer, Torch::ResourceType::SOH_PlayerAnimation, 0);

    writer.Write((uint32_t)playerAnimData->mLimbRotData.size());
    SPDLOG_INFO("LIMB DATA COUNT {}", playerAnimData->mLimbRotData.size());
    for (size_t i = 0; i < playerAnimData->mLimbRotData.size(); i++) {
        writer.Write(playerAnimData->mLimbRotData.at(i));
    }

    writer.Finish(write);
    return std::nullopt;
}

ExportResult OOT::PlayerAnimationModdingExporter::Export(std::ostream &write, std::shared_ptr<IParsedData> raw, std::string& entryName, YAML::Node &node, std::string* replacement) {
    const auto playerAnimData = std::static_pointer_cast<PlayerAnimationData>(raw);
    const auto symbol = GetSafeNode(node, "symbol", entryName);

    *replacement += ".yaml";

    std::stringstream stream;
    YAML::Emitter out;

    out << YAML::BeginMap;
    out << YAML::Key << symbol;
    out << YAML::Value;
    out.SetIndent(2);
    out << YAML::BeginMap;
    out << YAML::Key << "Data";
    out << YAML::Value;
    out << YAML::Newline;
    out << YAML::Flow << YAML::BeginSeq;
    for (size_t i = 0; i < playerAnimData->mLimbRotData.size() - 1; i++) {
        out << YAML::Hex << playerAnimData->mLimbRotData.at(i);
    }
    out << YAML::EndSeq << YAML::Block << YAML::Dec;
    out << YAML::EndMap;
    out << YAML::EndMap;

    write.write(out.c_str(), out.size());

    return std::nullopt;
}

std::optional<std::shared_ptr<IParsedData>> OOT::PlayerAnimationFactory::parse(std::vector<uint8_t>& buffer, YAML::Node& node) {
    auto [_, segment] = Decompressor::AutoDecode(node, buffer);
    LUS::BinaryReader reader(segment.data, segment.size);

    reader.SetEndianness(Torch::Endianness::Big);

    const auto frameCount = GetSafeNode<size_t>(node, "count");
    std::vector<int16_t> limbRotData;

    for (size_t i = 0; i < (PLAYER_LIMB_COUNT * 3 + 1) * frameCount; i++) {
        limbRotData.push_back(reader.ReadInt16());
    }
  

    return std::make_shared<PlayerAnimationData>(limbRotData);
}

std::optional<std::shared_ptr<IParsedData>> OOT::PlayerAnimationFactory::parse_modding(std::vector<uint8_t>& buffer, YAML::Node& node) {
    YAML::Node assetNode;

    try {
        std::string text((char*) buffer.data(), buffer.size());
        assetNode = YAML::Load(text.c_str());
    } catch (YAML::ParserException& e) {
        SPDLOG_ERROR("Failed to parse message data: {}", e.what());
        SPDLOG_ERROR("{}", (char*) buffer.data());
        return std::nullopt;
    }

    const auto info = assetNode.begin()->second;

    auto data = info["Data"];
    std::vector<int16_t> limbRotData;

    for (YAML::iterator it = data.begin(); it != data.end(); ++it) {
        limbRotData.push_back((int16_t)(*it).as<uint16_t>());
    }

    return std::make_shared<PlayerAnimationData>(limbRotData);
}
