import subprocess
import os
import numpy as np

CASE_NUMBER = 200
os.system("/opt/LLVM/install/bin/clang -static sg_mcvtnf.c sg_mcvtnf_asm.S -o sg_mcvtnf -O3 -march=rv64imafdcv_zicsr_zifencei_zfh_zba_zbb_zvfh_xsfvfnrclipxfqf_xsfvfwmaccqqq_xsfvqmaccqoq_xsfvcp_xsgmat --gcc-toolchain=/opt/riscv --sysroot=/opt/riscv/sysroot/ -g")

# data configuration
executable_path = './sg_mcvtnf'  # excutable file
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
    output_matrix = np.random.rand(8, 8).astype(np.float16)
    input_matrix = output_matrix.astype(np.float32)
    # store array into the file
    with open(input_file_path, 'wb') as f:
        input_matrix.tofile(f)
        output_matrix.tofile(f)
    
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
        print("\033[34m input matrix:")
        print(input_matrix)
        print("\033[35m output matrix:")
        print(output_matrix)
        print("\033[36m qemu result matrix:")
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
