import numpy as np
import subprocess
import os
R_SIZE = 8
C_SIZE = 6
CASE_NUMBER = 200
os.system("/opt/LLVM/install/bin/clang -static sg_mdequant.c sg_mdequant_asm.S -o sg_mdequant -O3 -march=rv64imafdcv_zicsr_zifencei_zfh_zba_zbb_zvfh_xsfvfnrclipxfqf_xsfvfwmaccqqq_xsfvqmaccqoq_xsfvcp_xsgmat --gcc-toolchain=/opt/riscv --sysroot=/opt/riscv/sysroot/ -g")

# data configuration
executable_path = './sg_mdequant'  # excutable file
input_file_path = 'input_matrices.bin'  # input matrix
output_file_path = 'output_result.txt'
correct_case_num = 0


def split_int8(num):
    num = np.int8(num)
    high = np.int8(num >> 4)
    low = np.int8(num & 0x0F)
    return high, low

def combine_int8s(arr1, arr2):
    # Ensure the input arrays are int8
    arr1 = arr1.astype(np.int8)
    arr2 = arr2.astype(np.int8)
    arr = np.zeros((arr1.shape[0], arr1.shape[1]), dtype=np.int8)
    for i in range(arr1.shape[0]):
        for j in range(arr1.shape[1]):
            a = (arr1[i, j] << 4) & 0xF0
            b = arr2[i, j] & 0xF
            arr[i, j] = a | b
    return arr

def mdequant(md, ms1, ms2, Amode):
    ptr_a = ms1.astype(np.int8)
    ptr_b = ms2.astype(np.float32)
    ptr_c = md.astype(np.float16)
    Rsize, Csize = (R_SIZE, C_SIZE)

    for row in range(Rsize):
        for col in range(Csize):
            idx = (row * Csize + col) * 2
            idx_ = row * Csize + col
            a = ptr_a.flat[idx_]
            a0, a1 = split_int8(a)
            if Amode == 'mrv':
                scale, zp = ptr_b[row, 0], ptr_b[row, 1]
            else:
                assert False, "Invalid Amode"
            ptr_c.flat[idx] = float(a0) * scale + zp
            ptr_c.flat[idx + 1] = float(a1) * scale + zp
    return ptr_c

def get_cpu_list():
    result = subprocess.run(["qemu-riscv64", "-cpu", "help"], capture_output=True, text=True)
    output_lines = result.stdout.split('\n')
    cpu_list = [line.strip() for line in output_lines if line.strip()]
    return cpu_list[1:]  # Skip the first line "Available CPUs:"
cpu_list = get_cpu_list()

for i in range(CASE_NUMBER):
    # generate random matrix
    input_int1 = np.random.randint(low=0, high=8, size=(R_SIZE, C_SIZE), dtype=np.int8)
    input_int2 = np.random.randint(low=0, high=8, size=(R_SIZE, C_SIZE), dtype=np.int8)
    input_int = combine_int8s(input_int1, input_int2)
    input_float = None
    amod = 1 # np.random.randint(1, 3)
    input_float = np.random.rand(R_SIZE, 2).astype(np.float16)
    output_matrix = np.zeros((R_SIZE, 2 * C_SIZE), dtype=np.float16)
    output_matrix = mdequant(output_matrix, input_int, input_float, 'mrv' if amod == 1 else 'mcv').astype(np.float16)
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
