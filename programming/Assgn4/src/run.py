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

# Constants
IMG_PATH = "../report/images"
CC = "g++"
SRC_LIST = ["ofs.cpp", "wfs.cpp"]
EXE = ".\\a.exe" # "./a.out" for Linux
INPUT_FILE = "inp-params.txt"
OUTPUT_FILE = "out.txt"
UPDATE_THREAD = "Writer"
SCAN_THREAD = "Snapshot"
NUM_THREADS = [4, 8, 16, 32]
FLAG_ARR = [(True, False, "update"), (False, True, "scan"), (True, True, "all")]
M = 40
LAMBDA = 0.5
K = 5
FIXED_RATIO = 3
FIXED_NS = 4
RATIO = [1, 2, 4, 6, 8, 10]
NUM_RUNS = 5
UNITS_PER_MS = 1e6

def create_input_file(nw: int, ns: int, M: int, lambda_w: float, lambda_s: float, k: int):
    with open(INPUT_FILE, "w") as fh:
        fh.write(f'{nw} {ns} {M} {lambda_w} {lambda_s} {k}')

def compile_source(src: str, flags: list[str] = ["-O3", "-std=c++20"]):
    output = subprocess.check_output([CC, src] + flags, shell=True)
    assert not output, f'Error: Exit code {output}'

def run_program():
    subprocess.run([EXE], shell=True)
    
def get_avg_time(upd: bool, scan: bool) -> float:
    tot, cnt = 0, 0
    with open(OUTPUT_FILE, "r") as fh:
        for line in fh.readlines():
            L = line.split()
            if L[1] == UPDATE_THREAD and upd:
                tot += int(L[-2])
                cnt += 1
            if L[1] == SCAN_THREAD and scan:
                tot += int(L[-2])
                cnt += 1
    return tot / (cnt * UNITS_PER_MS)

def get_worst_time(upd: bool, scan: bool) -> float:
    tot = 0
    with open(OUTPUT_FILE, "r") as fh:
        for line in fh.readlines():
            L = line.split()
            if L[1] == UPDATE_THREAD and upd:
                tot = max(tot, int(L[-2]))
            if L[1] == SCAN_THREAD and scan:
                tot = max(tot, int(L[-2]))
    return tot / UNITS_PER_MS

def run_exp_1(
    src_list: list[str] = SRC_LIST,
    ratio: int = FIXED_RATIO,
    M: int = M,
    lambda_w: float = LAMBDA,
    lambda_s: float = LAMBDA,
    k: int = K,
):
    plt.clf()
    for src in src_list:
        compile_source(src)
        alg = src.split('.')[0]
        L = [[0.0] * len(NUM_THREADS) for _ in range(len(FLAG_ARR))]
        for j, n in enumerate(NUM_THREADS):
            ns = n // (1 + ratio)
            nw = ratio * ns
            create_input_file(nw, ns, M, lambda_w, lambda_s, k)
            for _ in range(NUM_RUNS):
                run_program()
                for i, v in enumerate(FLAG_ARR):
                    upd, scan, _ = v
                    L[i][j] += get_avg_time(upd, scan)
        L = [[x / NUM_RUNS for x in l] for l in L]
        for i, v in enumerate(FLAG_ARR):
            _, _, lbl = v
            plt.plot(NUM_THREADS, L[i], label=f'{alg}-{lbl}')
    plt.title(f'Average-case Scalability ($n_w/n_s = {ratio}$, $M = {M}$, $\\mu_w = {lambda_w}$ ms, $\\mu_s = {lambda_s}$ ms, $k = {K}$)', loc='center', wrap=True)
    plt.xlabel(f'Total number of threads')
    plt.ylabel(f'Average-case time (ms)')
    plt.legend(bbox_to_anchor=(1.05, 1))
    plt.grid()
    plt.tight_layout()
    plt.savefig(f'{IMG_PATH}/exp1.png')  

