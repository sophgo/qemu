.global vcix_mstt_asm
.section .text.vcix_mstt_asm

vcix_mstt_asm:
    # set M and N and stride
    sg.mset tilem, a0
    sg.mset tilen, a1
    sg.mset strc, a1

    # load from CPR
    vsetvli t0, a2, e8, m1, tu, mu
    vle8.v v0, (a3)
    sg.m.v m0, v0

    # load MPR to memory
    sg.mst.t a4, m0, e8, c

    ret
