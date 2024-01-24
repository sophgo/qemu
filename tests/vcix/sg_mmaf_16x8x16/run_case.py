import numpy as np
import subprocess
import os
import warnings
import sys

CASE_NUMBER = 200
os.system("/opt/LLVM/install/bin/clang -static sg_mmaf.c sg_mmaf_asm.S -o sg_mmaf -O3 -march=rv64imafdcv_zicsr_zifencei_zfh_zba_zbb_zvfh_xsfvfnrclipxfqf_xsfvfwmaccqqq_xsfvqmaccqoq_xsfvcp_xsgmat --gcc-toolchain=/opt/riscv --sysroot=/opt/riscv/sysroot/ -g")

# data configuration
matrix_size = 128  # set matrix size as 16*8=128
executable_path = './sg_mmaf'  # excutable file
input_file_path = 'input_matrices.bin'  # input matrix
output_file_path = 'output_result.txt'
def get_cpu_list():
    result = subprocess.run(["qemu-riscv64", "-cpu", "help"], capture_output=True, text=True)
    output_lines = result.stdout.split('\n')
    cpu_list = [line.strip() for line in output_lines if line.strip()]
    return cpu_list[1:]  # Skip the first line "Available CPUs:"
cpu_list = get_cpu_list()

correct_case_num = 0
for i in range(CASE_NUMBER):
    # generate random matrix

    a = np.random.rand(16, 8).astype(np.float16)
    b = np.random.rand(8, 16).astype(np.float16)
    c = np.dot(a, b).astype(np.float16)  # expected output
    # store matrices into the file
    with open(input_file_path, 'wb') as f:
        a.tofile(f)
        b.T.tofile(f)
        c.tofile(f)
    # run simulation
    if 'sifive-x280' in cpu_list:
        result = subprocess.run(["qemu-riscv64", "-cpu", "sifive-x280", executable_path, input_file_path, output_file_path], capture_output=True, text=True)
    else:
        result = subprocess.run(["qemu-riscv64", "-cpu", "max,vlen=512", executable_path, input_file_path, output_file_path], capture_output=True, text=True)

    # exam the output
    if result.returncode == 0 and 'passed' in result.stdout:
        correct_case_num += 1
    else:
        print("====================================")
        print("\033[31m Test failed for case {}, details will be printed below: ".format(i + 1))
        print("\033[34m left matrix:")
        print(a)
        print("\033[34m right matrix:")
        print(b.T)
        print("\033[34m standard result matrix:")
        print(c)
        print("\033[36m qemu result matrix:")
        print(result.stdout)
        print("====================================")

# clear the file
os.remove(input_file_path)
os.remove(executable_path)
per = correct_case_num/CASE_NUMBER*100
if per > 99:
    print(f"Correct percentage: \033[32m{per}%")
elif per > 80:
    print(f"Correct percentage: \033[33m{per}%")
else:
    print(f"Correct percentage: \033[31m{per}%")
