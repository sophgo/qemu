/*
 * RISC-V Matrix Extension Helpers for QEMU.
 *
 */

#include "qemu/osdep.h"
#include "qemu/host-utils.h"
#include "qemu/bitops.h"
#include "cpu.h"
#include "exec/memop.h"
#include "exec/exec-all.h"
#include "exec/cpu_ldst.h"
#include "exec/helper-proto.h"
#include "fpu/softfloat.h"
#include "tcg/tcg-gvec-desc.h"
#include "internals.h"
#include "vector_internals.h"
#include <math.h>
#include <fenv.h>

/************************************
*
*  memory load and store functions: im-
*  plement load and store functions for
*  transpose or not.
*
************************************/

void mld_operation(void *md, target_ulong rs, target_ulong m, target_ulong n, target_ulong stride, target_ulong ew, CPURISCVState *env);
void mld_t_operation(void *md, target_ulong rs, target_ulong m, target_ulong n, target_ulong stride, target_ulong ew, CPURISCVState *env);
void mst_operation(void *ms, target_ulong rs, target_ulong m, target_ulong n, target_ulong stride, target_ulong ew, CPURISCVState *env);
void mst_t_operation(void *ms, target_ulong rs, target_ulong m, target_ulong n, target_ulong stride, target_ulong ew, CPURISCVState *env);

/************************************
*
*  sg_mset related functions: loading
*  system parameters or decompose GPR
*  value to assign the corresponding
*  values to system paramters.
*
************************************/

void HELPER(sg_setv)(target_ulong rs_num, target_ulong type, CPURISCVState *env) {
    switch (type)
    {
        case 0:
            env->tm = rs_num;
            break;
        case 1:
            env->tn = rs_num;
            break;
        case 2:
            env->tk = rs_num;
            break;
        case 3:
            env->stra = rs_num;
            break;
        case 4:
            env->strb = rs_num;
            break;
        case 5:
            env->strc = rs_num;
            break;
        default:
            break;
    }
}

void HELPER(sg_conv)(target_ulong rs, CPURISCVState *env)
{
    uint64_t num = (uint64_t)rs;
    env->strh = (num >> 52) & 0xF;
    env->strw = (num >> 48) & 0xF;
    env->pt = (num >> 44) & 0xF;
    env->pb = (num >> 40) & 0xF;
    env->pl = (num >> 36) & 0xF;
    env->pr = (num >> 32) & 0xF;
    env->ih = (num >> 24) & 0xFF;
    env->iw = (num >> 16) & 0xFF;
    env->oh = (num >> 8) & 0xFF;
    env->ow = num & 0xFF;
}

void HELPER(sg_mnk)(target_ulong rs, CPURISCVState *env) {
    uint64_t num = (uint64_t)rs & 0xFFFFFF;
    env->tk = (num >> 0) & 0xFF;
    env->tn = (num >> 8) & 0xFF;
    env->tm = (num >> 16) & 0xFF;
}

void HELPER(sg_i2c)(target_ulong rs, CPURISCVState *env) {
    uint64_t num = (uint64_t)rs & 0xFFFFFF;
    env->offw= (num >> 0) & 0xFF;
    env->offh = (num >> 8) & 0xFF;
    env->kw = (num >> 16) & 0xF;
    env->kh = (num >> 20) & 0xF;
}

void HELPER(sg_mtype)(target_ulong rs, target_ulong rs_name, target_ulong ew, target_ulong am, CPURISCVState *env) {
    if (rs_name != 0) {
        uint64_t num = (uint64_t)rs;
        target_ulong mstride = (num >> 32) & 0xFFFFFFFF;
        if (mstride != 0) {
            env->m_stride = mstride;
        }
        env->r_size = (num >> 8) & 0xFFFF;
        env->c_size = num & 0xFF;
    }
    env->mew = ew;
    env->amod = am;
}

/************************************
*
*  sg_move: exchange content between
*  riscv vector registers and tensor
*  registers.
*
************************************/

/*****************************************************
 * Encode SEW as follows:
 *     sew    byte width   type
 *      0         1       uint8_t
 *      1         2       uint16_t
 *      2         4     uint32_t & target_ulong (x32)
 *      3         8     uint64_t & target_ulong (x64)
 *
*****************************************************/

