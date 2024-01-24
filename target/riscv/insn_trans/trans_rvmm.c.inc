#include "tcg/tcg-op-gvec.h"
#include "tcg/tcg-gvec-desc.h"
#include "internals.h"
#include <stdio.h>
#include <math.h>


/************************************
*
*  sophgo mset command: accept rs gpr
*  content and then store them in the
*  corresponding global variable.
*
************************************/

static bool trans_sg_mset(DisasContext *s, arg_sg_mset *arg)
{
    switch (arg->csr)
    {
    case 0x0:
        tcg_gen_mov_tl(tm, cpu_gpr[arg->rs]);
        gen_helper_sg_setv(cpu_gpr[arg->rs], tcg_constant_tl(arg->csr),tcg_env);
        break;
    case 0x1:
        tcg_gen_mov_tl(tn, cpu_gpr[arg->rs]);
        gen_helper_sg_setv(cpu_gpr[arg->rs], tcg_constant_tl(arg->csr),tcg_env);
        break;
    case 0x2:
        tcg_gen_mov_tl(tk, cpu_gpr[arg->rs]);
        gen_helper_sg_setv(cpu_gpr[arg->rs], tcg_constant_tl(arg->csr),tcg_env);
        break;
    case 0x3:
        tcg_gen_mov_tl(stra, cpu_gpr[arg->rs]);
        gen_helper_sg_setv(cpu_gpr[arg->rs], tcg_constant_tl(arg->csr),tcg_env);
        break;
    case 0x4:
        tcg_gen_mov_tl(strb, cpu_gpr[arg->rs]);
        gen_helper_sg_setv(cpu_gpr[arg->rs], tcg_constant_tl(arg->csr),tcg_env);
        break;
    case 0x5:
        tcg_gen_mov_tl(strc, cpu_gpr[arg->rs]);
        gen_helper_sg_setv(cpu_gpr[arg->rs], tcg_constant_tl(arg->csr),tcg_env);
        break;
    case 0x6:
        gen_helper_sg_conv(cpu_gpr[arg->rs], tcg_env);
        break;
    case 0x7:
        gen_helper_sg_mnk(cpu_gpr[arg->rs], tcg_env);
        break;
    case 0x8:
        gen_helper_sg_i2c(cpu_gpr[arg->rs], tcg_env);
        break;
    case 0x9:
        gen_helper_sg_mtype(cpu_gpr[arg->rs], tcg_constant_tl(arg->rs), tcg_constant_tl(arg->ew), tcg_constant_tl(arg->am), tcg_env);
        break;
    default:
        break;
    }
    return true;
}

/************************************
*  @note: archived function.
*  sophgo mseti command: accept rs imm
*  (unsigned immediate) and then store
*  them in the corresponding global vari-
*  able.
*  @warning: archived function.
*
***********************************

static bool trans_sg_mseti(DisasContext *s, arg_sg_mseti *arg) {
    target_ulong uimm = arg->uimm1 << 10 | arg->uimm2;
    switch (arg->tm)
    {
    case 0x0:
        tcg_gen_movi_tl(tm, uimm);
        break;
    case 0x1:
        tcg_gen_movi_tl(tn, uimm);
        break;
    case 0x2:
        tcg_gen_movi_tl(tk, uimm);
        break;
    case 0x3:
        tcg_gen_movi_tl(stra, uimm);
        break;
    case 0x4:
        tcg_gen_movi_tl(strb, uimm);
        break;
    case 0x5:
        tcg_gen_movi_tl(strc, uimm);
        break;
    case 0x6:
        tcg_gen_movi_tl(conv, uimm);
        break;
    default:
        break;
    }
    return true;
}
*/

/*
 * Encode LMUL to lmul as follows:
 *     LMUL    vlmul    lmul
 *      1       000       0
 *      2       001       1
 *      4       010       2
 *      8       011       3
 *      -       100       -
 *     1/8      101      -3
 *     1/4      110      -2
 *     1/2      111      -1
 */

