# pyright: strict
"""Reaching a DuetScreen over SSH as root: running commands on it, starting and stopping DuetScreen, and
copying files to it. The screen runs dropbear and busybox, so the commands sent stick to what busybox's
shell and tools understand."""

from __future__ import annotations

import hashlib
import shlex
import shutil
import subprocess
import tarfile
from collections.abc import Iterator, Sequence
from pathlib import Path
from typing import IO

LOG = '/var/log/DuetScreen.log'
"""DuetScreen's log on the screen; spdlog rotates it to DuetScreen.1.log, DuetScreen.2.log..."""

BINARY = '/usr/bin/DuetScreen'
"""DuetScreen on the screen."""

ASSETS = '/etc/assets'
"""Where the screen keeps the assets."""

CONFIG = '/etc/duetscreen.json'
"""The screen's configuration file."""

MONITOR = '/etc/init.d/S21DuetScreenMonitor'
"""The service that starts DuetScreen at boot and restarts it when it stops."""

SERVICE = '/etc/init.d/20DuetScreen'
"""The service that runs DuetScreen once, which the monitor uses."""


def ssh_program() -> str:
    """The ssh to use. Under WSL the Windows ssh.exe has the user's keys and agent, which a script does not
    get through the aliases an interactive shell may have for it, so it is used when it is found."""
    return 'ssh.exe' if shutil.which('ssh.exe') else 'ssh'


def ssh_command(host: str, command: str) -> list[str]:
    """The command line that runs a shell command on a screen."""
    return [ssh_program(), f'root@{host}', command]


def run_on(host: str, command: str) -> bool:
    """Run a shell command on a screen, showing it first, and say whether it succeeded."""
    print(f'{host}$ {command}', flush=True)
    return subprocess.run(ssh_command(host, command), check=False).returncode == 0


def stop(host: str) -> bool:
    """Stop DuetScreen and the monitor that would restart it."""
    return run_on(host, f'{MONITOR} stop; start-stop-daemon -K -n DuetScreen; true')


def start(host: str, monitor: bool) -> bool:
    """Start DuetScreen as it starts at boot, under the monitor, or else on its own so it stays stopped
    once it exits."""
    return run_on(host, f'{MONITOR} start' if monitor else f'{SERVICE} start')


def lines_of(host: str, command: str) -> Iterator[str]:
    """The lines a shell command on a screen prints, as it prints them, without their line ends."""
    with subprocess.Popen(ssh_command(host, command), stdout=subprocess.PIPE) as process:
        assert process.stdout is not None
        try:
            for line in process.stdout:
                yield line.decode('utf-8', errors='replace').rstrip('\r\n')
        finally:
            process.terminate()


def copy_to(host: str, files: Sequence[tuple[Path, str]], before: str = 'true') -> bool:
    """Copy files and directories to a screen in one tar stream over SSH, then sync them to its card so
    a power cycle cannot lose them. Each is a local path and the absolute path it goes to. before is a
    shell command run on the screen first, such as one removing what the files replace."""
    remote = f'{before} && tar -x -f - -C / && sync'
    shown = ' '.join(shlex.quote(target) for _, target in files) if len(files) <= 5 else f'{len(files)} files'
    print(f'{host}$ {remote}  < {shown}', flush=True)
    with subprocess.Popen(ssh_command(host, remote), stdin=subprocess.PIPE) as process:
        stdin: IO[bytes] | None = process.stdin
        assert stdin is not None
        try:
            with tarfile.open(fileobj=stdin, mode='w|', format=tarfile.GNU_FORMAT) as tar:
                for source, target in files:
                    tar.add(source, arcname=target.lstrip('/'), filter=as_installed)
        except BrokenPipeError:
            pass  # the screen stopped reading; its exit status says why
        finally:
            stdin.close()
        return process.wait() == 0


def sync_to(host: str, source: Path, target: str) -> bool:
    """Make a file or directory on a screen the same as a local one, sending only what differs: the files
    that are new or changed, by their MD5, are copied, and those no longer there locally are removed. The
    screen has no rsync, but its busybox has md5sum."""
    local = _local_digests(source)
    remote = _remote_digests(host, target)
    changed = [name for name, digest in local.items() if remote.get(name) != digest]
    removed = sorted(remote.keys() - local.keys())
    print(f'{target}: {len(changed)} to copy, {len(removed)} to remove, {len(local) - len(changed)} unchanged')
    if not changed and not removed:
        return True
    remove = 'true'
    if removed:
        # Then any folders that are left empty, deepest first; rmdir leaves those with files in them alone
        remove = (f'cd {shlex.quote(target)} && rm -f -- {" ".join(shlex.quote(name) for name in removed)} && '
                  f'{{ find . -type d | sort -r | xargs rmdir 2>/dev/null; true; }}')
    if not changed:
        return run_on(host, f'{remove} && sync')
    return copy_to(host, [(source / name if name else source, f'{target}/{name}' if name else target)
                          for name in changed], before=remove)


def _local_digests(source: Path) -> dict[str, str]:
    """The MD5 of each local file under a directory, by its path in it, or of a file, by the empty path."""
    files = {'': source} if source.is_file() else {p.relative_to(source).as_posix(): p
                                                    for p in sorted(source.rglob('*')) if p.is_file()}
    return {name: hashlib.md5(path.read_bytes()).hexdigest() for name, path in files.items()}


def _remote_digests(host: str, target: str) -> dict[str, str]:
    """The MD5 of each file on a screen under a directory, by its path in it, or of a file, by the empty
    path; none if it is not there."""
    quoted = shlex.quote(target)
    command = (f'if [ -d {quoted} ]; then cd {quoted} && find . -type f -exec md5sum {{}} +; '
               f'elif [ -f {quoted} ]; then md5sum < {quoted}; fi')
    digests: dict[str, str] = {}
    for line in lines_of(host, command):
        digest, _, name = line.partition('  ')
        digests['' if name == '-' else name.removeprefix('./')] = digest
    return digests


def as_installed(info: tarfile.TarInfo) -> tarfile.TarInfo:
    """A file in a tar archive as the screen should have it: root's, readable by all, as Buildroot installs
    them."""
    info.uid = info.gid = 0
    info.uname = info.gname = 'root'
    info.mode = 0o755 if info.isdir() or info.mode & 0o111 else 0o644
    return info
