# YAL - Yet Another Launcher

## A ELF first loader for Hollyhock-3.

It supports multiple versions and sets up dynamic offsets.

## App format:
- binary format: elf (static)
- extension: `.hh3`
- enty: `int entry(argc, char ** argv, char ** envp)`
  - argc: number of entries in argv; usually 1
  - argv: command line
    - argv[0]: file path of the executable
  - envp: environment variables; `NAME=VALUE`
    - `HHK_SYMBOL_TABLE`: 8 charachers of hexadecimal - pointer to the compact offset table
    - `HHK_SYMBOL_TABLE_LEN`: 8 charachters of hexadecimal - length of the compact offset table
- notes:
  - `VERSION`: optional, version string
  - `AUTHOR`: optional, auther string
  - `NAME`: optional, display name string
  - `DESCRIPTION`: optional, longer description string
- only PHDR and EHDR information is used
