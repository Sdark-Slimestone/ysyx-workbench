#ifndef __FTRACE_H__
#define __FTRACE_H__

#include <memory/paddr.h>

void find_func_from_elf_and_store(const char *elf_file);
const char *ftrace_find_func(paddr_t addr);

#endif