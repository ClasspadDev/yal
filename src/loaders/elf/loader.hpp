#include "../interface.hpp"
#include "elfnotes.hpp"
#include <elf.h>
#include <memory>

class ELFLoader : public FileBasedExecutable<".hh3"> {
public:
  using FileBasedExecutable::FileBasedExecutable;
  void load() override;
  void unload() override;
  int execute() override;

  std::unique_ptr<char[]> getName() override;
  std::unique_ptr<char[]> getAuthor() override;
  std::unique_ptr<char[]> getDescription() override;
  std::unique_ptr<char[]> getVersion() override;

private:
  std::unique_ptr<Elf32_Ehdr> ehdr = nullptr;
  std::unique_ptr<Elf32_Phdr[]> phdrs = nullptr;
  std::unique_ptr<std::forward_list<std::unique_ptr<ELFNotes>>> notes = nullptr;
  bool in_vbak = false;

  void init_ehdr();
  void init_phdrs();
  void init_notes();
};