void HELPER(sg_move)(void *dst, void *src, CPURISCVState *env)
{
    target_ulong sew = (target_ulong)pow(2, FIELD_EX64(env->vtype, VTYPE, VSEW));
    target_ulong vl= env->vl;
    target_ulong size = vl * sew;
    memcpy(dst, src, size);
    return;
}

/************************************
*
*  sg_m_x: assign one number to the
*  tensor register and broadcast regard-
*  ing vl.
*
************************************/

void HELPER(sg_m_x)(void *dst, target_ulong data, CPURISCVState *env)
{
    target_ulong sew = FIELD_EX64(env->vtype, VTYPE, VSEW);
    target_ulong vl= env->vl;
    for(target_ulong i = 0; i < vl; ++i) {
        switch (sew)
        {
        case 0:
            ((uint8_t*) dst)[i] = (uint8_t)data;
            break;
        case 1:
            ((uint16_t*) dst)[i] = (uint16_t)data;
            break;
        case 2:
            ((uint32_t*) dst)[i] = (uint32_t)data;
            break;
        case 3:
            ((uint64_t*) dst)[i] = (uint64_t)data;
            break;
        default:
            break;
        }
    }
}

/************************************
*
*  sg_m_vvw: copy 4 vector registers'
*  content to one tensor register: 4
*  v gprs with continuous memory space.
*
************************************/

void HELPER(sg_m_vvw)(void *dst, void *src1, void *src2, void *src3, CPURISCVState *env)
{
    target_ulong sew = (target_ulong)pow(2, FIELD_EX64(env->vtype, VTYPE, VSEW));
    target_ulong vl= env->vl;
    uint8_t* tmp_ptr = (uint8_t*)malloc(sizeof(uint8_t) * 256);
    uint8_t* cur = tmp_ptr;
    
    memcpy(cur, src1, sew * vl);
    cur += sew * vl;

    memcpy(cur, src2, sew * vl);
    cur += sew * vl;

    memcpy(cur, src3, sew * vl * 2);
    memcpy(dst, tmp_ptr, sew * vl * 4);
    free(tmp_ptr);
    return;
}

/************************************
*
*  TODO: page probe: probe function.
*
************************************/

/*static void probe_pages(CPURISCVState *env, target_ulong addr,
                        target_ulong len, uintptr_t ra,
                        MMUAccessType access_type)
{
    target_ulong pagelen = -(addr | TARGET_PAGE_MASK);
    target_ulong curlen = MIN(pagelen, len);

    probe_access(env, addr, curlen, access_type,
                 cpu_mmu_index(env, false), ra);
    if (len > curlen) {
        addr += curlen;
        curlen = len - curlen;
        probe_access(env, addr, curlen, access_type,
                     cpu_mmu_index(env, false), ra);
    }
}*/


/************************************
*
*  sg_mld: load matrix data from memory
*  to tensor registers:
*  1) md: register matrix location
*  2) rs: address of memory
*  3) m, n: dimensions of matrix
*  4) stride: stride between rows
*  5) env: CPU content
*  @note: For computing which signs default
*  0x0 value of matrix type, assign mtype
*  parameters for the loading functions.
*
************************************/

void mld_operation(void *md, target_ulong rs, target_ulong m, target_ulong n, target_ulong stride, target_ulong ew, CPURISCVState *env)
{
    target_ulong addr = 0;
    target_ulong sew = ew;
    target_ulong storage_stride = (target_ulong)pow(2, sew);
    for (target_ulong i = 0; i < m; i++)
    {
        for (target_ulong j = 0; j < n; j++)
        {
            addr = rs + (i * stride + j) * storage_stride;
            switch (sew)
            {
            case 0:
                ((int8_t *)md)[i * n + j] = cpu_ldsb_data(env, addr);
                break;
            case 1:
                ((int16_t *)md)[i * n + j] = cpu_ldsw_data(env, addr);
                break;
            case 2:
                ((int32_t *)md)[i * n + j] = cpu_ldl_data(env, addr);
                break;
            case 3:
                ((int64_t *)md)[i * n + j] = cpu_ldq_data(env, addr);
                break;
            default:
                break;
            }
        }
    }
}

