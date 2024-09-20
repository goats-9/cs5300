#!/usr/bin/env python3

'''
File    : run.py
Author  : Gautam Singh
Date    : 2024-09-18
Purpose : Run tests and generate plots for report
'''

# Imports
import subprocess
import sys
import matplotlib.pyplot as plt

# Constants
IMG_PATH = "../report/images"
CC = "g++"
SRC_LIST = ["bakery.cpp", "filter.cpp"]
EXE = ".\\a.exe" # "./a.out" for Linux
INPUT_FILE = "inp-params.txt"
OUTPUT_FILE = "out.txt"
NUM_THREADS = [2, 4, 8, 16, 32, 64]
NUM_REQS = [5, 10, 15, 20, 25]
NUM_RUNS = 20
LAMBDA_1 = 1
LAMBDA_2 = 2
UNITS_PER_MS = 1e6

def create_input_file(n: int, k: int, lambda_1: float, lambda_2: float):
    with open(INPUT_FILE, "w") as fh:
        fh.write(f'{n} {k} {lambda_1} {lambda_2}')

def compile_source(src: str, flags: list[str] = ["-O3", "-std=c++20"]):
    subprocess.run([CC, src] + flags, shell=True)

def run_program():
    subprocess.run([EXE], shell=True)
    
def get_total_runtime() -> float:
    with open(OUTPUT_FILE, "r") as fh:
        L = fh.readlines()
        # Convert to ms
        return int(L[-1].split()[-2]) / UNITS_PER_MS

def get_avg_entry_time() -> float:
    cnt = 0
    cs_entry_time = 0
    with open(OUTPUT_FILE, "r") as fh:
        for line in fh.readlines():
            words = line.split()
            if words[:3] == ["CS", "Entry", "Request"]:
                cs_entry_time -= int(words[5])
            elif words[:2] == ["CS", "Entry"]:
                cs_entry_time += int(words[4])
                cnt += 1
    # Convert to ms
    return cs_entry_time / (cnt * UNITS_PER_MS)

def get_worst_entry_time() -> float:
    worst_cs_entry_time = 0
    cs_entry_time = 0
    with open(OUTPUT_FILE, "r") as fh:
        for line in fh.readlines():
            words = line.split()
            if words[:3] == ["CS", "Entry", "Request"]:
                cs_entry_time -= int(words[5])
            elif words[:2] == ["CS", "Entry"]:
                cs_entry_time += int(words[4])
                worst_cs_entry_time = max(worst_cs_entry_time, cs_entry_time)
    # Convert to ms
    return worst_cs_entry_time / UNITS_PER_MS

def run_exp_1(src_list: list[str] = SRC_LIST, k: int = 15, lambda_1: float = LAMBDA_1, lambda_2: float = LAMBDA_2):
    plt.clf()
    for src in src_list:
        compile_source(src)
        L = [0.0] * len(NUM_THREADS)
        for i, n in enumerate(NUM_THREADS):
            create_input_file(n, k, lambda_1, lambda_2)
            for _ in range(NUM_RUNS):
                run_program()
                L[i] += (n * k) / get_total_runtime()
            L[i] /= NUM_RUNS
        plt.plot(NUM_THREADS, L, label=f'{src.split('.')[0]}')
    plt.title(f'Throughput vs. Threads ($k={k}$, $\\lambda_1={lambda_1}$, $\\lambda_2={lambda_2}$)')
    plt.xlabel(f'Number of Threads $n$')
    plt.ylabel(f'Throughput (tasks / ms)')
    plt.legend()
    plt.grid()
    plt.tight_layout()
    plt.savefig(f'{IMG_PATH}/exp1.png')  