/************************************
*
*  sophgo m.v command: move data from
*  vector registers to tensor registers.
*
************************************/

static bool trans_sg_m_v(DisasContext *s, arg_sg_m_v *arg)
{
    TCGv_ptr src = tcg_temp_new_ptr();
    TCGv_ptr dst = tcg_temp_new_ptr();
    tcg_gen_addi_ptr(src, tcg_env, offsetof(CPURISCVState, vreg) + s->cfg_ptr->vlen / 8 * arg->vs);
    tcg_gen_addi_ptr(dst, tcg_env, offsetof(CPURISCVState, treg[arg->md].data));
    gen_helper_sg_move(dst, src, tcg_env);
    return true;
}

/************************************
*
*  sophgo v.m command: move data from
*  tensor registers to vector registers.
*
************************************/

static bool trans_sg_v_m(DisasContext *s, arg_sg_v_m *arg)
{
    TCGv_ptr src = tcg_temp_new_ptr();
    TCGv_ptr dst = tcg_temp_new_ptr();
    tcg_gen_addi_ptr(dst, tcg_env, offsetof(CPURISCVState, vreg) + s->cfg_ptr->vlen / 8 * arg->vd);
    tcg_gen_addi_ptr(src, tcg_env, offsetof(CPURISCVState, treg[arg->ms].data));
    gen_helper_sg_move(dst, src, tcg_env);
    return true;
}

/************************************
*
*  sophgo m.x command: move data from
*  constant registers to tensor registers.
*  with broadcast functions.
*
************************************/

static bool trans_sg_m_x(DisasContext *s, arg_sg_m_x *arg)
{
    TCGv_ptr dst = tcg_temp_new_ptr();
    tcg_gen_addi_ptr(dst, tcg_env, offsetof(CPURISCVState, treg[arg->md].data));
    gen_helper_sg_m_x(dst, cpu_gpr[arg->rs], tcg_env);
    return true;
}

/************************************
*
*  sophgo m.vvw command: move data 
*  from several (x4) vector registers
*  to tensor registers.
*
************************************/

static bool trans_sg_m_vvw(DisasContext *s, arg_sg_m_vvw *arg)
{
    TCGv_ptr src1 = tcg_temp_new_ptr();
    TCGv_ptr src2 = tcg_temp_new_ptr();
    TCGv_ptr src3 = tcg_temp_new_ptr();
    TCGv_ptr dst = tcg_temp_new_ptr();
    tcg_gen_addi_ptr(src1, tcg_env, offsetof(CPURISCVState, vreg) + s->cfg_ptr->vlen / 8 * arg->vs1);
    tcg_gen_addi_ptr(src2, tcg_env, offsetof(CPURISCVState, vreg) + s->cfg_ptr->vlen / 8 * (arg->vs2));
    tcg_gen_addi_ptr(src3, tcg_env, offsetof(CPURISCVState, vreg) + s->cfg_ptr->vlen / 8 * arg->vs3);
    tcg_gen_addi_ptr(dst, tcg_env, offsetof(CPURISCVState, treg[1<<(arg->md)].data));
    gen_helper_sg_m_vvw(dst, src1, src2, src3, tcg_env);
    return true;
}

/************************************
*
*  sophgo mld command: load matrix
*  to tensor registers from memory.
*
************************************/

static bool trans_sg_mld(DisasContext *s, arg_sg_mld *arg)
{
    TCGv_ptr reg_ptr = tcg_temp_new_ptr();
    tcg_gen_addi_ptr(reg_ptr, tcg_env, offsetof(CPURISCVState, treg[arg->md].data));
    switch (arg->abc)
    {
    case 0x0:
        gen_helper_default_mld(reg_ptr, get_gpr(s, arg->rs, EXT_NONE), tcg_constant_tl(arg->ew), tcg_env);
        break;
    case 0x1:
        gen_helper_sg_mld(reg_ptr, get_gpr(s, arg->rs, EXT_NONE), tm, tk, stra, tcg_constant_tl(arg->ew), tcg_env);
        break;
    case 0x2:
        gen_helper_sg_mld(reg_ptr, get_gpr(s, arg->rs, EXT_NONE), tn, tk, strb, tcg_constant_tl(arg->ew), tcg_env);
        break;
    case 0x3:
        gen_helper_sg_mld(reg_ptr, get_gpr(s, arg->rs, EXT_NONE), tm, tn, strc, tcg_constant_tl(arg->ew), tcg_env);
        break;
    default:
        break;
    }
    return true;
}

