#include "instr_helpers.hpp"

namespace riscv
{
    // Instruction handlers
    EXTERN_INSTR(FMADD);
    EXTERN_INSTR(FMSUB);
    EXTERN_INSTR(FNMADD);
    EXTERN_INSTR(FNMSUB);
    EXTERN_INSTR(FADD);
    EXTERN_INSTR(FSUB);
    EXTERN_INSTR(FMUL);
    EXTERN_INSTR(FDIV);
    EXTERN_INSTR(FSGNJ_NX);
    EXTERN_INSTR(FMIN_FMAX);
    EXTERN_INSTR(FSQRT);
    EXTERN_INSTR(FEQ_FLT_FLE);
    EXTERN_INSTR(FCVT_SD_DS);
    EXTERN_INSTR(FCVT_W_SD);
    EXTERN_INSTR(FCVT_SD_W);
    EXTERN_INSTR(FMV_X_W);
    EXTERN_INSTR(FCLASS);
    EXTERN_INSTR(FMV_W_X);
    EXTERN_INSTR(VLE32);
    EXTERN_INSTR(VSE32);
    EXTERN_INSTR(VOPI_VV);
    EXTERN_INSTR(VOPF_VV);
    EXTERN_INSTR(VOPM_VV);
    EXTERN_INSTR(VOPI_VI);
    EXTERN_INSTR(VOPF_VF);
    EXTERN_INSTR(VSETVLI);
    EXTERN_INSTR(VSETVL);
    EXTERN_INSTR(VSETIVLI);
}