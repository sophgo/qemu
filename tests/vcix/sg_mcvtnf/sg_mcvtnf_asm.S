.global vcix_mcvtnf_asm
.section .text.vcix_mcvtnf_asm

vcix_mcvtnf_asm:
    # load from CPR to MPR
    vsetvli t0, a1, e32, m4, tu, mu
    vle32.v v0, (a2)
    sg.m.v m0, v0
    sg.mset mtype, a0, e16, mm
    # transfer format
    sg.mcvtnf m1, m0
    # load back to CPR
    vsetvli t0, a0, e16, m2, tu, mu
    sg.v.m v8, m1
    vse16.v v8, (a3)
    ret
