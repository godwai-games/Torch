#pragma once

#include <factories/BaseFactory.h>

namespace OOT {

class PlayerAnimationData : public IParsedData {
public:
    std::vector<int16_t> mLimbRotData;

    PlayerAnimationData(std::vector<int16_t> limbRotData) : mLimbRotData(std::move(limbRotData)) {}
};

class PlayerAnimationHeaderExporter : public BaseExporter {
    ExportResult Export(std::ostream& write, std::shared_ptr<IParsedData> data, std::string& entryName, YAML::Node& node, std::string* replacement) override;
};

class PlayerAnimationBinaryExporter : public BaseExporter {
    ExportResult Export(std::ostream& write, std::shared_ptr<IParsedData> data, std::string& entryName, YAML::Node& node, std::string* replacement) override;
};

class PlayerAnimationModdingExporter : public BaseExporter {
    ExportResult Export(std::ostream& write, std::shared_ptr<IParsedData> data, std::string& entryName, YAML::Node& node, std::string* replacement) override;
};

class PlayerAnimationFactory : public BaseFactory {
public:
    std::optional<std::shared_ptr<IParsedData>> parse(std::vector<uint8_t>& buffer, YAML::Node& data) override;
    std::optional<std::shared_ptr<IParsedData>> parse_modding(std::vector<uint8_t>& buffer, YAML::Node& data) override;
    inline std::unordered_map<ExportType, std::shared_ptr<BaseExporter>> GetExporters() override {
        return {
            REGISTER(Header, PlayerAnimationHeaderExporter)
            REGISTER(Binary, PlayerAnimationBinaryExporter)
            REGISTER(Modding, PlayerAnimationModdingExporter)
        };
    }
    bool SupportModdedAssets() override { return true; }
};
} // namespace FZX