static bool trans_sg_mld_t(DisasContext *s, arg_sg_mld_t *arg)
{
    TCGv_ptr reg_ptr = tcg_temp_new_ptr();
    tcg_gen_addi_ptr(reg_ptr, tcg_env, offsetof(CPURISCVState, treg[arg->md].data));
    switch (arg->abc)
    {
    case 0x0:
        gen_helper_default_mld_t(reg_ptr, get_gpr(s, arg->rs, EXT_NONE), tcg_constant_tl(arg->ew), tcg_env);
        break;
    case 0x1:
        gen_helper_sg_mld_t(reg_ptr, get_gpr(s, arg->rs, EXT_NONE), tk, tm, stra, tcg_constant_tl(arg->ew), tcg_env);
        break;
    case 0x2:
        gen_helper_sg_mld_t(reg_ptr, get_gpr(s, arg->rs, EXT_NONE), tk, tn, strb, tcg_constant_tl(arg->ew), tcg_env);
        break;
    case 0x3:
        gen_helper_sg_mld_t(reg_ptr, get_gpr(s, arg->rs, EXT_NONE), tn, tm, strc, tcg_constant_tl(arg->ew), tcg_env);
        break;
    default:
        break;
    }
    return true;
}

/************************************
*
*  sophgo mst command: store matrix
*  to memory from tensor registers.
*
************************************/

static bool trans_sg_mst(DisasContext *s, arg_sg_mst *arg)
{
    TCGv_ptr reg_ptr = tcg_temp_new_ptr();
    tcg_gen_addi_ptr(reg_ptr, tcg_env, offsetof(CPURISCVState, treg[arg->ms].data));
    switch (arg->abc)
    {
    case 0x0:
        gen_helper_default_mst(reg_ptr, get_gpr(s, arg->rs, EXT_NONE), tcg_constant_tl(arg->ew), tcg_env);
        break;
    case 0x1:
        gen_helper_sg_mst(reg_ptr, get_gpr(s, arg->rs, EXT_NONE), tm, tk, stra, tcg_constant_tl(arg->ew), tcg_env);
        break;
    case 0x2:
        gen_helper_sg_mst(reg_ptr, get_gpr(s, arg->rs, EXT_NONE), tn, tk, strb, tcg_constant_tl(arg->ew), tcg_env);
        break;
    case 0x3:
        gen_helper_sg_mst(reg_ptr, get_gpr(s, arg->rs, EXT_NONE), tm, tn, strc, tcg_constant_tl(arg->ew), tcg_env);
        break;
    default:
        break;
    }
    return true;
}

static bool trans_sg_mst_t(DisasContext *s, arg_sg_mst_t *arg)
{
    TCGv_ptr reg_ptr = tcg_temp_new_ptr();
    tcg_gen_addi_ptr(reg_ptr, tcg_env, offsetof(CPURISCVState, treg[arg->ms].data));
    switch (arg->abc)
    {
    case 0x0:
        gen_helper_default_mst_t(reg_ptr, get_gpr(s, arg->rs, EXT_NONE), tcg_constant_tl(arg->ew), tcg_env);
        break;
    case 0x1:
        gen_helper_sg_mst_t(reg_ptr, get_gpr(s, arg->rs, EXT_NONE), tk, tm, stra, tcg_constant_tl(arg->ew), tcg_env);
        break;
    case 0x2:
        gen_helper_sg_mst_t(reg_ptr, get_gpr(s, arg->rs, EXT_NONE), tk, tn, strb, tcg_constant_tl(arg->ew), tcg_env);
        break;
    case 0x3:
        gen_helper_sg_mst_t(reg_ptr, get_gpr(s, arg->rs, EXT_NONE), tn, tm, strc, tcg_constant_tl(arg->ew), tcg_env);
        break;
    default:
        break;
    }
    return true;
}

