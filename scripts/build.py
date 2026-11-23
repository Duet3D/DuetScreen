#!/usr/bin/env python3
# pyright: strict
"""Build DuetScreen for the simulator, the tests or the screen, and put it on a screen.

What is built, and whether as Release or Debug, chooses the CMake preset (CMakePresets.json), or
--preset names it. The simulator and the tests are built here, in out/build/<preset>. The screen's
DuetScreen is cross-compiled by Buildroot, which has its toolchain: it is cloned beside this repository
(see docs/DEVELOPMENT.md), or BUILDROOT_WORKSPACE says where. --image goes on to make the SD card image of
the whole firmware, and --deploy puts DuetScreen and the assets on a screen over SSH and restarts it
there, copying only the files that differ from the screen's, and removing the assets no longer in
assets/. Both build for the screen without --screen.

    scripts/build.py                              the simulator (Simulation-Release)
    scripts/build.py --debug --fresh              the simulator as Debug (Simulation), configured from scratch
    scripts/build.py --tests                      the tests (Test)
    scripts/build.py --screen                     for the screen (T113-Release)
    scripts/build.py --deploy 192.168.1.20        for the screen, then onto it
    scripts/build.py --deploy 192.168.1.20 --debug --no-start
                                                  as Debug (T113-Debug), left stopped to debug it
    scripts/build.py --clean --image              the SD card image, DuetScreen built from clean
    scripts/build.py --image --hardware-test AD-04-01_
                                                  the image of a hardware test
    scripts/build.py --preset Test -t DuetScreen  a target of a preset
"""

from __future__ import annotations

import argparse
import sys
import tempfile
from collections.abc import Sequence
from dataclasses import dataclass
from pathlib import Path

from common import buildroot, cmake, screen
from common.project import ASSETS, complete, complete_with


@dataclass(frozen=True)
class Options:
    """The command line, typed."""
    preset: str
    targets: list[str]
    fresh: bool
    clean: bool
    jobs: int | None
    duet_simulator: bool
    hardware_test: str | None
    image: bool
    deploy: str | None
    no_start: bool


def parse_options(argv: Sequence[str]) -> Options:
    """Read the command line, and check its options suit the preset."""
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    what = parser.add_argument_group('what is built')
    what.add_argument('-d', '--debug', action='store_true', help='as Debug, with profiling (default: Release)')
    what.add_argument('-p', '--profiling', action='store_true', help='as Release with profiling (the screen only)')
    what.add_argument('--tests', action='store_true', help='the tests, not the simulator')
    what.add_argument('-s', '--screen', action='store_true', help='for the screen, not the simulator')
    complete_with(what.add_argument('--preset', help='this CMake preset, rather than the one the options above choose'),
                  cmake.presets)
    parser.add_argument('-c', '--clean', action='store_true', help='build from clean')

    local = parser.add_argument_group('the simulator and the tests')
    complete_with(local.add_argument('-t', '--target', dest='targets', action='append', default=[], metavar='TARGET',
                                     help='a target to build (repeatable; default: DuetScreen, or the tests for Test)'),
                  lambda: ['DuetScreen', 'DuetScreen.tests', 'clang-format', 'update_version_h'])
    local.add_argument('-f', '--fresh', action='store_true', help='configure from scratch')
    local.add_argument('-j', '--jobs', type=int, help='parallel jobs (default: as the generator chooses)')

    screen_group = parser.add_argument_group('the screen')
    screen_group.add_argument('--duet-simulator', action='store_true', help='with the simulated Duet to connect to')
    screen_group.add_argument('--hardware-test', metavar='SERIAL_PREFIX',
                              help="as a hardware test image, whose serial numbers start with this")
    screen_group.add_argument('--image', action='store_true', help='then make the SD card image')
    screen_group.add_argument('--deploy', metavar='HOST', help='then copy it to this screen and restart it there')
    screen_group.add_argument('--no-start', action='store_true',
                              help='leave DuetScreen stopped once deployed, e.g. to start it under gdbserver')
    complete(parser)
    args = vars(parser.parse_args(argv))
    chosen = {name: args.pop(name) for name in ('debug', 'profiling', 'tests', 'screen')}
    preset: str | None = args.pop('preset')
    if preset is not None:
        if any(chosen.values()):
            parser.error('--preset chooses the preset by itself, without --debug, --profiling, --tests or --screen')
        if preset not in cmake.presets():
            parser.error(f'no preset {preset}: choose from {", ".join(cmake.presets())}')
    else:
        # Deploying and making the image are for the screen
        chosen['screen'] = chosen['screen'] or args['deploy'] is not None or args['image']
        try:
            preset = cmake.choose(**chosen)
        except ValueError as e:
            parser.error(str(e))
    options = Options(preset=preset, **args)

    for_screen = options.preset in cmake.screen_presets()
    wrong = ([] if not for_screen else
             [name for name, given in (('--target', options.targets), ('--fresh', options.fresh),
                                       ('--jobs', options.jobs is not None)) if given])
    wrong += ([] if for_screen else
              [name for name, given in (('--duet-simulator', options.duet_simulator),
                                        ('--hardware-test', options.hardware_test is not None),
                                        ('--image', options.image), ('--deploy', options.deploy),
                                        ('--no-start', options.no_start)) if given])
    if wrong:
        parser.error(f'{", ".join(wrong)} cannot be used with {options.preset}')
    if options.no_start and not options.deploy:
        parser.error('--no-start is only for --deploy')
    return options


def build_local(options: Options) -> bool:
    """Build a simulator or test preset, configuring it first if it has not been, or if asked."""
    if options.fresh or not (cmake.build_dir(options.preset) / 'CMakeCache.txt').exists():
        if not cmake.configure(options.preset, fresh=options.fresh):
            return False
    targets = options.targets or cmake.default_targets(options.preset)
    if not cmake.build(options.preset, targets, clean=options.clean, jobs=options.jobs):
        return False
    print(f'Built {", ".join(targets)} in {cmake.build_dir(options.preset)}')
    return True


def build_for_screen(options: Options) -> bool:
    """Build a screen preset with Buildroot, then its SD card image if asked."""
    buildroot.require()
    cmake_options = buildroot.options(options.duet_simulator, options.hardware_test)
    if not buildroot.configure():
        return False
    if options.clean:
        buildroot.clean()
    if not buildroot.make(['duetscreen-reconfigure'], options.preset, cmake_options):
        return False
    print(f'Built {buildroot.BINARY}')
    if options.image:
        if not buildroot.make([], options.preset, cmake_options):
            return False
        print(f'SD card image: {buildroot.IMAGE}')
    return True


def deploy(host: str, start: bool) -> bool:
    """Put DuetScreen, without its debug symbols, and the assets on a screen, and restart it. Only what
    differs from what the screen has is copied, and the assets no longer in assets/ are removed from it."""
    if not screen.stop(host):
        return False
    with tempfile.TemporaryDirectory() as directory:
        binary = buildroot.stripped(buildroot.BINARY, Path(directory) / 'DuetScreen')
        if not screen.sync_to(host, binary, screen.BINARY) or not screen.sync_to(host, ASSETS, screen.ASSETS):
            return False
    if start and not screen.start(host, monitor=True):
        return False
    print(f'Deployed to {host}' + ('' if start else '; DuetScreen is stopped'))
    return True


def main() -> int:
    """Build, and deploy, as asked."""
    options = parse_options(sys.argv[1:])
    if options.preset not in cmake.screen_presets():
        return 0 if build_local(options) else 1
    if not build_for_screen(options):
        return 1
    if options.deploy and not deploy(options.deploy, start=not options.no_start):
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
