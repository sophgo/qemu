.global vcix_mma8_asm
.section .text.vcix_mma8_asm

vcix_mma8_asm:
    # load vset---
    vsetivli t0, 8, e8, m1, tu, mu

    # load stride
    sg.mset stra, a2
    sg.mset strb, a2
    sg.mset strc, a1

    # load m, n, k
    sg.mset tilem, a0
    sg.mset tilen, a1
    sg.mset tilek, a2

    # load a, b matrix
    sg.mld m0, a3, e8, a
    sg.mld m1, a4, e8, b

    # calculate matrix
    sg.mmaqs m2, m0, m1, nreuse

    # load vset---
    # export c back to memory
    vsetivli t0, 8, e32, m1, tu, mu
    sg.mst a5, m2, e32, c
    ret
