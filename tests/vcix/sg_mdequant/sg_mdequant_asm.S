.global vcix_mdequant_asm
.section .text.vcix_mdequant_asm

vcix_mdequant_asm:
    # load vset and array to vgpr
    vsetvli t0, a0, e8, m1, tu, mu
    vle8.v v0, (a4)
    sg.m.v m1, v0

    vsetvli t0, a1, e16, m1, tu, mu
    vle16.v v4, (a5)
    sg.m.v m2, v4

    # calculate matrix
    sg.mset mtype, a3, e8, mrv
    sg.mdequant m0, m1, m2
    # export back to memory
    vsetvli t0, a2, e16, m4, tu, mu
    sg.v.m v8, m0
    vse16.v v8, (a6)
    ret
