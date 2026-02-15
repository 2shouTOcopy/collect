#include "core/IPlugin.h"
#include <iostream>

class logfile_writerPlugin : public IPlugin {
public:
    std::string Name() const override { return "logfile_writer"; }
    // TODO Phase 2: implement correct HasRead/HasWrite/HasFlush
    int Init() override { return 0; }
};

extern "C" {
    IPlugin *CreateModule() { return new logfile_writerPlugin(); }
    void DestroyModule(IPlugin *p) { delete p; }
}
