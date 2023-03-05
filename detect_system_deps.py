#!/usr/bin/env python3
import sys
import yaml
import tempfile
import textwrap
from pathlib import Path
from subprocess import run, DEVNULL

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


def add_required_components(options):
    result = set(options)
    all_tools = None
    for name in options:
        if name == 'widgets':
            result.add('text')
            result.add('graphics')
        if name == 'text':
            result.add('graphics')
        if name == 'script':
            result.add('data')
        if name == 'tools' and all_tools is None:
            all_tools = True
        if name.endswith('_tool'):
            all_tools = False
    if all_tools:
        if 'widgets' in result:
            result.add('shed_tool')
        if 'script' in result:
            result.add('fire_tool')
        if 'data' in result:
            result.add('dati_tool')
        result |= {'ff_tool', 'tc_tool'}
    return result


def requirements():
    with open(script_dir.joinpath('conandata.yml'), newline='') as conandata:
        return yaml.safe_load(conandata)['requirements']


def filtered_requirements(options: set):
    for name, info in requirements().items():
        if 'prereq' not in info or set(info['prereq']).intersection(options):
            yield name, info


def detect_deps(reqs, precached_deps):
    cmake_name_to_sysopt = {info['cmake'].split('/')[0]
                            : f"system_{name}" if 'conan' in info else f"with_{name}"
                            for name, info in reqs}

    items = ';'.join(info['cmake'] for _, info in reqs)
    with tempfile.TemporaryDirectory() as tmp_dir:
        # Convert the path to posix (forward slashes) even on Windows.
        # Paths with backslashes are not supported by CMake.
        custom_modules = '"' + str(script_dir.joinpath('cmake').as_posix()) + '"'
        cml = textwrap.dedent("""
                    cmake_minimum_required(VERSION 3.16)
                    project(SystemPackageFinder CXX)
                    list(APPEND CMAKE_MODULE_PATH """ + custom_modules + """)
                    foreach (ITEM IN LISTS DEPS)
                        string(REPLACE "/" ";" ITEM ${ITEM})
                        list(GET ITEM 0 NAME)
                        list(GET ITEM 1 VERSION)
                        message(STATUS "Find ${NAME} ${VERSION}")
                        find_package(${NAME} ${VERSION})
                        if (${NAME}_FOUND)
                            message(NOTICE "FOUND ${NAME} ${${NAME}_VERSION}")
                        endif()
                    endforeach()
                """)
        debug(cml)
        with open(tmp_dir + "/CMakeLists.txt", 'w') as f:
            f.write(cml)
        # Prefer ninja if available. Needed to allow choice, otherwise `make` installation
        # would be required on unixes, as it's the default in cmake.
        ninja = ""
        if run("command -v ninja", shell=True, stdout=DEVNULL, stderr=DEVNULL).returncode == 0:
            ninja = "-G Ninja"
        cmd = f"cmake . {ninja} -DDEPS='{items}' -DCMAKE_PREFIX_PATH='{';'.join(precached_deps)}'"
        debug(cmd)
        p = run(cmd, shell=True,
                capture_output=True, encoding='UTF-8', cwd=tmp_dir)
        debug(p.stdout)
        debug(p.stderr)
        if p.returncode != 0:
            print(f'Failed: {p.returncode}', file=sys.stderr)
            return
        for line in p.stderr.splitlines():
            if line.startswith('FOUND '):
                _, name, version = line.split(' ')
                print(f"Found: {name} {version}")
                yield cmake_name_to_sysopt[name]


def main():
    options = parse_args()
    options = add_required_components(options)
    reqs = tuple(filtered_requirements(options))
    precached_deps_dir = Path.home().joinpath('.xcikit/deps')
    if precached_deps_dir.is_dir():
        precached_deps = [str(d) for d in precached_deps_dir.glob("[!.]*") if d.is_symlink()]
    else:
        precached_deps = []
    deps = tuple(detect_deps(reqs, precached_deps))
    cmake_args = [f'-DXCI_{o.upper()}=ON' for o in deps if o.startswith('with_')] + \
                 [f"-DCMAKE_PREFIX_PATH={';'.join(precached_deps)}"]
    print(' '.join(cmake_args))
    print(' '.join(f'-o xcikit/*:{o}=True' for o in deps))


if __name__ == '__main__':
    main()
