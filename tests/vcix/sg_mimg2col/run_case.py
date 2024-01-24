import numpy as np
import subprocess
import os
import random

CASE_NUMBER = 200
os.system("/opt/LLVM/install/bin/clang -static sg_mimg2col.c sg_mimg2col_asm.S -o sg_mimg2col -O3 -march=rv64imafdcv_zicsr_zifencei_zfh_zba_zbb_zvfh_xsfvfnrclipxfqf_xsfvfwmaccqqq_xsfvqmaccqoq_xsfvcp_xsgmat --gcc-toolchain=/opt/riscv --sysroot=/opt/riscv/sysroot/ -g")

# data configuration
executable_path = './sg_mimg2col'  # excutable file
input_file_path = 'input_matrices.bin'  # input matrix
output_file_path = 'output_result.txt'
correct_case_num = 0

def img2col(dst, output_h, output_w,
            src, input_h, input_w,
            kernel_h, kernel_w, offset_h, offset_w,
            stride_h, stride_w, pad_up, pad_left):
    for row in range(output_h):
        for col in range(output_w):
            for m in range(kernel_h):
                if m < offset_h:
                    continue
                for n in range(kernel_w):
                    if n < offset_w:
                        continue
                    img_row = row * stride_h - pad_up + m
                    img_col = col * stride_w - pad_left + n
                    input_index = img_row * input_w + img_col
                    output_index = ((row * output_w + col) * kernel_h + m) * kernel_w + n
                    if 0 <= img_row and img_row < input_h and 0 <= img_col and img_col < input_w:
                        dst[0, output_index] = src[0,input_index]
                    else:
                        dst[0,output_index] = 0
    return dst

def pack_into_rs_conv(pad_right, pad_left, pad_down, pad_up, stride_w, stride_h, output_w, output_h, input_w, input_h):
    rs = 0
    rs |= (output_w & 0xFF)
    rs |= (output_h & 0xFF) << 8
    rs |= (input_w & 0xFF) << 16
    rs |= (input_h & 0xFF) << 24
    rs |= (pad_right & 0xF) << 32
    rs |= (pad_left & 0xF) << 36
    rs |= (pad_down & 0xF) << 40
    rs |= (pad_up & 0xF) << 44
    rs |= (stride_w & 0xF) << 48
    rs |= (stride_h & 0xF) << 52
    return rs

def pack_into_rs_img2col(offset_w, offset_h, kernel_w, kernel_h):
    rs = 0
    rs |= (offset_w & 0xFF)
    rs |= (offset_h & 0xFF) << 8
    rs |= (kernel_w & 0xF) << 16
    rs |= (kernel_h & 0xF) << 20
    return rs

def get_cpu_list():
    result = subprocess.run(["qemu-riscv64", "-cpu", "help"], capture_output=True, text=True)
    output_lines = result.stdout.split('\n')
    cpu_list = [line.strip() for line in output_lines if line.strip()]
    return cpu_list[1:]  # Skip the first line "Available CPUs:"
cpu_list = get_cpu_list()

for i in range(CASE_NUMBER):
    # generate key parameters
    k_h = 3
    k_w = 1

    p_b = np.random.choice(np.array([1, 2]))
    p_l = np.random.choice(np.array([1, 2]))
    p_t = np.random.choice(np.array([1, 2]))
    p_r = np.random.choice(np.array([1, 2]))

    strw = np.random.choice(np.array([1, 2]))
    strh = np.random.choice(np.array([1, 2]))

    i_h = random.randint(6, 10)
    i_w = random.randint(6, 10)

    o_h = ((i_h - k_h + p_b + p_t) // strh) + 1
    o_w = ((i_w - k_w + p_l + p_r) // strw) + 1


    # generate random matrix
    input_matrix = np.random.randint(-128, 128, size=(1, i_w * i_h), dtype=np.int8)
    output_matrix = np.zeros((1, o_h * o_w * k_h * k_w), dtype=np.int8)
    output_matrix = img2col(output_matrix, o_h, o_w, input_matrix, i_h, i_w, k_h, k_w, 0, 0, strh, strw, p_t, p_l)

    conv = pack_into_rs_conv(p_r, p_l, p_b, p_t, strw, strh, o_w, o_h, i_w, i_h)
    conv_shape = pack_into_rs_img2col(0, 0, k_w, k_h)
    # store matrices into the file
    with open(input_file_path, 'wb') as f:
        input_matrix = input_matrix.astype(np.int8)
        output_matrix = output_matrix.astype(np.int8)
        input_matrix.tofile(f)
        output_matrix.tofile(f)

    # run simulation
    if 'sifive-x280' in cpu_list:
        result = subprocess.run(["qemu-riscv64", "-cpu", "sifive-x280", executable_path, input_file_path, str(conv), str(conv_shape)], capture_output=True, text=True)
    else:
        result = subprocess.run(["qemu-riscv64", "-cpu", "max,vlen=512", executable_path, input_file_path, str(conv), str(conv_shape)], capture_output=True, text=True)

    # exam the output
    if result.returncode == 0 and 'passed' in result.stdout:
        correct_case_num += 1
    else:
        print("====================================")
        print("\033[31m Test failed for case {}, details will be printed below: ".format(i + 1))
        print("\033[34m input matrix:")
        print(input_matrix)
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
