.global vcix_mma8_asm
.section .text.vcix_mma8_asm
vcix_mma8_asm:
    vsetivli t0, 6, e8, m1, tu, mu
    vle8.v v1, (a3)
    sg.m.v m1, v1

    vsetivli t0, 8, e8, m1, tu, mu
    vle8.v v2, (a4)
    sg.m.v m2, v2

    # load m, n, k
    sg.mset tilem, a0
    sg.mset tilen, a1
    sg.mset tilek, a2

    # calculate matrix
    sg.mmaqs m3, m1, m2
    vsetivli t0, 12, e32, m1, tu, mu
    sg.v.m v0, m3
    vse32.v v0, (a5)
    ret
