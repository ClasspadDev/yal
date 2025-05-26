#include "../interface.hpp"

class BinaryLoader : public FileBasedExecutable<".bin"> {
public:
  using FileBasedExecutable::FileBasedExecutable;
  void load() override;
  void unload() override;
  int execute() override;

private:
  void *execution_address = nullptr;
  bool in_vram_or_vbak = false;
};