void mld_t_operation(void *md, target_ulong rs, target_ulong m, target_ulong n, target_ulong stride, target_ulong ew, CPURISCVState *env)
{
    target_ulong addr = 0;
    target_ulong sew = ew;
    target_ulong storage_stride = (target_ulong)pow(2, sew);
    for (target_ulong i = 0; i < m; i++)
    {
        for (target_ulong j = 0; j < n; j++)
        {
            addr = rs + (j * m + i) * storage_stride;
            switch (sew)
            {
            case 0:
                ((int8_t *)md)[i * n + j] = cpu_ldsb_data(env, addr);
                break;
            case 1:
                ((int16_t *)md)[i * n + j] = cpu_ldsw_data(env, addr);
                break;
            case 2:
                ((int32_t *)md)[i * n + j] = cpu_ldl_data(env, addr);
                break;
            case 3:
                ((int64_t *)md)[i * n + j] = cpu_ldq_data(env, addr);
                break;
            default:
                break;
            }
        }
    }
}

void HELPER(default_mld) (void *md, target_ulong rs, target_ulong ew, CPURISCVState *env) {
    mld_operation(md, rs, env->r_size, env->c_size, env->m_stride, ew, env);
}

void HELPER(sg_mld) (void *md, target_ulong rs, target_ulong m, target_ulong n, target_ulong stride, target_ulong ew, CPURISCVState *env) {
    mld_operation(md, rs, m, n, stride, ew, env);
}

// load by transpose
void HELPER(default_mld_t) (void *md, target_ulong rs, target_ulong ew, CPURISCVState *env) {
    mld_t_operation(md, rs, env->c_size, env->r_size, env->m_stride, ew, env);
}

void HELPER(sg_mld_t)(void *md, target_ulong rs, target_ulong m, target_ulong n, target_ulong stride, target_ulong ew, CPURISCVState *env)
{
    mld_t_operation(md, rs, m, n, stride, ew, env);
}

/************************************
*
*  sg_mst: store matrix data to memory
*  from tensor registers:
*  1) ms: register matrix location
*  2) rs: address of memory
*  3) m, n: dimensions of matrix
*  4) stride: stride between rows
*  5) env: CPU content
*  @note: For computing which signs default
*  0x0 value of matrix type, assign mtype
*  parameters for the storing functions.
*
************************************/

void mst_operation(void *ms, target_ulong rs, target_ulong m, target_ulong n, target_ulong stride, target_ulong ew, CPURISCVState *env)
{
    target_ulong addr = 0;
    target_ulong sew = ew;
    target_ulong storage_stride = (target_ulong)pow(2, sew);
    for (target_ulong i = 0; i < m; i++)
    {
        for (target_ulong j = 0; j < n; j++)
        {
            addr = rs + (i * stride + j) * storage_stride;
            switch (sew)
            {
            case 0:
                cpu_stb_data(env, addr, ((int8_t *)ms)[i * n + j]);
                break;
            case 1:
                cpu_stw_data(env, addr, ((int16_t *)ms)[i * n + j]);
                break;
            case 2:
                cpu_stl_data(env, addr, ((int32_t *)ms)[i * n + j]);
                break;
            case 3:
                cpu_stq_data(env, addr, ((int64_t *)ms)[i * n + j]);
                break;
            default:
                break;
            }
        }
    }
}

void mst_t_operation(void *ms, target_ulong rs, target_ulong m, target_ulong n, target_ulong stride, target_ulong ew, CPURISCVState *env)
{
    target_ulong addr = 0;
    target_ulong sew = ew;
    target_ulong storage_stride = (target_ulong)pow(2, sew);
    for (target_ulong i = 0; i < m; i++)
    {
        for (target_ulong j = 0; j < n; j++)
        {
            addr = rs + (j * m + i) * storage_stride;
            switch (sew)
            {
            case 0:
                cpu_stb_data(env, addr, ((int8_t *)ms)[i * n + j]);
                break;
            case 1:
                cpu_stw_data(env, addr, ((int16_t *)ms)[i * n + j]);
                break;
            case 2:
                cpu_stl_data(env, addr, ((int32_t *)ms)[i * n + j]);
                break;
            case 3:
                cpu_stq_data(env, addr, ((int64_t *)ms)[i * n + j]);
                break;
            default:
                break;
            }
        }
    }
}

