import random
import subprocess
import os
import numpy as np

VL = 20
CASE_NUMBER = 200
os.system("/opt/LLVM/install/bin/clang -static sg_mvvw.c sg_mvvw_asm.S -o sg_mvvw -O3 -march=rv64imafdcv_zicsr_zifencei_zfh_zba_zbb_zvfh_xsfvfnrclipxfqf_xsfvfwmaccqqq_xsfvqmaccqoq_xsfvcp_xsgmat --gcc-toolchain=/opt/riscv --sysroot=/opt/riscv/sysroot/ -g")

# data configuration
executable_path = './sg_mvvw'  # excutable file
input_file_path = 'input_matrices.bin'  # input array
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
    input_array1 = np.random.randint(-100, 100, size=(1, VL), dtype=np.int8)
    input_array2 = np.random.randint(-100, 100, size=(1, VL), dtype=np.int8)
    input_array3 = np.random.randint(-100, 100, size=(1, VL), dtype=np.int16)
    # store array into the file
    with open(input_file_path, 'wb') as f:
        input_array1.tofile(f)
        input_array2.tofile(f)
        input_array3.tofile(f)
    
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
        print("\033[34m input array:")
        print(input_array1)
        print(input_array2)
        print(input_array3)
        print("\033[36m qemu result array:")
        print(result.stdout)
        print("====================================")

# clear the file
os.remove(executable_path)
os.remove(input_file_path)
per = correct_case_num/CASE_NUMBER*100
if per > 99:
    print(f"Correct percentage: \033[32m{per}%")
elif per > 80:
    print(f"Correct percentage: \033[33m{per}%")
else:
    print(f"Correct percentage: \033[31m{per}%")
