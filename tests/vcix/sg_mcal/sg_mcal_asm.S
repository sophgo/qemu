.global vcix_mcal_asm
.section .text.vcix_mcal_asm

vcix_mcal_asm:
    # load from CPR to MPR
    vsetvli t0, a1, e16, m2, tu, mu
    sg.mset mtype, a0, e16, mm
    vle16.v v0, (a2)
    vle16.v v2, (a3)
    sg.m.v m0, v0
    sg.m.v m1, v2

    # mul and add calculation
    sg.mset mtype, a0, e16, mm
    sg.mmulf m2, m0, m1
    sg.maddf m3, m0, m1

    # load back to CPR
    sg.v.m v8, m2
    sg.v.m v10, m3
    vse16.v v8, (a4)
    vse16.v v10, (a5)
    ret