void HELPER(default_mst) (void *ms, target_ulong rs, target_ulong ew, CPURISCVState *env) {
    mst_operation(ms, rs, env->r_size, env->c_size, env->m_stride, ew, env);
}

void HELPER(sg_mst)(void *ms, target_ulong rs, target_ulong m, target_ulong n, target_ulong stride, target_ulong ew, CPURISCVState *env)
{
    mst_operation(ms, rs, m, n, stride, ew, env);
}

// store by transpose
void HELPER(default_mst_t) (void *ms, target_ulong rs, target_ulong ew, CPURISCVState *env) {
    mst_t_operation(ms, rs, env->c_size, env->r_size, env->m_stride, ew, env);
}
void HELPER(sg_mst_t)(void *ms, target_ulong rs, target_ulong m, target_ulong n, target_ulong stride, target_ulong ew, CPURISCVState *env)
{
    mst_t_operation(ms, rs, m, n, stride, ew, env);
}

/************************************
*
*  sg_mma type: matrix multiplication.
*  1) a: left matrix
*  2) b: right matrix
*  3) c: result matrix
*  4) is_accum: whether to reuse the
*   accumulator
*  5) env: CPU state
*
************************************/

void HELPER(sg_mmaqs)(void *c, void *a, void *b,
                      target_ulong is_accum, CPURISCVState *env)
{
    target_ulong m = env->tm;
    target_ulong n = env->tn;
    target_ulong k = env->tk;
    void* acc = env->accumulator;
    if(!is_accum) {
        for(target_ulong i = 0; i < m; ++i) {
            for(target_ulong j = 0; j < n; ++j) {
                ((int32_t*)acc)[i * n + j] = ((int32_t*)c)[i * n + j];
            }
        }
    }
    for (target_ulong i = 0; i < m; i++)
    {
        for (target_ulong j = 0; j < n; j++)
        {
            for (target_ulong l = 0; l < k; l++)
            {
                ((int32_t*)acc)[i * n + j] += (int32_t)((int8_t *)a)[i * k + l] * (int32_t)((int8_t *)b)[j * k + l];
            }
            ((int32_t *)c)[i * n + j] = ((int32_t*)acc)[i * n + j];
        }
    }

    return;
}

void HELPER(sg_mmqs)(void *c, void *a, void *b, CPURISCVState *env)
{
    target_ulong m = env->tm;
    target_ulong n = env->tn;
    target_ulong k = env->tk;
    int32_t acc[16][16] = {0};
    for (target_ulong i = 0; i < m; i++)
    {
        for (target_ulong j = 0; j < n; j++)
        {
            for (target_ulong l = 0; l < k; l++)
            {
                acc[i][j] += (int32_t)((int8_t *)a)[i * k + l] * (int32_t)((int8_t *)b)[j * k + l];
            }
            ((int32_t *)c)[i * n + j] = acc[i][j];
        }
    }
    return;
}

/************************************
*
*  @note: for float type computing in
*  qemu, we manipulate them by qemu
*  intrinsic methods, which corresponds
*  to float16 and float32 type, mainly
*  including add and multiply ops.
*
************************************/

void HELPER(sg_mmaf)(void *c, void *a, void *b,
                     target_ulong is_accum, CPURISCVState *env)
{
    target_ulong m = env->tm;
    target_ulong n = env->tn;
    target_ulong k = env->tk;
    uint16_t oprd_a, oprd_b;
    void* acc = env->accumulator;
    if(!is_accum) {
        for(target_ulong i = 0; i < m; ++i) {
            for(target_ulong j = 0; j < n; ++j) {
                ((float16*)acc)[i * n + j] = ((float16*)c)[i * n + j];
            }
        }
    }

    for (target_ulong i = 0; i < m; i++)
    {
        for (target_ulong j = 0; j < n; j++)
        {
            for (target_ulong l = 0; l < k; l++)
            {
                oprd_a = ((float16 *)a)[i * k + l];
                oprd_b = ((float16 *)b)[j * k + l];
                ((float16*)acc)[i * n + j] = float16_muladd(oprd_a, oprd_b, ((float16*)acc)[i * n + j], 0, &env->fp_status);
            }
            ((float16 *)c)[i * n + j] = ((float16*)acc)[i * n + j];
        }
    }
    return;
}

