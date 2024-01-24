.global vcix_mimg2col_asm
.section .text.vcix_mimg2col_asm

vcix_mimg2col_asm:
    # load vset and array to vgpr
    vsetvli t0, a0, e8, m8, tu, mu
    vle8.v v0, (a4)

    # set conv parameters
    sg.mset conv, a2
    sg.mset img2col, a3

    # load to matrix gpr
    sg.m.v m0, v0

    # calculate matrix
    sg.img2col m1, m0, e8
    # export back to memory

    vsetvli t0, a1, e8, m8, tu, mu
    sg.v.m v8, m1
    vse8.v v8, (a5)
    ret