/************************************
*
*  sophgo mma command: calculate matrix
*  (with accumulation) multiplication:
*  1) mmaqs: in8 -> int32
*  2) mmaf:  float16 -> float16
*  3) mmawf: float16 -> float32
*
************************************/

#define TRANS_MMA(CTX)                                                          \
static bool trans_sg_##CTX(DisasContext *s, arg_sg_##CTX *arg)                  \
{                                                                               \
    TCGv_ptr a = tcg_temp_new_ptr();                                            \
    TCGv_ptr b = tcg_temp_new_ptr();                                            \
    TCGv_ptr c = tcg_temp_new_ptr();                                            \
    tcg_gen_addi_ptr(a, tcg_env, offsetof(CPURISCVState, treg[arg->ms1].data)); \
    tcg_gen_addi_ptr(b, tcg_env, offsetof(CPURISCVState, treg[arg->ms2].data)); \
    tcg_gen_addi_ptr(c, tcg_env, offsetof(CPURISCVState, treg[arg->md].data));  \
    gen_helper_sg_##CTX(c, a, b, tcg_constant_tl(arg->rz), tcg_env);               \
    return true;                                                                \
}                                                                               \

TRANS_MMA(mmaqs)
TRANS_MMA(mmaf)
TRANS_MMA(mmawf)

/************************************
*
*  sophgo mma command: calculate matrix
*  multiplication:
*  1) mmaqs: in8 -> int32
*  2) mmaf:  float16 -> float16
*  3) mmawf: float16 -> float32
*
************************************/

#define TRANS_MM(CTX)                                                          \
static bool trans_sg_##CTX(DisasContext *s, arg_sg_##CTX *arg)                  \
{                                                                               \
    TCGv_ptr a = tcg_temp_new_ptr();                                            \
    TCGv_ptr b = tcg_temp_new_ptr();                                            \
    TCGv_ptr c = tcg_temp_new_ptr();                                            \
    tcg_gen_addi_ptr(a, tcg_env, offsetof(CPURISCVState, treg[arg->ms1].data)); \
    tcg_gen_addi_ptr(b, tcg_env, offsetof(CPURISCVState, treg[arg->ms2].data)); \
    tcg_gen_addi_ptr(c, tcg_env, offsetof(CPURISCVState, treg[arg->md].data));  \
    gen_helper_sg_##CTX(c, a, b, tcg_env);                                      \
    return true;                                                                \
}                                                                               \

TRANS_MM(mmqs)
TRANS_MM(mmf)
TRANS_MM(mmwf)

/************************************
*
*  sophgo transpose command: transpose
*  the shape of the matrix.
*
************************************/

static bool trans_sg_mtrans(DisasContext *s, arg_sg_mtrans *arg)
{
    TCGv_ptr src = tcg_temp_new_ptr();
    TCGv_ptr dst = tcg_temp_new_ptr();
    tcg_gen_addi_ptr(src, tcg_env, offsetof(CPURISCVState, treg[arg->ms1].data));
    tcg_gen_addi_ptr(dst, tcg_env, offsetof(CPURISCVState, treg[arg->md].data));
    gen_helper_sg_mtrans(dst, src, tcg_constant_tl(arg->ew), tcg_env);
    return true;
}

/************************************
*
*  sophgo i2c command: transfer the
*  original ic*iw*ih matrix to the new
*  oh*ow one for convolution operations.
*
************************************/

static bool trans_sg_img2col(DisasContext *s, arg_sg_img2col *arg)
{
    TCGv_ptr src = tcg_temp_new_ptr();
    TCGv_ptr dst = tcg_temp_new_ptr();
    tcg_gen_addi_ptr(src, tcg_env, offsetof(CPURISCVState, treg[arg->ms1].data));
    tcg_gen_addi_ptr(dst, tcg_env, offsetof(CPURISCVState, treg[arg->md].data));
    gen_helper_sg_img2col(dst, src, tcg_env);
    return true;
}

