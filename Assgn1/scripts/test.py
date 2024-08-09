#!/usr/bin/env python3

'''
File    : test.py
Author  : Gautam Singh (CS21BTECH11018)
Date    : 2024-08-09
Purpose : Run tests and generate plots for report
'''

# Imports
import os
import matplotlib.pyplot as plt

def create_input_file(N: int, S: int, K: int, row_inc: int, mat_file: str, out_file: str) -> int:
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
    with open(out_file, 'w') as outfile:
        outfile.write(f'{N} {S} {K} {row_inc}\n')
        outfile.writelines(L)
    return 0