def run_exp_2(src_list: list[str] = SRC_LIST, n: int = 16, lambda_1: float = LAMBDA_1, lambda_2: float = LAMBDA_2):
    plt.clf()
    for src in src_list:
        compile_source(src)
        L = [0.0] * len(NUM_REQS)
        for i, k in enumerate(NUM_REQS):
            create_input_file(n, k, lambda_1, lambda_2)
            for _ in range(NUM_RUNS):
                run_program()
                L[i] += (n * k) / get_total_runtime()
            L[i] /= NUM_RUNS
        plt.plot(NUM_REQS, L, label=f'{src.split('.')[0]}')
    plt.title(f'Throughput vs. CS Requests ($n={n}$, $\\lambda_1={lambda_1}$, $\\lambda_2={lambda_2}$)')
    plt.xlabel(f'Number of Critical Section Requests $k$')
    plt.ylabel(f'Throughput (tasks / ms)')
    plt.legend()
    plt.grid()
    plt.tight_layout()
    plt.savefig(f'{IMG_PATH}/exp2.png')  

def run_exp_3(src_list: list[str] = SRC_LIST, k: int = 10, lambda_1: float = LAMBDA_1, lambda_2: float = LAMBDA_2):
    plt.clf()
    for src in src_list:
        compile_source(src)
        L = [0.0] * len(NUM_THREADS)
        for i, n in enumerate(NUM_THREADS):
            create_input_file(n, k, lambda_1, lambda_2)
            for _ in range(NUM_RUNS):
                run_program()
                L[i] += get_avg_entry_time()
            L[i] /= NUM_RUNS
        plt.plot(NUM_THREADS, L, label=f'{src.split('.')[0]}-avg')
        L = [0.0] * len(NUM_THREADS)
        for i, n in enumerate(NUM_THREADS):
            create_input_file(n, k, lambda_1, lambda_2)
            for _ in range(NUM_RUNS):
                run_program()
                L[i] += get_worst_entry_time()
            L[i] /= NUM_RUNS
        plt.plot(NUM_THREADS, L, label=f'{src.split('.')[0]}-worst')
    plt.title(f'Entry Time vs. Threads ($k={k}$, $\\lambda_1={lambda_1}$, $\\lambda_2={lambda_2}$)')
    plt.ylabel(f'Entry time (ms)')
    plt.xlabel(f'Number of Threads $n$')
    plt.legend()
    plt.grid()
    plt.tight_layout()
    plt.savefig(f'{IMG_PATH}/exp3.png')  

def run_exp_4(src_list: list[str] = SRC_LIST, n: int = 16, lambda_1: float = LAMBDA_1, lambda_2: float = LAMBDA_2):
    plt.clf()
    for src in src_list:
        compile_source(src)
        L = [0.0] * len(NUM_REQS)
        for i, k in enumerate(NUM_REQS):
            create_input_file(n, k, lambda_1, lambda_2)
            for _ in range(NUM_RUNS):
                run_program()
                L[i] += get_avg_entry_time()
            L[i] /= NUM_RUNS
        plt.plot(NUM_REQS, L, label=f'{src.split('.')[0]}-avg')
        L = [0.0] * len(NUM_REQS)
        for i, k in enumerate(NUM_REQS):
            create_input_file(n, k, lambda_1, lambda_2)
            for _ in range(NUM_RUNS):
                run_program()
                L[i] += get_worst_entry_time()
            L[i] /= NUM_RUNS
        plt.plot(NUM_REQS, L, label=f'{src.split('.')[0]}-worst')
    plt.title(f'Entry Time vs. CS Requests ($n={n}$, $\\lambda_1={lambda_1}$, $\\lambda_2={lambda_2}$)')
    plt.ylabel(f'Entry time (ms)')
    plt.xlabel(f'Number of Critical Section Requests $k$')
    plt.legend()
    plt.grid()
    plt.tight_layout()
    plt.savefig(f'{IMG_PATH}/exp4.png')  

if sys.argv[1] == "1":
    run_exp_1()
elif sys.argv[1] == "2":
    run_exp_2()
elif sys.argv[1] == "3":
    run_exp_3()
elif sys.argv[1] == "4":
    run_exp_4()
else:
    run_exp_1()
    run_exp_2()
    run_exp_3()
    run_exp_4()