.global vcix_madds_asm
.section .text.vcix_madds_asm

vcix_madds_asm:
    # load from CPR to MPR
    vsetvli t0, a1, e8, m1, tu, mu
    vle8.v v0, (a2)
    vle8.v v1, (a3)
    sg.m.v m0, v0
    sg.m.v m1, v1
    sg.mset mtype, a0, e8, mm
    # add calculation
    sg.madds m2, m0, m1

    # load back to CPR
    sg.v.m v8, m2
    vse8.v v8, (a4)
    ret
