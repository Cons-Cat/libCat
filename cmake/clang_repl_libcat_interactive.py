#!/usr/bin/env python3
from __future__ import annotations

import errno
import fcntl
import os
import select
import signal
import sys
import termios
import tty


def _write_all(fd: int, data: bytes) -> None:
    while data:
        written = os.write(fd, data)
        data = data[written:]


def _copy_window_size(master_fd: int) -> None:
    try:
        size = fcntl.ioctl(sys.stdin.fileno(), termios.TIOCGWINSZ, b"\0" * 8)
        fcntl.ioctl(master_fd, termios.TIOCSWINSZ, size)
    except OSError:
        pass


def main() -> int:
    if len(sys.argv) < 4 or sys.argv[2] != "--":
        sys.exit(f"usage: {sys.argv[0]} <libcat.so> -- <clang-repl> [args...]")

    libcat_so = sys.argv[1]
    command = sys.argv[3:]
    stdin_fd = sys.stdin.fileno()
    stdout_fd = sys.stdout.fileno()

    if not os.isatty(stdin_fd):
        sys.exit("clang-repl-libcat interactive relay requires tty stdin")

    pid, master_fd = os.forkpty()
    if pid == 0:
        os.execvp(command[0], command)

    _copy_window_size(master_fd)
    old_winch = signal.getsignal(signal.SIGWINCH)
    signal.signal(signal.SIGWINCH, lambda _signum, _frame: _copy_window_size(master_fd))

    original_terminal = termios.tcgetattr(stdin_fd)
    try:
        tty.setraw(stdin_fd)
        _write_all(master_fd, f"%lib {libcat_so}\n".encode())
        initial_input = os.environ.get("CAT_REPL_INITIAL_INPUT", "")
        if initial_input:
            _write_all(master_fd, initial_input.encode())
            if not initial_input.endswith("\n"):
                _write_all(master_fd, b"\n")

        watched = [stdin_fd, master_fd]
        while True:
            try:
                ready, _, _ = select.select(watched, [], [])
            except InterruptedError:
                continue

            if stdin_fd in ready:
                data = os.read(stdin_fd, 4096)
                if not data:
                    watched.remove(stdin_fd)
                    continue
                _write_all(master_fd, data)

            if master_fd in ready:
                try:
                    data = os.read(master_fd, 4096)
                except OSError as error:
                    if error.errno == errno.EIO:
                        break
                    raise
                if not data:
                    break
                _write_all(stdout_fd, data)
    finally:
        termios.tcsetattr(stdin_fd, termios.TCSADRAIN, original_terminal)
        signal.signal(signal.SIGWINCH, old_winch)

    _, status = os.waitpid(pid, 0)
    if os.WIFEXITED(status):
        return os.WEXITSTATUS(status)
    if os.WIFSIGNALED(status):
        return 128 + os.WTERMSIG(status)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
