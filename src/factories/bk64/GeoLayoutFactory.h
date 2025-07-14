#pragma once

#include <factories/BaseFactory.h>

namespace BK64 {

typedef std::variant<uint8_t, int8_t, uint16_t, int16_t, uint32_t, int32_t, float> GeoLayoutArg;

enum class GeoLayoutArgType {
    U8, S8, U16, S16, U32, S32, FLOAT
};

enum class GeoLayoutOpCode {
    Sort = 1,
    Bone,
    LoadDL,
    Skinning = 5,
    Branch,
    LOD = 8,
    ReferencePoint = 10,
    Selector = 12,
    DrawDistance = 13,
    UnknownCmd = 14
};

typedef struct GeoLayoutCommand {
    GeoLayoutOpCode opCode;
    std::vector<GeoLayoutArg> args;

    GeoLayoutCommand(GeoLayoutOpCode opCode, std::vector<GeoLayoutArg> args) : opCode(opCode), args(std::move(args)) {}
} GeoLayoutCommand;

class GeoLayoutData : public IParsedData {
  public:
    std::vector<GeoLayoutCommand> mCmds;

    GeoLayoutData(std::vector<GeoLayoutCommand> cmds) : mCmds(std::move(cmds)) {}
};

class GeoLayoutHeaderExporter : public BaseExporter {
    ExportResult Export(std::ostream& write, std::shared_ptr<IParsedData> data, std::string& entryName,
                        YAML::Node& node, std::string* replacement) override;
};

class GeoLayoutBinaryExporter : public BaseExporter {
    ExportResult Export(std::ostream& write, std::shared_ptr<IParsedData> data, std::string& entryName,
                        YAML::Node& node, std::string* replacement) override;
};

class GeoLayoutCodeExporter : public BaseExporter {
    ExportResult Export(std::ostream& write, std::shared_ptr<IParsedData> data, std::string& entryName,
                        YAML::Node& node, std::string* replacement) override;
};

class GeoLayoutFactory : public BaseFactory {
  public:
    std::optional<std::shared_ptr<IParsedData>> parse(std::vector<uint8_t>& buffer, YAML::Node& data) override;
    inline std::unordered_map<ExportType, std::shared_ptr<BaseExporter>> GetExporters() override {
        return { 
            REGISTER(Code, GeoLayoutCodeExporter) 
            REGISTER(Header, GeoLayoutHeaderExporter)                     
            REGISTER(Binary, GeoLayoutBinaryExporter) 
        };
    }
};
} // namespace BK64
