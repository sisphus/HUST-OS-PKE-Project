/*
 * routines that scan and load a (host) Executable and Linkable Format (ELF) file
 * into the (emulated) memory.
 */

#include "elf.h"
#include "string.h"
#include "riscv.h"
#include "spike_interface/spike_utils.h"

static spike_file_t* g_user_elf_file;
static elf_header g_user_elf_header;

typedef struct elf_info_t {
  spike_file_t *f;
  process *p;
} elf_info;

//
// the implementation of allocater. allocates memory space for later segment loading
//
static void *elf_alloc_mb(elf_ctx *ctx, uint64 elf_pa, uint64 elf_va, uint64 size) {
  // directly returns the virtual address as we are in the Bare mode in lab1_x
  return (void *)elf_va;
}

//
// actual file reading, using the spike file interface.
//
static uint64 elf_fpread(elf_ctx *ctx, void *dest, uint64 nb, uint64 offset) {
  elf_info *msg = (elf_info *)ctx->info;
  // call spike file utility to load the content of elf file into memory.
  // spike_file_pread will read the elf file (msg->f) from offset to memory (indicated by
  // *dest) for nb bytes.
  return spike_file_pread(msg->f, dest, nb, offset);
}

static int elf_read_bytes(void* dest, uint64 nb, uint64 offset) {
  if (!g_user_elf_file) return 0;
  return spike_file_pread(g_user_elf_file, dest, nb, offset) == (ssize_t)nb;
}

static int elf_read_section_header(uint16 index, elf_section_header* section) {
  if (index >= g_user_elf_header.shnum || g_user_elf_header.shentsize < sizeof(*section))
    return 0;

  uint64 offset = g_user_elf_header.shoff + (uint64)index * g_user_elf_header.shentsize;
  return elf_read_bytes(section, sizeof(*section), offset);
}

static int elf_read_string(const elf_section_header* string_section, uint32 string_offset,
                           char* dest, size_t dest_size) {
  if (dest_size == 0 || (uint64)string_offset >= string_section->size) return 0;

  uint64 available = string_section->size - string_offset;
  size_t read_size = available < dest_size ? available : dest_size;
  if (!elf_read_bytes(dest, read_size, string_section->offset + string_offset)) return 0;

  for (size_t i = 0; i < read_size; i++) {
    if (dest[i] == '\0') return 1;
  }

  dest[dest_size - 1] = '\0';
  return 1;
}

int elf_lookup_symbol(uint64 addr, char* name, size_t name_size) {
  if (!g_user_elf_file || name_size == 0 || g_user_elf_header.shstrndx >= g_user_elf_header.shnum)
    return 0;

  elf_section_header section_name_table;
  if (!elf_read_section_header(g_user_elf_header.shstrndx, &section_name_table)) return 0;

  elf_section_header symbol_table;
  int found_symbol_table = 0;
  for (uint16 i = 0; i < g_user_elf_header.shnum; i++) {
    elf_section_header section;
    char section_name[32];
    if (!elf_read_section_header(i, &section) ||
        !elf_read_string(&section_name_table, section.name, section_name, sizeof(section_name)))
      continue;

    if (section.type == ELF_SHT_SYMTAB && strcmp(section_name, ".symtab") == 0) {
      symbol_table = section;
      found_symbol_table = 1;
      break;
    }
  }

  if (!found_symbol_table || symbol_table.link >= g_user_elf_header.shnum ||
      symbol_table.entsize < sizeof(elf_symbol))
    return 0;

  elf_section_header string_table;
  if (!elf_read_section_header(symbol_table.link, &string_table)) return 0;

  uint64 symbol_count = symbol_table.size / symbol_table.entsize;
  for (uint64 i = 0; i < symbol_count; i++) {
    elf_symbol symbol;
    uint64 offset = symbol_table.offset + i * symbol_table.entsize;
    if (!elf_read_bytes(&symbol, sizeof(symbol), offset)) return 0;

    uint8 symbol_type = symbol.info & 0xf;
    if (symbol_type != ELF_STT_FUNC || symbol.shndx == ELF_SHN_UNDEF || symbol.size == 0)
      continue;

    if (symbol.value <= addr && addr - symbol.value < symbol.size)
      return elf_read_string(&string_table, symbol.name, name, name_size);
  }

  return 0;
}

