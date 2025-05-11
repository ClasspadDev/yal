#include "loader.hpp"
#include <cstring>
#include <elf.h>
#include <memory>
#include <unistd.h>

std::unique_ptr<char[]> ELFLoader::getName() {
  init_notes();

  for (const auto &note : *notes)
    if (std::strncmp(note->getName().data(), "NAME", note->getName().size()) ==
        0) {
      auto name = std::make_unique<char[]>(note->getDesc().size() + 1);
      std::copy_n(note->getDesc().data(), note->getDesc().size(), name.get());
      name[note->getDesc().size()] =
          '\0'; // just in case its not allready (wich is allowed)
      return name;
    }

  return getPath();
}

std::unique_ptr<char[]> ELFLoader::getAuthor() {
  init_notes();

  for (const auto &note : *notes)
    if (std::strncmp(note->getName().data(), "AUTHOR",
                     note->getName().size()) == 0) {
      auto author = std::make_unique<char[]>(note->getDesc().size() + 1);
      std::copy_n(note->getDesc().data(), note->getDesc().size(), author.get());
      author[note->getDesc().size()] =
          '\0'; // just in case its not allready (wich is allowed)
      return author;
    }

  return nullptr;
}

std::unique_ptr<char[]> ELFLoader::getDescription() {
  init_notes();

  for (const auto &note : *notes)
    if (std::strncmp(note->getName().data(), "DESCRIPTION",
                     note->getName().size()) == 0) {
      auto desc = std::make_unique<char[]>(note->getDesc().size() + 1);
      std::copy_n(note->getDesc().data(), note->getDesc().size(), desc.get());
      desc[note->getDesc().size()] =
          '\0'; // just in case its not allready (wich is allowed)
      return desc;
    }

  return nullptr;
}

std::unique_ptr<char[]> ELFLoader::getVersion() {
  init_notes();

  for (const auto &note : *notes)
    if (std::strncmp(note->getName().data(), "VERSION",
                     note->getName().size()) == 0) {
      auto version = std::make_unique<char[]>(note->getDesc().size() + 1);
      std::copy_n(note->getDesc().data(), note->getDesc().size(),
                  version.get());
      version[note->getDesc().size()] =
          '\0'; // just in case its not allready (wich is allowed)
      return version;
    }

  return nullptr;
}
