#!/usr/bin/env python3
# pyright: strict
"""Control a DuetScreen over SSH: start and stop DuetScreen, debug it, and copy files to it.

The screen is reached as root, which a development image allows. To build DuetScreen and put it on a
screen, use scripts/build.py --deploy; to read its logs, scripts/filter_logs.py --remote.

    scripts/screen.py 192.168.1.20 start                 restart DuetScreen as at boot, under its monitor
    scripts/screen.py 192.168.1.20 start --no-monitor    restart it on its own, so it stays stopped once it exits
    scripts/screen.py 192.168.1.20 stop
    scripts/screen.py 192.168.1.20 gdbserver             run it under gdbserver, for the Remote Debug launch
    scripts/screen.py 192.168.1.20 push assets           sync its assets with assets/, and restart it
    scripts/screen.py 192.168.1.20 push config           replace its configuration with config.json
"""

from __future__ import annotations

import argparse
import sys
from collections.abc import Sequence
from dataclasses import dataclass
from pathlib import Path

from common import screen
from common.project import ASSETS, ROOT, complete, complete_files

GDB_PORT = 3000
"""The port gdbserver listens on, which the Remote Debug launch configurations connect to."""


@dataclass(frozen=True)
class Options:
    """The command line, typed."""
    host: str
    command: str
    monitor: bool
    port: int
    what: str | None
    file: Path | None
    no_start: bool


def parse_options(argv: Sequence[str]) -> Options:
    """Read the command line."""
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('host', help="the screen's address")
    commands = parser.add_subparsers(dest='command', required=True, metavar='COMMAND')
    start = commands.add_parser('start', help='restart DuetScreen')
    start.add_argument('--no-monitor', dest='monitor', action='store_false',
                       help='without the monitor, which would restart it when it exits')
    commands.add_parser('stop', help='stop DuetScreen and its monitor')
    gdbserver = commands.add_parser('gdbserver', help='restart DuetScreen under gdbserver, until it exits')
    gdbserver.add_argument('--port', type=int, default=GDB_PORT, help=f"gdbserver's port (default: {GDB_PORT})")
    push = commands.add_parser('push', help='copy files to the screen')
    push.add_argument('what', choices=['assets', 'config'],
                      help='assets/ to its assets, or a configuration file to its configuration')
    complete_files(push.add_argument('file', nargs='?', type=Path,
                                     help='the configuration file (default: config.json)'), '.json')
    push.add_argument('--no-start', action='store_true', help='leave DuetScreen stopped')
    parser.set_defaults(monitor=True, port=GDB_PORT, what=None, file=None, no_start=False)  # for the other commands
    complete(parser)
    return Options(**vars(parser.parse_args(argv)))


def push(options: Options) -> bool:
    """Copy the assets or a configuration file to the screen while DuetScreen is stopped, then restart it,
    which reads them as it starts. Only the assets that differ are copied, and those no longer in assets/
    are removed."""
    if options.what == 'assets':
        if options.file is not None:
            sys.exit('push assets takes no file: it syncs assets/')
        source, target = ASSETS, screen.ASSETS
    else:
        source, target = options.file or ROOT / 'config.json', screen.CONFIG
        if not source.is_file():
            sys.exit(f'No configuration file {source}')
    return (screen.stop(options.host) and screen.sync_to(options.host, source, target)
            and (options.no_start or screen.start(options.host, monitor=True)))


def main() -> int:
    """Do what was asked."""
    options = parse_options(sys.argv[1:])
    host = options.host
    match options.command:
        case 'start':
            ok = screen.stop(host) and screen.start(host, options.monitor)
        case 'stop':
            ok = screen.stop(host)
        case 'gdbserver':
            ok = screen.stop(host) and screen.run_on(host, f'gdbserver :{options.port} DuetScreen')
        case _:
            ok = push(options)
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
