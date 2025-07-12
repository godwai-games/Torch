#pragma once

#include "factories/BaseFactory.h"
#include "utils/TextureUtils.h"
#include <types/RawBuffer.h>
#include <unordered_map>
#include <string>
#include <vector>

namespace BK64 {

class SpriteData : public IParsedData {
public:
    std::vector<uint16_t> mChunkCounts;

    SpriteData(std::vector<uint16_t> chunkCounts) : mChunkCounts(std::move(chunkCounts)) {}
};

class SpriteHeaderExporter : public BaseExporter {
    ExportResult Export(std::ostream& write, std::shared_ptr<IParsedData> raw, std::string& entryName, YAML::Node& node, std::string* replacement) override;
};

class SpriteBinaryExporter : public BaseExporter {
    ExportResult Export(std::ostream& write, std::shared_ptr<IParsedData> raw, std::string& entryName, YAML::Node& node, std::string* replacement) override;
};

class SpriteCodeExporter : public BaseExporter {
    ExportResult Export(std::ostream& write, std::shared_ptr<IParsedData> raw, std::string& entryName, YAML::Node& node, std::string* replacement) override;
};

class SpriteModdingExporter : public BaseExporter {
    ExportResult Export(std::ostream& write, std::shared_ptr<IParsedData> raw, std::string& entryName, YAML::Node& node, std::string* replacement) override;
};

class SpriteFactory : public BaseFactory {
public:
    std::optional<std::shared_ptr<IParsedData>> parse(std::vector<uint8_t>& buffer, YAML::Node& data) override;
    inline std::unordered_map<ExportType, std::shared_ptr<BaseExporter>> GetExporters() override {
        return {
            REGISTER(Header, SpriteHeaderExporter)
            REGISTER(Binary, SpriteBinaryExporter)
            REGISTER(Code, SpriteCodeExporter)
            REGISTER(Modding, SpriteModdingExporter)
        };
    }

    bool HasModdedDependencies() override { return true; }
    bool SupportModdedAssets() override { return true; }
};

} // namespace BK64
