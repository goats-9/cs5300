#!/usr/bin/env python3

'''
File    : run.py
Author  : Gautam Singh
Date    : 2024-10-10
Purpose : Run tests and generate plots for report
'''

# Imports
import subprocess
import sys
import matplotlib.pyplot as plt
import re

# Constants
IMG_PATH = "../report/images"
CC = "g++"
SRC_LIST = ["main.cpp", ]
# EXE = "./a.out" # ".\\a.exe" for Windows
EXE = ".\\a.exe"
INPUT_FILE = "inp-params.txt"
OUTPUT_FILE = "output.txt"
NUM_ACCOUNTS = [10, 20, 30, 40, 50]
NUM_THREADS = [5, 10, 15, 20, 25]
NUM_T = [10, 20, 30, 40, 50]
NUM_RUNS = 50
UNITS_PER_MS = 1e6

def create_input_file(n: int, p: int, t: int, alpha: float):
    with open(INPUT_FILE, "w") as fh:
        fh.write(f'{n} {p} {t} {alpha}')

def compile_source(src: str, flags: list[str] = ["-O3", "-std=c++20", "-Wall", "-pthread"]):
    subprocess.run([CC, src] + flags, stdout=subprocess.PIPE)

def run_program():
    subprocess.run([EXE,], stdout=subprocess.PIPE)

def get_avg_latency():
    r_start = re.compile(r"(.*)?account (\d)+\.$")
    r_end = re.compile(r"(.*)?completes")
    with open(OUTPUT_FILE, "r") as fh:
        L = fh.readlines()
        ans = 0
        start_list = list(filter(r_start.match, L))
        end_list = list(filter(r_end.match, L))
        for log in start_list:
            ans -= int(log[1:].strip().split()[0])
        for log in end_list:
            ans += int(log[1:].strip().split()[0])
        return ans / (len(start_list) * UNITS_PER_MS)

def get_throughput():
    return 1 / get_avg_latency()

def run_exp_1(
    src_list: list[str] = SRC_LIST,
    t: int = 10,
    alpha: float = 1.5,
    p_list: list[int] = [10, 50],
):
    print(f'Running Experiment 1')
    plt.clf()
    for src in src_list:
        compile_source(src)
        for _, p in enumerate(p_list):
            L = []
            for _, n in enumerate(NUM_THREADS):
                create_input_file(n, p, t, alpha)
                sm = 0
                for _ in range(NUM_RUNS):
                    run_program()
                    sm += get_throughput()
                L.append(sm / NUM_RUNS)
            plt.plot(NUM_THREADS, L, label=f'$p = {p}$')
    plt.title(f'Throughput of Savings Account ($t = {t}, \\alpha = {alpha}$)')
    plt.xlabel(f'Number of threads')
    plt.ylabel(f'Throughput (ops / ms)')
    plt.legend()
    plt.grid()
    plt.tight_layout()
    plt.savefig(f'{IMG_PATH}/exp1.png')  

def run_exp_2(
    src_list: list[str] = SRC_LIST,
    n: int = 15,
    alpha: float = 1.5,
    p_list: list[int] = [10, 50],
):
    print(f'Running Experiment 2')
    plt.clf()
    for src in src_list:
        compile_source(src)
        for _, p in enumerate(p_list):
            L = []
            for _, t in enumerate(NUM_T):
                create_input_file(n, p, t, alpha)
                sm = 0
                for _ in range(NUM_RUNS):
                    run_program()
                    sm += get_avg_latency()
                L.append(sm / NUM_RUNS)
            plt.plot(NUM_T, L, label=f'$p = {p}$')
    plt.title(f'Latency of Savings Account ($n = {n}, \\alpha = {alpha}$)')
    plt.xlabel(f'Number of operations per thread')
    plt.ylabel(f'Latency (ms)')
    plt.legend()
    plt.grid()
    plt.tight_layout()
    plt.savefig(f'{IMG_PATH}/exp2.png')  

def run_exp_3(
    src_list: list[str] = SRC_LIST,
    n: int = 15,
    t: int = 20,
    alpha: float = 1.5,
):
    print(f'Running Experiment 3')
    plt.clf()
    for src in src_list:
        compile_source(src)
        L = []
        for _, p in enumerate(NUM_ACCOUNTS):
            create_input_file(n, p, t, alpha)
            sm = 0
            for _ in range(NUM_RUNS):
                run_program()
                sm += get_avg_latency()
            L.append(sm / NUM_RUNS)
        plt.plot(NUM_ACCOUNTS, L)
    plt.title(f'Latency of Savings Account ($n = {n}, t = {t}, \\alpha = {alpha}$)')
    plt.xlabel(f'Number of accounts')
    plt.ylabel(f'Latency (ms)')
    plt.grid()
    plt.tight_layout()
    plt.savefig(f'{IMG_PATH}/exp3.png')  


if sys.argv[1] == "1":
    run_exp_1()
elif sys.argv[1] == "2":
    run_exp_2()
elif sys.argv[1] == "3":
    run_exp_3()
else:
    run_exp_1()
    run_exp_2()
    run_exp_3()