//
// init elf_ctx, a data structure that loads the elf.
//
elf_status elf_init(elf_ctx *ctx, void *info) {
  ctx->info = info;

  // load the elf header
  if (elf_fpread(ctx, &ctx->ehdr, sizeof(ctx->ehdr), 0) != sizeof(ctx->ehdr)) return EL_EIO;

  // check the signature (magic value) of the elf
  if (ctx->ehdr.magic != ELF_MAGIC) return EL_NOTELF;

  return EL_OK;
}

//
// load the elf segments to memory regions as we are in Bare mode in lab1
//
elf_status elf_load(elf_ctx *ctx) {
  // elf_prog_header structure is defined in kernel/elf.h
  elf_prog_header ph_addr;
  int i, off;

  // traverse the elf program segment headers
  for (i = 0, off = ctx->ehdr.phoff; i < ctx->ehdr.phnum; i++, off += sizeof(ph_addr)) {
    // read segment headers
    if (elf_fpread(ctx, (void *)&ph_addr, sizeof(ph_addr), off) != sizeof(ph_addr)) return EL_EIO;

    if (ph_addr.type != ELF_PROG_LOAD) continue;
    if (ph_addr.memsz < ph_addr.filesz) return EL_ERR;
    if (ph_addr.vaddr + ph_addr.memsz < ph_addr.vaddr) return EL_ERR;

    // allocate memory block before elf loading
    void *dest = elf_alloc_mb(ctx, ph_addr.vaddr, ph_addr.vaddr, ph_addr.memsz);

    // actual loading
    if (elf_fpread(ctx, dest, ph_addr.memsz, ph_addr.off) != ph_addr.memsz)
      return EL_EIO;
  }

  return EL_OK;
}

typedef union {
  uint64 buf[MAX_CMDLINE_ARGS];
  char *argv[MAX_CMDLINE_ARGS];
} arg_buf;

//
// returns the number (should be 1) of string(s) after PKE kernel in command line.
// and store the string(s) in arg_bug_msg.
//
static size_t parse_args(arg_buf *arg_bug_msg) {
  // HTIFSYS_getmainvars frontend call reads command arguments to (input) *arg_bug_msg
  long r = frontend_syscall(HTIFSYS_getmainvars, (uint64)arg_bug_msg,
      sizeof(*arg_bug_msg), 0, 0, 0, 0, 0);
  kassert(r == 0);

  size_t pk_argc = arg_bug_msg->buf[0];
  uint64 *pk_argv = &arg_bug_msg->buf[1];

  int arg = 1;  // skip the PKE OS kernel string, leave behind only the application name
  for (size_t i = 0; arg + i < pk_argc; i++)
    arg_bug_msg->argv[i] = (char *)(uintptr_t)pk_argv[arg + i];

  //returns the number of strings after PKE kernel in command line
  return pk_argc - arg;
}

//
// load the elf of user application, by using the spike file interface.
//
void load_bincode_from_host_elf(process *p) {
  arg_buf arg_bug_msg;

  // retrieve command line arguements
  size_t argc = parse_args(&arg_bug_msg);
  if (!argc) panic("You need to specify the application program!\n");

  sprint("Application: %s\n", arg_bug_msg.argv[0]);

  //elf loading. elf_ctx is defined in kernel/elf.h, used to track the loading process.
  elf_ctx elfloader;
  // elf_info is defined above, used to tie the elf file and its corresponding process.
  elf_info info;

  info.f = spike_file_open(arg_bug_msg.argv[0], O_RDONLY, 0);
  info.p = p;
  // IS_ERR_VALUE is a macro defined in spike_interface/spike_htif.h
  if (IS_ERR_VALUE(info.f)) panic("Fail on openning the input application program.\n");

  // init elfloader context. elf_init() is defined above.
  if (elf_init(&elfloader, &info) != EL_OK)
    panic("fail to init elfloader.\n");

  g_user_elf_file = info.f;
  g_user_elf_header = elfloader.ehdr;

  // load elf. elf_load() is defined above.
  if (elf_load(&elfloader) != EL_OK) panic("Fail on loading elf.\n");

  // entry (virtual, also physical in lab1_x) address
  p->trapframe->epc = elfloader.ehdr.entry;

  // Keep the host ELF open because the backtrace challenge reads its symbols.

  sprint("Application program entry point (virtual address): 0x%lx\n", p->trapframe->epc);
}
