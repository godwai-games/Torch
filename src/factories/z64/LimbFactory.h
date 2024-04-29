#pragma once

#include "factories/BaseFactory.h"
#include "types/Vec3D.h"

#define LIMB_DONE 0xFF

namespace Z64 {

class LimbData : public IParsedData {
public:
    Vec3s mJointPos;
    uint8_t mChild;
    uint8_t mSibling;
    uint32_t mDList;

    LimbData(Vec3s jointPos, uint8_t child, uint8_t sibling, uint32_t dList) : mJointPos(std::move(jointPos)), mChild(child), mSibling(sibling), mDList(dList) {}
};

class LimbHeaderExporter : public BaseExporter {
    ExportResult Export(std::ostream& write, std::shared_ptr<IParsedData> data, std::string& entryName, YAML::Node& node, std::string* replacement) override;
};

class LimbBinaryExporter : public BaseExporter {
    ExportResult Export(std::ostream& write, std::shared_ptr<IParsedData> data, std::string& entryName, YAML::Node& node, std::string* replacement) override;
};

class LimbCodeExporter : public BaseExporter {
    ExportResult Export(std::ostream& write, std::shared_ptr<IParsedData> data, std::string& entryName, YAML::Node& node, std::string* replacement) override;
};

class LimbFactory : public BaseFactory {
public:
    std::optional<std::shared_ptr<IParsedData>> parse(std::vector<uint8_t>& buffer, YAML::Node& data) override;
    inline std::unordered_map<ExportType, std::shared_ptr<BaseExporter>> GetExporters() override {
        return {
            REGISTER(Code, LimbCodeExporter)
            REGISTER(Header, LimbHeaderExporter)
            REGISTER(Binary, LimbBinaryExporter)
        };
    }
};
}
