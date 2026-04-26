#include <common.h>        
#include <stdio.h>         
#include <stdlib.h>        
#include <string.h>         
#include <elf.h>             
#include <memory/paddr.h>    

#define MAX_FUNC_NUM 256
typedef struct {
    paddr_t addr;
    paddr_t size;
    char    name[64];
} FuncSym;
static FuncSym func_table[MAX_FUNC_NUM];
static int     func_count = 0;

void find_func_from_elf_and_store(const char *elf_file) {
    if (elf_file == NULL) return;

    FILE *fp = fopen(elf_file, "rb");
    if (fp == NULL) {
        Log("ftrace: Can not open ELF file '%s'", elf_file);
        return;
    }

    // 1. 读 ELF 头（ELF Header）
    Elf32_Ehdr ELF_Header;
    if (fread(&ELF_Header, sizeof(ELF_Header), 1, fp) != 1) {
        Log("ftrace: Failed to read ELF header");
        fclose(fp); return;
    }
    if (memcmp(ELF_Header.e_ident, ELFMAG, SELFMAG) != 0) {
        Log("ftrace: Not a valid ELF file");
        fclose(fp);
        return;
    }

    // 2. 读节头表（Section Header Table），即整个“目录”
    Elf32_Shdr *Section_Header_Table_zhizhen = malloc(ELF_Header.e_shentsize * ELF_Header.e_shnum);
    fseek(fp, ELF_Header.e_shoff, SEEK_SET);
    if (fread(Section_Header_Table_zhizhen, ELF_Header.e_shentsize, ELF_Header.e_shnum, fp) != ELF_Header.e_shnum) {
        Log("ftrace: Failed to read section headers");
        free(Section_Header_Table_zhizhen); fclose(fp); return;
    }

    // 3. 通过 e_shstrndx 找到 .shstrtab 节的节头（Section Header）
    Elf32_Shdr *Section_Header_shstrtab_zhizhen = &Section_Header_Table_zhizhen[ELF_Header.e_shstrndx];
    // 分配内存并读出 .shstrtab 节的数据内容（shstrtab Data）
    char *shstrtab_Data_zhizhen = malloc(Section_Header_shstrtab_zhizhen->sh_size);
    fseek(fp, Section_Header_shstrtab_zhizhen->sh_offset, SEEK_SET);
    if (fread(shstrtab_Data_zhizhen, Section_Header_shstrtab_zhizhen->sh_size, 1, fp) != 1) {
        Log("ftrace: Failed to read .shstrtab");
        free(shstrtab_Data_zhizhen); free(Section_Header_Table_zhizhen); fclose(fp); return;
    }

    // 4. 在节头表中寻找 .symtab 和 .strtab 节的节头
    Elf32_Shdr *Section_Header_symtab_zhizhen = NULL;
    Elf32_Shdr *Section_Header_strtab_zhizhen = NULL;

    for (int i = 0; i < ELF_Header.e_shnum; i++) {
        // 用 sh_name 偏移从 shstrtab Data 中取出该节的名字
        char *section_name = shstrtab_Data_zhizhen + Section_Header_Table_zhizhen[i].sh_name;
        if (strcmp(section_name, ".symtab") == 0) {
            Section_Header_symtab_zhizhen = &Section_Header_Table_zhizhen[i];
        } else if (strcmp(section_name, ".strtab") == 0) {
            Section_Header_strtab_zhizhen = &Section_Header_Table_zhizhen[i];
        }
    }

    if (Section_Header_symtab_zhizhen == NULL || Section_Header_strtab_zhizhen == NULL) {
        Log("ftrace: .symtab or .strtab not found");
        free(shstrtab_Data_zhizhen);
        free(Section_Header_Table_zhizhen);
        fclose(fp);
        return;
    }

    // 5. 读 .symtab 节的内容（Symbol Table Data）
    Elf32_Sym *symtab_Data_zhizhen = malloc(Section_Header_symtab_zhizhen->sh_size);
    fseek(fp, Section_Header_symtab_zhizhen->sh_offset, SEEK_SET);
    if (fread(symtab_Data_zhizhen, Section_Header_symtab_zhizhen->sh_size, 1, fp) != 1) {
        Log("ftrace: Failed to read .symtab");
        free(symtab_Data_zhizhen); free(shstrtab_Data_zhizhen); free(Section_Header_Table_zhizhen); fclose(fp); return;
    }

    // 6. 读 .strtab 节的内容（String Table Data）
    char *strtab_Data_zhizhen = malloc(Section_Header_strtab_zhizhen->sh_size);
    fseek(fp, Section_Header_strtab_zhizhen->sh_offset, SEEK_SET);
    if (fread(strtab_Data_zhizhen, Section_Header_strtab_zhizhen->sh_size, 1, fp) != 1) {
        Log("ftrace: Failed to read .strtab");
        free(strtab_Data_zhizhen); free(symtab_Data_zhizhen); free(shstrtab_Data_zhizhen); free(Section_Header_Table_zhizhen); fclose(fp); return;
    }

    // 7. 遍历符号表，筛选出 FUNC 类型，存入 func_table
    int sym_count = Section_Header_symtab_zhizhen->sh_size / sizeof(Elf32_Sym);
    func_count = 0;
    for (int i = 0; i < sym_count && func_count < MAX_FUNC_NUM; i++) {  //MAX_FUNC_NUM 自定义func最大容量
        if (ELF32_ST_TYPE(symtab_Data_zhizhen[i].st_info) == STT_FUNC) {
            func_table[func_count].addr = symtab_Data_zhizhen[i].st_value;
            func_table[func_count].size = symtab_Data_zhizhen[i].st_size;
            // 从 strtab Data 中拷贝函数名
            strncpy(func_table[func_count].name,
                    strtab_Data_zhizhen + symtab_Data_zhizhen[i].st_name, 63);
            func_table[func_count].name[63] = '\0';
            func_count++;
        }
    }

    // 清理临时资源
    free(symtab_Data_zhizhen);
    free(strtab_Data_zhizhen);
    free(shstrtab_Data_zhizhen);
    free(Section_Header_Table_zhizhen);
    fclose(fp);
}



// 输入：指令地址 addr
// 输出：该地址所在的函数名，找不到返回 "???"
const char *ftrace_find_func(paddr_t addr) {
    for (int i = 0; i < func_count; i++) {
        // 检查 addr 是否在 [func_table[i].addr, addr + size) 之间
        if (addr >= func_table[i].addr && addr < func_table[i].addr + func_table[i].size) {
            return func_table[i].name;
        }
    }
    return "???";
}