void HELPER(sg_mmf)(void *c, void *a, void *b, CPURISCVState *env)
{
    target_ulong m = env->tm;
    target_ulong n = env->tn;
    target_ulong k = env->tk;
    uint16_t oprd_a, oprd_b;
    uint16_t acc[16][16] = {0};

    for (target_ulong i = 0; i < m; i++)
    {
        for (target_ulong j = 0; j < n; j++)
        {
            for (target_ulong l = 0; l < k; l++)
            {
                oprd_a = ((float16 *)a)[i * k + l];
                oprd_b = ((float16 *)b)[j * k + l];
                acc[i][j] = float16_muladd(oprd_a, oprd_b, acc[i][j], 0, &env->fp_status);
            }
            ((float16 *)c)[i * n + j] = acc[i][j];
        }
    }
    return;
}

void HELPER(sg_mmawf)(void *c, void *a, void *b,
                      target_ulong is_accum, CPURISCVState *env)
{
    target_ulong m = env->tm;
    target_ulong n = env->tn;
    target_ulong k = env->tk;
    uint32_t oprd_a, oprd_b;
    void* acc = env->accumulator;
    if(!is_accum) {
        for(target_ulong i = 0; i < m; ++i) {
            for(target_ulong j = 0; j < n; ++j) {
                ((float32*)acc)[i * n + j] = ((float32*)c)[i * n + j];
            }
        }
    }
    for (target_ulong i = 0; i < m; i++)
    {
        for (target_ulong j = 0; j < n; j++)
        {
            for (target_ulong l = 0; l < k; l++)
            {
                oprd_a = float16_to_float32(((float16 *)a)[i * k + l], true, &env->fp_status);
                oprd_b = float16_to_float32(((float16 *)b)[j * k + l], true, &env->fp_status);
                ((float32*)acc)[i * n + j] = float32_muladd(oprd_a, oprd_b, ((float32*)acc)[i * n + j], 0, &env->fp_status);
            }
            ((float32 *)c)[i * n + j] = ((float32*)acc)[i * n + j];
        }
    }
    return;
}

void HELPER(sg_mmwf)(void *c, void *a, void *b, CPURISCVState *env)
{
    target_ulong m = env->tm;
    target_ulong n = env->tn;
    target_ulong k = env->tk;
    uint32_t oprd_a, oprd_b;
    uint32_t acc[16][16] = {0};
    for (target_ulong i = 0; i < m; i++)
    {
        for (target_ulong j = 0; j < n; j++)
        {
            for (target_ulong l = 0; l < k; l++)
            {
                oprd_a = float16_to_float32(((float16 *)a)[i * k + l], true, &env->fp_status);
                oprd_b = float16_to_float32(((float16 *)b)[j * k + l], true, &env->fp_status);
                acc[i][j] = float32_muladd(oprd_a, oprd_b, acc[i][j], 0, &env->fp_status);
            }
            ((float32 *)c)[i * n + j] = acc[i][j];
        }
    }
    return;
}

/************************************
*
*  sg_mquant type: matrix quantification.
*  1) src1: source matrix as int32 type
*  2) src2: scale matrix as float32 type
*  3) dst: output matrix as int8 type
*
************************************/

