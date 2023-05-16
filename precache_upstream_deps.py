#!/usr/bin/env python3

import yaml
from pathlib import Path
from shlex import quote
import sys
import subprocess

script_dir = Path(__file__).parent


def parse_args():
    import argparse
    ap = argparse.ArgumentParser(description="Precache deps into $HOME/.xcikit/deps")
    ap.add_argument("--toolchain", type=str,
                    help="CMake toolchain to use when building packages.")
    return ap.parse_args()


def sgr(code: str):
    """ANSI colors"""
    return f"\x1b[{code}m" if sys.stdout.isatty() else ""


def run(popen_args, *args, **kwargs):
    print(f"> {' '.join(quote(str(x)) for x in popen_args)}")
    subprocess.run(popen_args, *args, **kwargs, check=True)


def requirements():
    with open(script_dir.joinpath('conandata.yml'), newline='') as conandata:
        return yaml.safe_load(conandata)['requirements']


def upstream_requirements():
    for name, info in requirements().items():
        if 'upstream' in info:
            yield name, info['upstream'], info.get('cmake_defs', '')


def main():
    args = parse_args()

    deps_dir = Path.home() / '.xcikit/deps'
    deps_all_dir = deps_dir / '.all'
    deps_source_dir = deps_dir / '.source'
    deps_build_dir = deps_dir / '.build'

    for name, upstream, cmake_defs in upstream_requirements():
        repo, project, git_ref = upstream.split(' ')
        url = f"https://{repo}/{project}.git"
        print(f"* {sgr('1;36')}{name}{sgr('0')} ({url})")

        package_dir = deps_dir / name
        install_dir = deps_all_dir / name / git_ref
        install_dir_rel = install_dir.relative_to(deps_dir)
        source_dir = deps_source_dir / name / git_ref
        build_dir = deps_build_dir / name / git_ref

        if install_dir.exists():
            if package_dir.exists():
                if not package_dir.is_symlink():
                    raise Exception(f"Package dir exists but is not a symlink: {package_dir}")
                if package_dir.resolve() == install_dir:
                    print(f"Already installed at {package_dir} -> {install_dir_rel}")
                    continue
                # Package dir points to different version -> relink
                package_dir.unlink()
            print(f"Linking {package_dir} -> {install_dir_rel}")
            package_dir.symlink_to(install_dir_rel, target_is_directory=True)
            continue

        if not source_dir.exists():
            print(f"Cloning into {source_dir}")
            source_dir.mkdir(parents=True)
            run(["git", "-c", "advice.detachedHead=false",
                 "clone", url, "--depth=1", "--branch", git_ref, source_dir])

        build_dir.mkdir(parents=True, exist_ok=True)
        cmake_args = ["-D" + d for d in cmake_defs.split()]
        if args.toolchain is not None:
            cmake_args += ["--toolchain", args.toolchain]
        run(["cmake", "-G", "Ninja",
             "-S", source_dir, "-B", build_dir,
             "-DCMAKE_BUILD_TYPE=Release",
             f"-DCMAKE_INSTALL_PREFIX={install_dir}",
             *cmake_args])
        run(["cmake", "--build", build_dir])
        run(["cmake", "--install", build_dir])

        package_dir.unlink(missing_ok=True)
        package_dir.symlink_to(install_dir_rel, target_is_directory=True)


if __name__ == '__main__':
    main()
