import subprocess
import random
import numpy as np
import matplotlib.pyplot as plt


def generate_test_case(leaf_count, batch_size):
    batch = sorted(random.sample(range(leaf_count), batch_size))
    with open("inp.txt", "w") as f:
        f.write(f"{leaf_count}\n")
        f.write(f"{batch_size}\n")
        f.write(" ".join(map(str, batch)) + "\n")


def run_cpp_file(executable):
    result = subprocess.run([executable], capture_output=True, text=True)
    return float(result.stdout.strip())


def average_time(executable, runs=5):
    times = [run_cpp_file(executable) for _ in range(runs)]
    return np.mean(times)


def main():
    leaf_counts = [2**i for i in range(2, 7)]  # 4, 8, 16, 32, 64
    results = {}

    for leaf_count in leaf_counts:
        results[leaf_count] = {"sequential": [], "angela": [], "atomic": []}
        for batch_size in range(1, leaf_count + 1):
            generate_test_case(leaf_count, batch_size)

            seq_avg_time = average_time("./sequential")
            angela_avg_time = average_time("./angela")
            atomic_avg_time = average_time("./atomic")

            results[leaf_count]["sequential"].append(seq_avg_time)
            results[leaf_count]["angela"].append(angela_avg_time)
            results[leaf_count]["atomic"].append(atomic_avg_time)

            print(
                f"Leaf Count: {leaf_count}, Batch Size: {batch_size}, "
                f"Sequential Time: {seq_avg_time} ms, Angela Time: {angela_avg_time} ms, Atomic Time: {atomic_avg_time} ms"
            )

    for leaf_count in leaf_counts:
        batch_sizes = list(range(1, leaf_count + 1))

        plt.figure(figsize=(10, 6))
        plt.plot(
            batch_sizes,
            results[leaf_count]["sequential"],
            label="Sequential",
            marker="o",
        )
        plt.plot(batch_sizes, results[leaf_count]["angela"], label="Angela", marker="^")
        plt.plot(batch_sizes, results[leaf_count]["atomic"], label="Atomic", marker="x")

        plt.title(f"Execution Time vs. Batch Size (Leaf Count = {leaf_count})")
        plt.xlabel("Batch Size")
        plt.ylabel("Time (ms)")
        plt.legend()
        plt.grid()

        filename = f"execution_time_n_{leaf_count}.png"
        plt.savefig(filename)
        print(f"Plot saved as {filename}")
        plt.show()


if __name__ == "__main__":
    main()
