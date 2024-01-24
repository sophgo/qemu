.global vcix_mldt_asm
.section .text.vcix_mldt_asm

vcix_mldt_asm:
    # set M and N and stride
    sg.mset tilem, a0
    sg.mset tilen, a1
    sg.mset strc, a1

    # load from memory to MPR with transpose
    vsetvli t0, a2, e8, m1, tu, mu
    sg.mld.t m0, a3, e8, c

    # load back by VPR
    sg.v.m v0, m0

    # load back to CPR
    vse8.v v0, (a4)
    ret