void HELPER(sg_mquant)(void *dst, void *src1, void *src2, CPURISCVState *env)
{
    assert(env->mew == 2);
    for(target_ulong row = 0; row < env->r_size; ++row) {
        for(target_ulong col = 0; col < env->c_size; ++col) {
            target_ulong idx = row * env->c_size + col;
            float32 scale = 0;
            int32_t a = ((int32_t*)src1)[idx];
            if(env->amod == 0x1) {
                scale = ((float32*)src2)[row];
            } else if(env->amod == 0x2) {
                scale = ((float32*)src2)[col];
            } else {
                assert(0);
            }
            float32 oprd = int32_to_float32(a, &env->fp_status);
            int32_t c = float32_to_int32_round_to_zero(float32_mul(oprd, scale, &env->fp_status), &env->fp_status);
            c = (c <= INT8_MAX ? c : INT8_MAX);
            c = (c >= INT8_MIN ? c : INT8_MIN);
            ((int8_t*)dst)[idx] = (int8_t)c;
        }
    }
}

/************************************
*
*  sg_mdequant type: matrix dequantifi-
*  cation.
*  1) src1: source matrix as int8 type
*  2) src2: scale matrix as float16 type
*  3) dst: output matrix as float16 type
*
************************************/

void HELPER(sg_mdequant)(void *dst, void *src1, void *src2, CPURISCVState *env)
{
    assert(env->mew == 0 || env->mew == 1);
    for(target_ulong row = 0; row < env->r_size; ++row) {
        for(target_ulong col = 0; col < env->c_size; ++col) {
            float16 scale = 0;
            float16 zp = 0;
            target_ulong idx = row * env->c_size + col;
            if(env->amod == 0x1) {
                scale = ((float16*)src2)[row * 2];
                zp = ((float16*)src2)[row * 2 + 1];
            } else {
                assert(0);
            }
            float16 oprd_a0 = 0;
            float16 oprd_a1 = 0;
            float16 oprd_scale = scale;
            float16 oprd_zp = zp;
            if(env->mew == 0) {
                int8_t a = ((int8_t*)src1)[idx];
                int8_t a0 = a >> 4;
                int8_t a1 = a & 0x0F;
                oprd_a0 = int8_to_float16(a0, &env->fp_status);
                oprd_a1 = int8_to_float16(a1, &env->fp_status);
            } else {
                int16_t a = ((int16_t*)src1)[idx];
                int8_t a0 = (int8_t) (a>> 8);
                int8_t a1 = (int8_t) (a & 0xFF);
                oprd_a0 = int8_to_float16(a0, &env->fp_status);
                oprd_a1 = int8_to_float16(a1, &env->fp_status);
            }
            ((float16*)dst)[idx * 2] = float16_muladd(oprd_a0, oprd_scale, oprd_zp, 0, &env->fp_status);
            ((float16*)dst)[idx * 2 + 1] = float16_muladd(oprd_a1, oprd_scale, oprd_zp, 0, &env->fp_status);
        }
    }
}

/************************************
*
*  sg_mimg2col type: matrix dimension change.
*  1) dst: result matrix
*  2) src: source matrix
*  3) env: CPU state
*
************************************/

void HELPER(sg_img2col)(void *dst, void *src, CPURISCVState *env) {
    int kernel_h, kernel_w, stride_h, stride_w,
    pad_left, pad_up, input_h, input_w, output_h, output_w, offset_w, offset_h;

    target_ulong sew = env->mew;

    kernel_h = env->kh; kernel_w = env->kw; stride_h = env->strh;
    stride_w = env->strw; pad_left = env->pl; pad_up = env->pt;
    input_h = env->ih; input_w = env->iw;
    output_h = env->oh; output_w = env->ow;
    offset_w = env->offw; offset_h = env->offh;

    for (target_ulong row = 0; row < output_h; ++row) {
        for (target_ulong col = 0; col < output_w; ++col) {
            for (target_ulong m = 0; m < kernel_h; ++m) {
                if((int)m < offset_h) continue;
                for (target_ulong n = 0; n < kernel_w; ++n) {
                    if((int)n < offset_w) continue;

                    int img_row = row * stride_h - pad_up + m;
                    int img_col = col * stride_w - pad_left + n;
                    int input_index = img_row * input_w + img_col;
                    int output_index = ((row * output_w + col) * kernel_h + m) * kernel_w + n;
                        if (img_row >= 0 && img_row < input_h && img_col >= 0 && img_col < input_w) {
                            switch (sew)
                            {
                            case 0:
                                ((int8_t*)dst)[output_index] = ((int8_t*)src)[input_index];
                                break;
                            case 1:
                                ((int16_t*)dst)[output_index] = ((int16_t*)src)[input_index];
                                break;
                            case 2:
                                ((int32_t*)dst)[output_index] = ((int32_t*)src)[input_index];
                                break;
                            case 3:
                                ((int64_t*)dst)[output_index] = ((int64_t*)src)[input_index];
                                break;
                            default:
                                break;
                            }
                        } else {
                            switch (sew)
                            {
                            case 0:
                                ((int8_t*)dst)[output_index] = 0;
                                break;
                            case 1:
                                ((int16_t*)dst)[output_index] = 0;
                                break;
                            case 2:
                                ((int32_t*)dst)[output_index] = 0;
                                break;
                            case 3:
                                ((int64_t*)dst)[output_index] = 0;
                                break;
                            default:
                                break;
                            }
                        }
                }
            }
        }
    }
}

