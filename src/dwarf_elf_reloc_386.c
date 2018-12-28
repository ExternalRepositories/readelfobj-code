/* Created by build_access.py */
#include "dwarf_elf_reloc_386.h"

/* returns string of length 0 if invalid arg */
const char *
dwarf_get_elf_relocname_386(unsigned long val)
{
    switch(val) {
    case R_386_NONE : return "R_386_NONE";
    case R_386_32 : return "R_386_32";
    case R_386_PC32 : return "R_386_PC32";
    case R_386_GOT32 : return "R_386_GOT32";
    case R_386_PLT32 : return "R_386_PLT32";
    case R_386_COPY : return "R_386_COPY";
    case R_386_GLOB_DAT : return "R_386_GLOB_DAT";
    case R_386_JMP_SLOT : return "R_386_JMP_SLOT";
    case R_386_RELATIVE : return "R_386_RELATIVE";
    case R_386_GOTOFF : return "R_386_GOTOFF";
    case R_386_GOTPC : return "R_386_GOTPC";
    case R_386_32PLT : return "R_386_32PLT";
    case R_386_TLS_TPOFF : return "R_386_TLS_TPOFF";
    case R_386_TLS_IE : return "R_386_TLS_IE";
    case R_386_TLS_GOTIE : return "R_386_TLS_GOTIE";
    case R_386_TLS_LE : return "R_386_TLS_LE";
    case R_386_TLS_LDM : return "R_386_TLS_LDM";
    case R_386_16 : return "R_386_16";
    case R_386_PC16 : return "R_386_PC16";
    case R_386_8 : return "R_386_8";
    case R_386_PC8 : return "R_386_PC8";
    case R_386_TLS_GD_32 : return "R_386_TLS_GD_32";
    case R_386_TLS_GD_PUSH : return "R_386_TLS_GD_PUSH";
    case R_386_TLS_GD_CALL : return "R_386_TLS_GD_CALL";
    case R_386_TLS_GD_POP : return "R_386_TLS_GD_POP";
    case R_386_TLS_LDM_32 : return "R_386_TLS_LDM_32";
    case R_386_TLS_LDM_PUSH : return "R_386_TLS_LDM_PUSH";
    case R_386_TLS_LDM_CALL : return "R_386_TLS_LDM_CALL";
    case R_386_TLS_LDM_POP : return "R_386_TLS_LDM_POP";
    case R_386_TLS_LDO_32 : return "R_386_TLS_LDO_32";
    case R_386_TLS_IE_32 : return "R_386_TLS_IE_32";
    case R_386_TLS_LE_32 : return "R_386_TLS_LE_32";
    case R_386_TLS_DTPMOD32 : return "R_386_TLS_DTPMOD32";
    case R_386_TLS_DTPOFF32 : return "R_386_TLS_DTPOFF32";
    case R_386_TLS_TPOFF32 : return "R_386_TLS_TPOFF32";
    case R_386_SIZE32 : return "R_386_SIZE32";
    case R_386_TLS_GOTDESC : return "R_386_TLS_GOTDESC";
    case R_386_TLS_DESC_CALL : return "R_386_TLS_DESC_CALL";
    case R_386_TLS_DESC : return "R_386_TLS_DESC";
    case R_386_IRELATIVE : return "R_386_IRELATIVE";
    }
return "";
}
