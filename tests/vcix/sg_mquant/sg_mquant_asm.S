.global vcix_mquant_asm
.section .text.vcix_mquant_asm

vcix_mquant_asm:
    # load vset and array to vgpr
    vsetvli t0, a0, e32, m4, tu, mu
    vle32.v v0, (a3)
    sg.m.v m1, v0

    vsetvli t0, a1, e32, m1, tu, mu
    vle32.v v4, (a4)
    sg.m.v m2, v4

    # calculate matrix
    sg.mset mtype, a2, e32, mrv
    sg.mquant m0, m1, m2
    # export back to memory
    vsetvli t0, a0, e8, m1, tu, mu
    sg.v.m v5, m0
    vse8.v v5, (a5)
    ret
