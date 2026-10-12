#!/usr/bin/env python3
"""Checks the keyboard on a real terminal (a pseudo-terminal).

Usage: keys_terminal.py ES WORKDIR

Each case builds a small EasyScript program and runs it from a shell whose
terminal is a pseudo-terminal (stdin, stdout and stderr, and its controlling
terminal, so Ctrl+C sends SIGINT). The shell prints the terminal's settings
("stty -g") before and after the program and its exit status. The test types
keys into it, and checks:
  - keys arrive one at a time, without Enter, and aren't shown as typed;
  - afterwards the terminal's settings are exactly what they were, whether
    the program ended normally, stopped with an error, or got Ctrl+C;
  - "ask" still reads a whole line, shown as typed, after keys were read.
Exits 1 with a message on the first failure."""

import fcntl
import os
import select
import subprocess
import sys
import termios
import time

ES, WORK = sys.argv[1], sys.argv[2]

CASES = [
    # name, program, keys to type (each after the program asks), expected exit, text the output must contain
    ("normal end",
     'say "ready"\nwait for a key and call it key\nsay "got " followed by key\n',
     [b"x"], 0, "got x"),
    ("arrow key",
     'say "ready"\nwait for a key and call it key\nsay "got " followed by key\n',
     [b"\x1b[A"], 0, "got up"),
    ("error after keys",
     'say "ready"\nwait for a key and call it key\nsay 1 divided by 0\n',
     [b"x"], 1, "You divided by zero."),
    ("ctrl+c while waiting",
     'say "ready"\nwait for a key and call it key\nsay "never"\n',
     [b"\x03"], 130, "ready"),
    ("ask after keys",
     'say "ready"\nwait for a key and call it key\nask "Name? " and call it name\nsay "hi " followed by name\n',
     [b"x", b"Ada\r"], 0, "hi Ada"),
    ("key being pressed",
     'say "ready"\nwait 1 second\nlet key be the key being pressed\nif key is not nothing, say "pressed " followed by key\n',
     [b"q"], 0, "pressed q"),
]


def settings(stty):
    """stty -g without macOS's PENDIN bit in lflag: a state the kernel sets by
    itself when line mode comes back with input waiting, not a setting."""
    parts = []
    for part in stty.split(":"):
        if part.startswith("lflag="):
            part = "lflag=%x" % (int(part[6:], 16) & ~0x20000000)
        parts.append(part)
    return ":".join(parts)


def controlling_terminal():
    os.setsid()
    fcntl.ioctl(0, termios.TIOCSCTTY, 0)


def read_until(master, want, timeout=10.0):
    data = b""
    end = time.time() + timeout
    while want not in data and time.time() < end:
        ready, _, _ = select.select([master], [], [], 0.1)
        if ready:
            try:
                chunk = os.read(master, 4096)
            except OSError:
                break
            if not chunk:
                break
            data += chunk
    return data


def run_case(name, program, keys, want_status, want_text):
    source = os.path.join(WORK, "keys.es")
    binary = os.path.join(WORK, "keys_program")
    with open(source, "w") as f:
        f.write(program)
    subprocess.run([ES, "build", source, "-o", binary], check=True)
    master, slave = os.openpty()
    script = 'trap : INT; stty -g; echo start; "$0"; echo "status=$?"; stty -g; echo done'
    child = subprocess.Popen(["sh", "-c", script, binary], stdin=slave, stdout=slave, stderr=slave,
                             preexec_fn=controlling_terminal)
    output = read_until(master, b"ready")
    time.sleep(0.3)  # until it's waiting for the key
    for key in keys:
        os.write(master, key)
        time.sleep(0.3)
    output += read_until(master, b"done")
    child.wait(timeout=10)
    os.close(master)
    os.close(slave)
    text = output.decode(errors="replace").replace("\r\n", "\n")
    lines = text.split("\n")
    if "start" not in lines or not any(line.startswith("status=") for line in lines):
        raise SystemExit(f"{name}: the program didn't end; output:\n{text}")
    before = lines[lines.index("start") - 1]
    status_at = [i for i, line in enumerate(lines) if line.startswith("status=")][-1]
    status, after = int(lines[status_at][7:]), lines[status_at + 1]
    shown = "\n".join(lines[lines.index("start") + 1:status_at])
    if status != want_status:
        raise SystemExit(f"{name}: exit status {status}, expected {want_status}; output:\n{shown}")
    if want_text not in shown:
        raise SystemExit(f"{name}: expected {want_text!r} in the output:\n{shown}")
    if settings(before) != settings(after):
        raise SystemExit(f"{name}: the terminal wasn't put back as it was:\nbefore {before}\nafter  {after}")
    shown_lines = shown.split("\n")
    if name == "normal end" and shown_lines[shown_lines.index("ready") + 1] != "got x":
        raise SystemExit(f"{name}: the key was shown as it was typed:\n{shown}")
    if name == "ask after keys" and "Name? Ada" not in shown:
        raise SystemExit(f"{name}: the answer wasn't shown as it was typed:\n{shown}")


for case in CASES:
    run_case(*case)