def run_exp_2(
    src_list: list[str] = SRC_LIST,
    ratio: int = FIXED_RATIO,
    M: int = M,
    lambda_w: float = LAMBDA,
    lambda_s: float = LAMBDA,
    k: int = K,
):
    plt.clf()
    for src in src_list:
        compile_source(src)
        alg = src.split('.')[0]
        L = [[0.0] * len(NUM_THREADS) for _ in range(len(FLAG_ARR))]
        for j, n in enumerate(NUM_THREADS):
            ns = n // (1 + ratio)
            nw = ratio * ns
            create_input_file(nw, ns, M, lambda_w, lambda_s, k)
            for _ in range(NUM_RUNS):
                run_program()
                for i, v in enumerate(FLAG_ARR):
                    upd, scan, _ = v
                    L[i][j] += get_worst_time(upd, scan)
        L = [[x / NUM_RUNS for x in l] for l in L]
        for i, v in enumerate(FLAG_ARR):
            _, _, lbl = v
            plt.plot(NUM_THREADS, L[i], label=f'{alg}-{lbl}')
    plt.title(f'Worst-case Scalability ($n_w/n_s = {ratio}$, $M = {M}$, $\\mu_w = {lambda_w}$ ms, $\\mu_s = {lambda_s}$ ms, $k = {K}$)', loc='center', wrap=True)
    plt.xlabel(f'Total number of threads')
    plt.ylabel(f'Worst-case time (ms)')
    plt.legend(bbox_to_anchor=(1.05, 1))
    plt.grid()
    plt.tight_layout()
    plt.savefig(f'{IMG_PATH}/exp2.png')  

def run_exp_3(
    src_list: list[str] = SRC_LIST,
    ns: int = FIXED_NS,
    M: int = M // 2,
    lambda_w: float = LAMBDA,
    lambda_s: float = LAMBDA,
    k: int = K,
):
    plt.clf()
    for src in src_list:
        compile_source(src)
        alg = src.split('.')[0]
        L = [[0.0] * len(RATIO) for _ in range(len(FLAG_ARR))]
        for j, ratio in enumerate(RATIO):
            nw = ratio * ns
            create_input_file(nw, ns, M, lambda_w, lambda_s, k)
            for _ in range(NUM_RUNS):
                run_program()
                for i, v in enumerate(FLAG_ARR):
                    upd, scan, _ = v
                    L[i][j] += get_worst_time(upd, scan)
        L = [[x / NUM_RUNS for x in l] for l in L]
        for i, v in enumerate(FLAG_ARR):
            _, _, lbl = v
            plt.plot(RATIO, L[i], label=f'{alg}-{lbl}')
    plt.title(f'Average-case Impact of Update on Scan ($n_s = {ns}$, $M = {M}$, $\\mu_w = {lambda_w}$ ms, $\\mu_s = {lambda_s}$ ms, $k = {k}$)', loc='center', wrap=True)
    plt.xlabel(f'$n_w / n_s$')
    plt.ylabel(f'Average-case time (ms)')
    plt.legend(bbox_to_anchor=(1.05, 1))
    plt.grid()
    plt.tight_layout()
    plt.savefig(f'{IMG_PATH}/exp3.png')  

def run_exp_4(
    src_list: list[str] = SRC_LIST,
    ns: int = FIXED_NS,
    M: int = M // 2,
    lambda_w: float = LAMBDA,
    lambda_s: float = LAMBDA,
    k: int = K,
):
    plt.clf()
    for src in src_list:
        compile_source(src)
        alg = src.split('.')[0]
        L = [[0.0] * len(RATIO) for _ in range(len(FLAG_ARR))]
        for j, ratio in enumerate(RATIO):
            nw = ratio * ns
            create_input_file(nw, ns, M, lambda_w, lambda_s, k)
            for _ in range(NUM_RUNS):
                run_program()
                for i, v in enumerate(FLAG_ARR):
                    upd, scan, _ = v
                    L[i][j] += get_worst_time(upd, scan)
        L = [[x / NUM_RUNS for x in l] for l in L]
        for i, v in enumerate(FLAG_ARR):
            _, _, lbl = v
            plt.plot(RATIO, L[i], label=f'{alg}-{lbl}')
    plt.title(f'Worst-case Impact of Update on Scan ($n_s = {ns}$, $M = {M}$, $\\mu_w = {lambda_w}$ ms, $\\mu_s = {lambda_s}$ ms, $k = {k}$)', loc='center', wrap=True)
    plt.xlabel(f'$n_w / n_s$')
    plt.ylabel(f'Worst-case time (ms)')
    plt.legend(bbox_to_anchor=(1.05, 1))
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