/************************************
*
*  float add type: float add operation.
*  1) dst: result matrix
*  2) src1: source matrix 1
*  3) src2: source matrix 2
*  4) env: CPU state
*
************************************/

void HELPER(sg_maddf)(void *dst, void *src1, void *src2, CPURISCVState *env) {
    target_ulong sew = env->mew;
    for(target_ulong row = 0; row < env->r_size; ++row) {
        for(target_ulong col = 0; col < env->c_size; ++col) {
            target_ulong idx = row * env->c_size + col;
            switch (sew)
            {
            case 1:
                float16 b16;
                switch (env->amod)
                {
                case 0x0:
                    b16 = ((float16*)src2)[idx];
                    break;
                case 0x1:
                    b16 = ((float16*)src2)[row];
                    break;
                default:
                    b16 = ((float16*)src2)[col];
                    break;
                }
                ((float16*)dst)[idx] = float16_add(((float16*)src1)[idx], b16, &env->fp_status);
                break;
            case 2:
                float32 b32;
                switch (env->amod)
                {
                case 0x0:
                    b32 = ((float32*)src2)[idx];
                    break;
                case 0x1:
                    b32 = ((float32*)src2)[row];
                    break;
                default:
                    b32 = ((float32*)src2)[col];
                    break;
                }
                ((float32*)dst)[idx] = float32_add(((float32*)src1)[idx], b32, &env->fp_status);
                break;
            default:
                break;
            }
        }
    }
}

/************************************
*
*  integer add type: integer add operation.
*  1) dst: result matrix
*  2) src1: source matrix 1
*  3) src2: source matrix 2
*  4) env: CPU state
*
************************************/

void HELPER(sg_madds)(void *dst, void *src1, void *src2, CPURISCVState *env) {
    target_ulong sew = env->mew;
    for(target_ulong row = 0; row < env->r_size; ++row) {
        for(target_ulong col = 0; col < env->c_size; ++col) {
            target_ulong idx = row * env->c_size + col;
            switch (sew)
            {
            case 0:
                int8_t b8;
                switch (env->amod)
                {
                case 0x0:
                    b8 = ((int8_t*)src2)[idx];
                    break;
                case 0x1:
                    b8 = ((int8_t*)src2)[row];
                    break;
                default:
                    b8 = ((int8_t*)src2)[col];
                    break;
                }
                ((int8_t*)dst)[idx] = b8 + ((int8_t*)src1)[idx];
                break;
            case 1:
                int16_t b16;
                switch (env->amod)
                {
                case 0x0:
                    b16 = ((int16_t*)src2)[idx];
                    break;
                case 0x1:
                    b16 = ((int16_t*)src2)[row];
                    break;
                default:
                    b16 = ((int16_t*)src2)[col];
                    break;
                }
                ((int16_t*)dst)[idx] = b16 + ((int16_t*)src1)[idx];
                break;
            case 2:
                int32_t b32;
                switch (env->amod)
                {
                case 0x0:
                    b32 = ((int32_t*)src2)[idx];
                    break;
                case 0x1:
                    b32 = ((int32_t*)src2)[row];
                    break;
                default:
                    b32 = ((int32_t*)src2)[col];
                    break;
                }
                ((int32_t*)dst)[idx] = b32 + ((int32_t*)src1)[idx];
                break;
            case 3:
                int64_t b64;
                switch (env->amod)
                {
                case 0x0:
                    b64 = ((int64_t*)src2)[idx];
                    break;
                case 0x1:
                    b64 = ((int64_t*)src2)[row];
                    break;
                default:
                    b64 = ((int64_t*)src2)[col];
                    break;
                }
                ((int64_t*)dst)[idx] = b64 + ((int64_t*)src1)[idx];
                break;
            default:
                break;
            }
        }
    }
}

