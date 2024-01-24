/opt/LLVM/install/bin/clang -static sg_mmaqs.c sg_mmaqs_asm.S -o vcix_qs8  -march=rv64imafdcv_zicsr_zifencei_zfh_zba_zbb_zvfh_xsfvfnrclipxfqf_xsfvfwmaccqqq_xsfvqmaccqoq_xsfvcp_xsgmat --gcc-toolchain=/opt/riscv --sysroot=/opt/riscv/sysroot/ -g
qemu-riscv64 -cpu max,vlen=512 vcix_qs8
rm -rf vcix_qs8