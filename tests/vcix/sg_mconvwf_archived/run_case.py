import numpy as np
import subprocess
import os
import random

def combine_to_64bit(kh, kw, strh, strw, pt, pb, pl, pr, ih, iw, oh, ow):
    # ensure every number not exceed the corresponding digit amount.
    mask4 = 0b1111  # 4 bit mask
    mask8 = 0b11111111  # 8 bit mask
    # concat numbers
    combined = (kh & mask4) << 60 | (kw & mask4) << 56 | \
               (strh & mask4) << 52 | (strw & mask4) << 48 | \
               (pt & mask4) << 44 | (pb & mask4) << 40 | \
               (pl & mask4) << 36 | (pr & mask4) << 32 | \
               (ih & mask8) << 24 | (iw & mask8) << 16 | \
               (oh & mask8) << 8 | (ow & mask8)
    return combined

CASE_NUMBER = 1500
os.system("/opt/LLVM/install/bin/clang -static sg_mconvwf.c sg_mconvwf_asm.S -o sg_mconvwf -O3 -march=rv64imafdcv_zicsr_zifencei_zfh_zba_zbb_zvfh_xsfvfnrclipxfqf_xsfvfwmaccqqq_xsfvqmaccqoq_xsfvcp_xsgmat --gcc-toolchain=/opt/riscv --sysroot=/opt/riscv/sysroot/ -g")

# data configuration
executable_path = './sg_mconvwf'  # excutable file
input_file_path = 'input_matrices.bin'  # input matrix
output_file_path = 'output_result.txt'
correct_case_num = 0
for i in range(CASE_NUMBER):
    # generate random matrix
    k_h = 3
    k_w = 3

    p_b = np.random.choice(np.array([1, 2, 3, 4, 5]))
    p_l = np.random.choice(np.array([1, 2, 3, 4, 5]))
    p_t = np.random.choice(np.array([1, 2, 3, 4, 5]))
    p_r = np.random.choice(np.array([1, 2, 3, 4, 5]))

    strw = np.random.choice(np.array([1, 2, 3]))
    strh = np.random.choice(np.array([1, 2, 3]))

    i_h = random.randint(14, 17)
    i_w = random.randint(14, 17)

    o_h = ((i_h - k_h + p_b + p_t) // strh) + 1
    o_w = ((i_w - k_w + p_l + p_r) // strw) + 1

    kernel = np.random.rand(k_h, k_w).astype(np.float16)
    input_matrix = np.random.rand(i_h, i_w).astype(np.float16)
    conv_parameter = combine_to_64bit(k_h, k_w, strh, strw, p_t, p_b, p_l, p_r, i_h, i_w, o_h, o_w)

    padded_input = np.pad(input_matrix, ((p_b, p_t), (p_l, p_r)), mode='constant', constant_values=0)
    padded_input = padded_input.astype(np.float32)
    output_matrix = np.zeros((o_h, o_w), dtype=np.float32)
    for y in range(o_h):
        for x in range(o_w):
            sub_matrix = padded_input[y*strh:y*strh+k_h, x*strw:x*strw+k_w]
            conv_kernel = kernel.astype(np.float32)
            conv_result = (sub_matrix * conv_kernel).sum()
            output_matrix[y, x] = conv_result

    # store matrices into the file
    with open(input_file_path, 'wb') as f:
        input_matrix.tofile(f)
        kernel.tofile(f)
        output_matrix.tofile(f)
    # run simulation
    result = subprocess.run(["qemu-riscv64", "-cpu", "sifive-x280", executable_path, input_file_path, output_file_path, str(conv_parameter)], capture_output=True, text=True)
    # exam the output
    if result.returncode == 0 and 'passed' in result.stdout:
        # print("Test passed.")
        correct_case_num += 1
    else:
        print("====================================")
        print("\033[31m Test failed for case {}, details will be printed below: ".format(i + 1))
        print("\033[34m input matrix:")
        print(input_matrix)
        print("\033[34m kernel:")
        print(kernel)
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
