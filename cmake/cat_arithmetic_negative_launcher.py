#!/usr/bin/env python3
# This file is flagrantly "vibe-coded". It may not be up to the standards of most libCat code.

import argparse
import fcntl
import os
import subprocess
import sys


bold = "\033[1m"
red_bold = "\033[1;31m"
reset = "\033[0m"


def find_output_path(command):
    for index, argument in enumerate(command):
        if argument == "-o" and index + 1 < len(command):
            return command[index + 1]
        if argument.startswith("-o") and len(argument) > 2:
            return argument[2:]
    return None


def find_source_path(command):
    for argument in reversed(command):
        if argument.endswith((".cc", ".cpp", ".cxx")) and os.path.exists(argument):
            return argument
    return None


def touch_stale(path, source_path):
    if not path:
        return
    directory = os.path.dirname(path)
    if directory:
        os.makedirs(directory, exist_ok=True)
    with open(path, "ab"):
        pass
    if source_path:
        source_time = os.path.getmtime(source_path)
        os.utime(path, (source_time - 1, source_time - 1))
    else:
        os.utime(path, (0, 0))


def write_text(path, text):
    directory = os.path.dirname(path)
    if directory:
        os.makedirs(directory, exist_ok=True)
    with open(path, "w", encoding="utf-8") as file:
        file.write(text)


def update_progress(path, total):
    directory = os.path.dirname(path)
    if directory:
        os.makedirs(directory, exist_ok=True)
    lock_path = f"{path}.lock"
    with open(lock_path, "w", encoding="utf-8") as lock_file:
        fcntl.flock(lock_file, fcntl.LOCK_EX)
        try:
            with open(path, "r", encoding="utf-8") as progress_file:
                done = int(progress_file.read().strip() or "0")
        except FileNotFoundError:
            done = 0
        done += 1
        write_text(path, f"{done}\n")
        fcntl.flock(lock_file, fcntl.LOCK_UN)
    if done % 25 == 0 or done == total:
        print(f"-- {done}/{total} cases type-checked.", flush=True)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--target", required=True)
    parser.add_argument("--marker", required=True)
    parser.add_argument("--diagnostic", required=True)
    parser.add_argument("--progress", required=True)
    parser.add_argument("--total", type=int, required=True)
    parser.add_argument("command", nargs=argparse.REMAINDER)
    arguments = parser.parse_args()

    if not arguments.command:
        print("cat arithmetic negative launcher: missing compiler command",
              file=sys.stderr)
        return 2

    result = subprocess.run(
        arguments.command,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False)
    stdout = result.stdout.decode("utf-8", errors="replace")
    stderr = result.stderr.decode("utf-8", errors="replace")
    if result.returncode == 0:
        write_text(arguments.marker, f"{arguments.target}\n")
        status = "unexpected success"
        print(f"{bold}{arguments.target}{reset}", file=sys.stderr, flush=True)
        print(f"Compiled successfully. {red_bold}Expected a type error!{reset}",
              file=sys.stderr,
              flush=True)
    else:
        status = f"expected failure, exit code {result.returncode}"

    write_text(
        arguments.diagnostic,
        f"==== {arguments.target} ({status}) ====\n"
        f"stderr:\n{stderr}\n"
        f"stdout:\n{stdout}\n")
    touch_stale(find_output_path(arguments.command),
                find_source_path(arguments.command))
    update_progress(arguments.progress, arguments.total)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
