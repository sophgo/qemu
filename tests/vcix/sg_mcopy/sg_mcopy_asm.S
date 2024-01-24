.global vcix_mcopy_asm
.section .text.vcix_mcopy_asm

vcix_mcopy_asm:
    # load vset---
    vsetivli t0, 30, e8, m1, tu, mu

    #load input
    vle8.v v0, (a1)
    sg.m.v m0, v0
    sg.mset mtype, a0, e8, mm
    # copy matrix
    sg.mcopy m2, m0

    sg.v.m v1, m2
    vse8.v v1, (a2)
    ret
