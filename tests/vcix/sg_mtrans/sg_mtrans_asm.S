.global vcix_mtrans_asm
.section .text.vcix_mtrans_asm

vcix_mtrans_asm:
    # load from CPR to MPR
    vsetvli t0, a1, e32, m4, tu, mu
    vle32.v v0, (a2)
    sg.m.v m0, v0

    sg.mset mtype, a0, e32, mm
    sg.mtrans m2, m0, e32

    # load back to CPR
    sg.v.m v8, m2
    vse32.v v8, (a3)
    ret
