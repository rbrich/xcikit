#!/usr/bin/env python3
import sys
from pathlib import Path

script_dir = Path(__file__).parent


def debug(_msg):
    pass


def parse_args():
    import argparse
    ap = argparse.ArgumentParser()
    ap.add_argument('option', type=str, nargs='*',
                    help="Project option from conanfile.py that are set to True.")
    ap.add_argument("-v", "--verbose", action="store_true",
                    help="Print generated CMakeLists.txt, cmake command-line and full output.")
    args = ap.parse_args()
    if args.verbose:
        global debug
        debug = lambda msg: print(msg)
    return args.option


def requirements():
    import csv
    with open(script_dir.joinpath('requirements.csv'), newline='') as csvfile:
        reader = csv.reader(csvfile, delimiter=',', quotechar='"')
        for row in reader:
            if row[0].strip()[0] in '<#':
                continue  # header or comment
            row = [x.strip() for x in row]
            yield row


def filtered_requirements(options):
    for row in requirements():
        if not row[4] or row[4] in options:
            yield row


def detect_deps(reqs):
    import tempfile
    import textwrap
    from subprocess import run

    name_to_sysopt = {name: opt for _, name, _, _, _, opt in reqs}

    items = ';'.join(f"{name}/{ver}" for _, name, ver, _, _, _ in reqs)
    with tempfile.TemporaryDirectory() as tmp_dir:
        cml = textwrap.dedent("""
                    cmake_minimum_required(VERSION 3.13)
                    project(SystemPackageFinder CXX)
                    list(APPEND CMAKE_MODULE_PATH """ + str(script_dir.joinpath('cmake')) + """)
                    foreach (ITEM IN LISTS DEPS)
                        string(REPLACE "/" ";" ITEM ${ITEM})
                        list(GET ITEM 0 NAME)
                        list(GET ITEM 1 VERSION)
                        find_package(${NAME} ${VERSION})
                        if (${NAME}_FOUND)
                            message(NOTICE "FOUND ${NAME} ${${NAME}_VERSION}")
                        endif()
                    endforeach()
                """)
        debug(cml)
        with open(tmp_dir + "/CMakeLists.txt", 'w') as f:
            f.write(cml)
        cmd = f"cmake . -G Ninja -DDEPS='{items}'"
        debug(cmd)
        p = run(cmd, shell=True,
                capture_output=True, encoding='UTF-8', cwd=tmp_dir)
        debug(p.stdout)
        if p.returncode != 0:
            print(f'Failed:\n{p.stderr}', file=sys.stderr)
            return
        for line in p.stderr.splitlines():
            if line.startswith('FOUND '):
                _, name, version = line.split(' ')
                print(f"Found: {name} {version}")
                yield name_to_sysopt[name]


def main():
    options = parse_args()
    reqs = tuple(filtered_requirements(options))
    print(' '.join(f'-o xcikit:{o}=True' for o in detect_deps(reqs)))


if __name__ == '__main__':
    main()