/************************************
*
*  sophgo dequantify command: dequanti-
*  fy back to the source matrix (int32 ->
*  FP16).
*
************************************/
#define TRANS_QUANT(CTX)                                                            \
static bool trans_sg_##CTX(DisasContext *s, arg_sg_##CTX *arg)                      \
{                                                                                   \
    TCGv_ptr src1 = tcg_temp_new_ptr();                                             \
    TCGv_ptr src2 = tcg_temp_new_ptr();                                             \
    TCGv_ptr dst = tcg_temp_new_ptr();                                              \
    tcg_gen_addi_ptr(src1, tcg_env, offsetof(CPURISCVState, treg[arg->ms1].data));  \
    tcg_gen_addi_ptr(src2, tcg_env, offsetof(CPURISCVState, treg[arg->ms2].data));  \
    tcg_gen_addi_ptr(dst, tcg_env, offsetof(CPURISCVState, treg[arg->md].data));    \
    gen_helper_sg_##CTX(dst, src1, src2, tcg_env);                                  \
    return true;                                                                    \
}

TRANS_QUANT(mquant)
TRANS_QUANT(mdequant)

/************************************
*
*  sophgo copy command: copy content
*  from one tensor register to another.
*
************************************/

static bool trans_sg_mcopy(DisasContext *s, arg_sg_mcopy *arg)
{
    TCGv_ptr src = tcg_temp_new_ptr();
    TCGv_ptr dst = tcg_temp_new_ptr();
    tcg_gen_addi_ptr(src, tcg_env, offsetof(CPURISCVState, treg[arg->ms1].data));
    tcg_gen_addi_ptr(dst, tcg_env, offsetof(CPURISCVState, treg[arg->md].data));
    gen_helper_sg_mcopy(dst, src, tcg_env);
    return true;
}



/************************************
*
*  sophgo float comuputing command: use
*  add or multiple operations on float
*  type data.
*
************************************/

#define TRANS_ADDMUL_COMPUTE(CTX)                                                     \
static bool trans_sg_##CTX(DisasContext *s, arg_sg_##CTX *arg)                       \
{                                                                                    \
    TCGv_ptr src1 = tcg_temp_new_ptr();                                              \
    TCGv_ptr src2 = tcg_temp_new_ptr();                                              \
    TCGv_ptr dst = tcg_temp_new_ptr();                                               \
    tcg_gen_addi_ptr(src1, tcg_env, offsetof(CPURISCVState, treg[arg->ms1].data));   \
    tcg_gen_addi_ptr(src2, tcg_env, offsetof(CPURISCVState, treg[arg->ms2].data));   \
    tcg_gen_addi_ptr(dst, tcg_env, offsetof(CPURISCVState, treg[arg->md].data));     \
    gen_helper_sg_##CTX(dst, src1, src2, tcg_env);                                   \
    return true;                                                                     \
}                                                                                    \

TRANS_ADDMUL_COMPUTE(maddf)
TRANS_ADDMUL_COMPUTE(mmulf)
TRANS_ADDMUL_COMPUTE(madds)

/************************************
*
*  sophgo type transformation command:
*  transfer FP32 to FP16.
*
************************************/

static bool trans_sg_mcvtnf(DisasContext *s, arg_sg_mcvtnf *arg)
{
    TCGv_ptr src = tcg_temp_new_ptr();
    TCGv_ptr dst = tcg_temp_new_ptr();
    tcg_gen_addi_ptr(src, tcg_env, offsetof(CPURISCVState, treg[arg->ms1].data));
    tcg_gen_addi_ptr(dst, tcg_env, offsetof(CPURISCVState, treg[arg->md].data));
    gen_helper_sg_mcvtnf(dst, src, tcg_env);
    return true;
}

static bool trans_sg_fence(DisasContext *s, arg_sg_fence *arg)
{
    // TODO: add fence operation
    return true;
}