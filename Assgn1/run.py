#!/usr/bin/env python3

'''
File    : run.py
Author  : Gautam Singh (CS21BTECH11018)
Date    : 2024-08-09
Purpose : Run tests and generate plots for report
'''

# Imports
import subprocess
import matplotlib.pyplot as plt

# Constants
SRC_DIR = "src"
INFILE = f"{SRC_DIR}/inp.txt"
OUTFILE = f"{SRC_DIR}/out.txt"
IMG_PATH = "report/images"
CC = "g++"
EXE = ".\\a.exe" # "./a.out" for Linux
METHODS = ["chunk", "mixed", "dynamic", "block"]
RUNS = 5
SIZES = [1000, 2000, 3000, 4000, 5000]
THREADS = [1, 2, 4, 8, 16, 32]
SPARSITIES = [20, 40, 60, 80]
ROW_INCREMENTS = [10, 20, 30, 40, 50]

def create_input_file(N: int, S: int, K: int, row_inc: int, mat_file: str) -> int:
    '''
    Function to create an input file with the given parameters.
    
    #### Parameters
    - N : Number of rows of the matrix.
    - S : Sparsity (in percent) of the matrix.
    - K : Number of threads to run.
    - row_inc : Row increment (for dynamic techniques).
    - mat_file : File containing the matrix.
    - out_file : File to output the full input file.
    
    #### Returns
    0 on success.
    '''
    L = []
    with open(mat_file, 'r') as infile:
        L = infile.readlines()
    with open(INFILE, 'w') as outfile:
        outfile.write(f'{N} {S} {K} {row_inc}\n')
        outfile.writelines(L)
    return 0

def compile_source(src: str = "main.cpp", flags: list[str] = ["-O3", "-std=c++20"]):
    subprocess.run([CC, src] + flags, cwd=SRC_DIR, shell=True)

def run_source(exe: str, flags: list[str]):
    subprocess.run([exe] + flags, cwd=SRC_DIR, shell=True)

def get_total_runtime() -> int:
    with open(OUTFILE, "r") as fh:
        L = fh.readlines()
        return int(L[0].split()[-2])

def run_exp_1(S: int, K: int, row_inc: int):
    # Compile the source file
    compile_source()
    ax = plt.gca()
    L = [[0.0 for _ in range(len(SIZES))] for _ in range(len(METHODS))]
    for j, N in enumerate(SIZES):
        # Create suitable input file
        create_input_file(N, S, K, row_inc, f"inputs/exp1/{N}-{S}.txt")
        for i, method in enumerate(METHODS):
            for _ in range(RUNS):
                # Run source file
                run_source(EXE, ["-t", method])
                L[i][j] += get_total_runtime()
            L[i][j] /= RUNS
    for i, method in enumerate(METHODS):
        ax.plot(SIZES, L[i], label=method)
    ax.legend()
    ax.grid()
    ax.set_ylabel('Time (ms)')
    ax.set_xlabel('Size')
    ax.set_title(f'Time vs. Size N (S = {S}%, K = {K}, rowInc = {row_inc})')
    plt.tight_layout()
    plt.savefig(f'{IMG_PATH}/exp1.png')

def run_exp_2(N: int, S: int, row_inc: int):
    # Compile the source file
    compile_source()
    ax = plt.gca()
    L = [[0.0 for _ in range(len(THREADS))] for _ in range(len(METHODS))]
    for j, K in enumerate(THREADS):
        # Create suitable input file
        create_input_file(N, S, K, row_inc, f"inputs/exp2/{N}-{S}.txt")
        for i, method in enumerate(METHODS):
            for _ in range(RUNS):
                # Run source file
                run_source(EXE, ["-t", method])
                L[i][j] += get_total_runtime()
            L[i][j] /= RUNS
    for i, method in enumerate(METHODS):
        ax.plot(SIZES, L[i], label=method)
    ax.legend()
    ax.grid()
    ax.set_ylabel('Time (ms)')
    ax.set_xlabel('Number of threads')
    ax.set_title(f'Time vs. Number of Threads K (N = {N}, S = {S}%, rowInc = {row_inc})')
    plt.tight_layout()
    plt.savefig(f'{IMG_PATH}/exp2.png')

def run_exp_3(N: int, K: int, row_inc: int):
    # Compile the source file
    compile_source()
    ax = plt.gca()
    L = [[0.0 for _ in range(len(SPARSITIES))] for _ in range(len(METHODS))]
    for j, S in enumerate(SPARSITIES):
        # Create suitable input file
        create_input_file(N, S, K, row_inc, f"inputs/exp3/{N}-{S}.txt")
        for i, method in enumerate(METHODS):
            for _ in range(RUNS):
                # Run source file
                run_source(EXE, ["-t", method])
                L[i][j] += get_total_runtime()
            L[i][j] /= RUNS
    for i, method in enumerate(METHODS):
        ax.plot(SIZES, L[i], label=method)
    ax.legend()
    ax.grid()
    ax.set_ylabel('Time (ms)')
    ax.set_xlabel('Number of threads')
    ax.set_title(f'Time vs. Sparsity S (N = {N}, K = {K}, rowInc = {row_inc})')
    plt.tight_layout()
    plt.savefig(f'{IMG_PATH}/exp3.png')

def run_exp_4(N: int, S: int, K: int):
    # Compile the source file
    compile_source()
    ax = plt.gca()
    L = [[0.0 for _ in range(len(ROW_INCREMENTS))] for _ in range(len(METHODS[-2:]))]
    for j, row_inc in enumerate(ROW_INCREMENTS):
        # Create suitable input file
        create_input_file(N, S, K, row_inc, f"inputs/exp4/{N}-{S}.txt")
        for i, method in enumerate(METHODS[-2:]):
            for _ in range(RUNS):
                # Run source file
                run_source(EXE, ["-t", method])
                L[i][j] += get_total_runtime()
            L[i][j] /= RUNS
    for i, method in enumerate(METHODS[-2:]):
        ax.plot(SIZES, L[i], label=method)
    ax.legend()
    ax.grid()
    ax.set_ylabel('Time (ms)')
    ax.set_xlabel('Number of threads')
    ax.set_title(f'Time vs. Row Increment rowInc (N = {N}, K = {K}, S = {S}%)')
    plt.tight_layout()
    plt.savefig(f'{IMG_PATH}/exp4.png')

run_exp_1(40, 16, 50)
run_exp_2(5000, 40, 50)
run_exp_3(5000, 16, 50)
run_exp_4(5000, 40, 16)