/************************************
*
*  float mul type: float multipy ope-
*  ration.
*  1) dst: result matrix
*  2) src1: source matrix 1
*  3) src2: source matrix 2
*  4) env: CPU state
*
************************************/

void HELPER(sg_mmulf)(void *dst, void *src1, void *src2, CPURISCVState *env) {
    target_ulong sew = env->mew;
    for(target_ulong row = 0; row < env->r_size; ++row) {
        for(target_ulong col = 0; col < env->c_size; ++col) {
            target_ulong idx = row * env->c_size + col;
            switch(sew) {
                case 1:
                    float16 b16;
                    switch (env->amod)
                    {
                    case 0x0:
                        b16 = ((float16*)src2)[idx];
                        break;
                    case 0x1:
                        b16 = ((float16*)src2)[row];
                        break;
                    default:
                        b16 = ((float16*)src2)[col];
                        break;
                    }
                    ((float16*)dst)[idx] = float16_mul(((float16*)src1)[idx], b16, &env->fp_status);
                    break;
                case 2:
                    float32 b32;
                    switch (env->amod)
                    {
                    case 0x0:
                        b32 = ((float32*)src2)[idx];
                        break;
                    case 0x1:
                        b32 = ((float32*)src2)[row];
                        break;
                    default:
                        b32 = ((float32*)src2)[col];
                        break;
                    }
                    ((float32*)dst)[idx] = float32_mul(((float32*)src1)[idx], b32, &env->fp_status);
                    break;
                default:
                    break;
            }
        }
    }
}

/************************************
*
*  float trans type: FP32 -> FP16
*  1) dst: result matrix
*  2) src1: source matrix
*  3) env: CPU state
*
************************************/

void HELPER(sg_mcvtnf)(void *dst, void *src, CPURISCVState *env) {
    for(target_ulong i = 0; i < env->r_size * env->c_size; ++i) {
        ((float16*)dst)[i] = float32_to_float16(((float32*)src)[i], true, &env->fp_status);
    }
}

/************************************
*
*  matrix copy type: copy matrix data
*  1) dst: result matrix
*  2) src: source matrix
*  3) env: CPU state
*
************************************/


void HELPER(sg_mcopy) (void *dst, void *src, CPURISCVState *env) {
    target_ulong sew = env->mew;
    target_ulong size = env->r_size * env->c_size;
    memcpy(dst, src, size * (1 << sew));
}

/************************************
 *  matrix transpose type: transpose matrix data
 * 1) dst: result matrix
 * 2) src: source matrix
 * 3) env: CPU state
 * **********************************/

void HELPER(sg_mtrans) (void *dst, void *src, target_ulong ew, CPURISCVState *env) {
    target_ulong sew = ew;
    target_ulong m = env->r_size;
    target_ulong n = env->c_size;
    for(target_ulong i = 0; i < m; ++i) {
        for(target_ulong j = 0; j < n; ++j) {
            target_ulong idx = i * n + j;
            target_ulong idx_t = j * m + i;
            switch (sew)
            {
            case 0:
                ((uint8_t*)dst)[idx_t] = ((uint8_t*)src)[idx];
                break;
            case 1:
                ((uint16_t*)dst)[idx_t] = ((uint16_t*)src)[idx];
                break;
            case 2:
                ((uint32_t*)dst)[idx_t] = ((uint32_t*)src)[idx];
                break;
            case 3:
                ((uint64_t*)dst)[idx_t] = ((uint64_t*)src)[idx];
                break;
            default:
                break;
            }
        }
    }
}