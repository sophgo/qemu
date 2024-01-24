.global vcix_mx_asm
.section .text.vcix_mx_asm

vcix_mx_asm:
    # load vset---
    vsetvli t0, a2, e8, m2, tu, mu

    # use sg.m.x
    sg.m.x m0, a0

    # export to vector
    vsetvli t0, a2, e8, m2, tu, mu
    sg.v.m v0, m0
    vse8.v v0, (a1)
    ret
