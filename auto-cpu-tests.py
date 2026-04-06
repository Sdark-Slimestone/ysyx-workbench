#!/usr/bin/env python3
import subprocess
import os
import time

def run_test(name):
    print(f"Running {name}...", end=" ", flush=True)
    # 直接使用完整的测试根目录
    os.chdir("/home/sdark/ysyx-workbench/am-kernels/tests/cpu-tests")  #cd xxx
    proc = subprocess.Popen(
        ["make", "ARCH=riscv32-nemu", f"ALL={name}", "run"],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1
    )
    try:
        output = ""
        while True:
            char = proc.stdout.read(1)
            if not char:
                break
            output += char
            if "(nemu)" in output:
                proc.stdin.write("c\n")
                proc.stdin.flush()
                break
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
    # 直接使用完整的测试用例目录路径
    tests_dir = "/home/sdark/ysyx-workbench/am-kernels/tests/cpu-tests/tests"
    tests = []
    for f in os.listdir(tests_dir):
        if f.endswith(".c"):
            tests.append(f[:-2])
    tests.sort()
    passed = 0
    for t in tests:
        if run_test(t):
            passed += 1
        time.sleep(0)
    print(f"\nPassed {passed} / {len(tests)}")

if __name__ == "__main__":
    main()