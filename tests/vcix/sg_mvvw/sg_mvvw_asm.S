.global vcix_mvvw_asm
.section .text.vcix_mvvw_asm

vcix_mvvw_asm:
    # load three vectors from GPR to VR
    vsetvli t0, a0, e16, m1, tu, mu
    vle16.v v4, (a3)
    vsetvli t0, a0, e8, m1, tu, mu
    vle8.v v2, (a1)
    vle8.v v3, (a2)
    
    # test vvw
    sg.m.vvw m4, v2

    # load data back to GPR
    vsetvli t0, a6, e8, m1, tu, mu
    sg.v.m v8, m4
    vse8.v v8, (a4)

    vsetvli t0, a6, e16, m2, tu, mu
    sg.v.m v12, m4
    vse16.v v12, (a5)

    ret
