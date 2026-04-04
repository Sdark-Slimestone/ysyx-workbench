#!/usr/bin/env python3
import subprocess
import os
import time

TEST_DIR = "/home/sdark/ysyx-workbench/am-kernels/tests/cpu-tests"
ARCH = "riscv32-nemu"

def run_test(name):
    print(f"Running {name}...", end=" ", flush=True)
    os.chdir(TEST_DIR)
    # 启动 NEMU，通过管道读写
    proc = subprocess.Popen(
        ["make", f"ARCH={ARCH}", f"ALL={name}", "run"],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1
    )
    try:
        # 等待 (nemu) 提示符出现
        output = ""
        while True:
            char = proc.stdout.read(1)
            if not char:
                break
            output += char
            if "(nemu)" in output:
                # 发送 c 命令
                proc.stdin.write("c\n")
                proc.stdin.flush()
                break
        # 继续读取直到遇到 HIT GOOD TRAP 或超时
        timeout = 30
        start = time.time()
        success = False
        while time.time() - start < timeout:
            line = proc.stdout.readline()
            if not line:
                break
            output += line
            if "HIT GOOD TRAP" in line:
                success = True
                break
            if "nemu: ABORT" in line or "out of bound" in line or "invalid opcode" in line:
                break
        # 发送 q 退出
        proc.stdin.write("q\n")
        proc.stdin.flush()
        proc.wait(timeout=2)
    except Exception:
        success = False
    finally:
        proc.terminate()
    if success:
        print("✓ PASS")
    else:
        print("✗ FAIL")
    return success

def main():
    tests_dir = os.path.join(TEST_DIR, "tests")
    tests = []
    for f in os.listdir(tests_dir):
        if f.endswith(".c"):
            tests.append(f[:-2])
    tests.sort()
    passed = 0
    for t in tests:
        if run_test(t):
            passed += 1
        time.sleep(0.5)  # 避免过快启动
    print(f"\nPassed {passed} / {len(tests)}")

if __name__ == "__main__":
    main()