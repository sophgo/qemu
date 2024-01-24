import numpy as np
import subprocess
import os
R_SIZE = 8
C_SIZE = 6
CASE_NUMBER = 200
os.system("/opt/LLVM/install/bin/clang -static sg_mquant.c sg_mquant_asm.S -o sg_mquant -O3 -march=rv64imafdcv_zicsr_zifencei_zfh_zba_zbb_zvfh_xsfvfnrclipxfqf_xsfvfwmaccqqq_xsfvqmaccqoq_xsfvcp_xsgmat --gcc-toolchain=/opt/riscv --sysroot=/opt/riscv/sysroot/ -g")

# data configuration
executable_path = './sg_mquant'  # excutable file
input_file_path = 'input_matrices.bin'  # input matrix
output_file_path = 'output_result.txt'
correct_case_num = 0

def get_cpu_list():
    result = subprocess.run(["qemu-riscv64", "-cpu", "help"], capture_output=True, text=True)
    output_lines = result.stdout.split('\n')
    cpu_list = [line.strip() for line in output_lines if line.strip()]
    return cpu_list[1:]  # Skip the first line "Available CPUs:"
cpu_list = get_cpu_list()

def mquant(md, ms1, ms2, Amode):
    ptr_a = ms1.astype(np.int32)
    ptr_b = ms2.astype(np.float32)
    ptr_c = md.astype(np.int8)
    Rsize, Csize = (R_SIZE, C_SIZE)

    for row in range(Rsize):
        for col in range(Csize):
            scale = 0.0
            idx = row * Csize + col
            a = ptr_a.flat[idx]
            if Amode == 'mrv':
                scale = ptr_b.flat[row]
            elif Amode == 'mcv':
                scale = ptr_b.flat[col]
            else:
                assert False, "Invalid Amode"
            c = int(float(a) * scale)
            ptr_c.flat[idx] = min(max(c, -128), 127)
    return ptr_c

for i in range(CASE_NUMBER):
    # generate random matrix

    input_int = np.random.randint(low=-300, high=300, size=(R_SIZE, C_SIZE), dtype=np.int32)
    input_float = None
    amod = 1 # np.random.randint(1, 3)
    if amod == 1:
        input_float = np.random.rand(R_SIZE).astype(np.float32)
    else:
        input_float = np.random.rand(C_SIZE).astype(np.float32)
    output_matrix = np.zeros((R_SIZE, C_SIZE), dtype=np.int8)
    output_matrix = mquant(output_matrix, input_int, input_float, 'mrv' if amod == 1 else 'mcv').astype(np.int8)
    # store matrices into the file
    with open(input_file_path, 'wb') as f:
        input_int.tofile(f)
        input_float.tofile(f)
        output_matrix.tofile(f)

    # run simulation
    if 'sifive-x280' in cpu_list:
        result = subprocess.run(["qemu-riscv64", "-cpu", "sifive-x280", executable_path, input_file_path, str(amod)], capture_output=True, text=True)
    else:
        result = subprocess.run(["qemu-riscv64", "-cpu", "max,vlen=512", executable_path, input_file_path, str(amod)], capture_output=True, text=True)
    
    # exam the output
    if result.returncode == 0 and 'passed' in result.stdout:
        correct_case_num += 1
    else:
        print("====================================")
        print("\033[31m Test failed for case {}, details will be printed below: ".format(i + 1))
        print("\033[34m input int:")
        print(input_int)
        print("\033[34m input float:")
        print(input_float)
        print("\033[34m standard result matrix:")
        print(output_matrix)
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
