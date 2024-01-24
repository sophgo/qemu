.global vcix_mconvqs_asm
.section .text.vcix_mconvqs_asm

vcix_mconvqs_asm:
    # load vset---
    vsetivli t0, 9, e8, m1, tu, mu

    # load stride
    sg.mset strc, a1

    # load m, n
    sg.mset tilem, a0
    sg.mset tilen, a1
    # load conv
    sg.mset conv, a4

    #load kernel
    vle8.v v0, (a6)
    sg.m.v m0, v0

    # load input matrix
    sg.mld m1, a5, c

    # calculate matrix
    sg.mconvqs m5, m0, m1

    # load vset---
    # load m, n
    sg.mset tilem, a2
    sg.mset tilen, a3
    # export c back to memory
    vsetivli t0, 9, e32, m1, tu, mu
    sg.mset strc, a3
    sg.mst a7, m5, c
    ret
