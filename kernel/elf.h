#ifndef _ELF_H_
#define _ELF_H_

#include "util/types.h"
#include "process.h"

#define MAX_CMDLINE_ARGS 64

// elf header structure
typedef struct elf_header_t {
  uint32 magic;
  uint8 elf[12];
  uint16 type;      /* Object file type */
  uint16 machine;   /* Architecture */
  uint32 version;   /* Object file version */
  uint64 entry;     /* Entry point virtual address */
  uint64 phoff;     /* Program header table file offset */
  uint64 shoff;     /* Section header table file offset */
  uint32 flags;     /* Processor-specific flags */
  uint16 ehsize;    /* ELF header size in bytes */
  uint16 phentsize; /* Program header table entry size */
  uint16 phnum;     /* Program header table entry count */
  uint16 shentsize; /* Section header table entry size */
  uint16 shnum;     /* Section header table entry count */
  uint16 shstrndx;  /* Section header string table index */
} elf_header;

// Program segment header.
typedef struct elf_prog_header_t {
  uint32 type;   /* Segment type */
  uint32 flags;  /* Segment flags */
  uint64 off;    /* Segment file offset */
  uint64 vaddr;  /* Segment virtual address */
  uint64 paddr;  /* Segment physical address */
  uint64 filesz; /* Segment size in file */
  uint64 memsz;  /* Segment size in memory */
  uint64 align;  /* Segment alignment */
} elf_prog_header;

// Section header and symbol structures used by the user backtrace challenge.
typedef struct elf_section_header_t {
  uint32 name;       /* Section name, an offset in .shstrtab */
  uint32 type;       /* Section type */
  uint64 flags;      /* Section flags */
  uint64 addr;       /* Section virtual address */
  uint64 offset;     /* Section file offset */
  uint64 size;       /* Section size */
  uint32 link;       /* Related section index */
  uint32 info;       /* Section-specific information */
  uint64 addralign;  /* Section alignment */
  uint64 entsize;    /* Section entry size */
} elf_section_header;

typedef struct elf_symbol_t {
  uint32 name;       /* Symbol name, an offset in the linked string table */
  uint8 info;        /* Symbol type and binding */
  uint8 other;       /* Symbol visibility */
  uint16 shndx;      /* Section index */
  uint64 value;      /* Symbol value, function start address */
  uint64 size;       /* Symbol size */
} elf_symbol;

#define ELF_SHT_SYMTAB 2
#define ELF_STT_FUNC 2
#define ELF_SHN_UNDEF 0

#define ELF_MAGIC 0x464C457FU  // "\x7FELF" in little endian
#define ELF_PROG_LOAD 1

typedef enum elf_status_t {
  EL_OK = 0,

  EL_EIO,
  EL_ENOMEM,
  EL_NOTELF,
  EL_ERR,

} elf_status;

typedef struct elf_ctx_t {
  void *info;
  elf_header ehdr;
} elf_ctx;

elf_status elf_init(elf_ctx *ctx, void *info);
elf_status elf_load(elf_ctx *ctx);

void load_bincode_from_host_elf(process *p);

int elf_lookup_symbol(uint64 addr, char* name, size_t name_size);

#endif
