#!/usr/bin/env python3

import yaml
from pathlib import Path
from shlex import quote
from shutil import rmtree
import sys
import subprocess

script_dir = Path(__file__).parent


def parse_args():
    import argparse
    ap = argparse.ArgumentParser(description="Build deps into .deps")
    ap.add_argument("--profile", type=str,
                    help="Name of build profile. Should be different for each CMake toolchain "
                         "or build environment.")
    ap.add_argument("--toolchain", type=str,
                    help="CMake toolchain to use when building packages.")
    ap.add_argument("--force", action='store_true',
                    help="Force rebuild of all deps.")
    ap.add_argument("name", nargs='*', help="Names of deps to install (default is all deps)")
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
            yield name, info['upstream'], info.get('upstream_tool', 'cmake'), info.get('upstream_defs', '')


def main():
    args = parse_args()
    platform = f"{sys.platform}-{args.profile or 'default'}"
    force_rebuild = args.force
    filter_deps = set(args.name)

    deps_dir = script_dir / '.deps'
    deps_platform_dir = deps_dir / f".{platform}"
    deps_source_dir = deps_dir / '.source'
    deps_build_dir = deps_dir / '.build'

    for name, upstream, build_tool, defs in upstream_requirements():
        if filter_deps and name not in filter_deps:
            continue
        repo, project, git_ref = upstream.split(' ')
        url = f"https://{repo}/{project}.git"
        print(f"* {sgr('1;36')}{name}{sgr('0')} ({url})")

        package_dir = deps_dir / name
        install_dir = deps_platform_dir / name / git_ref
        install_dir_rel = install_dir.relative_to(deps_dir)
        source_dir = deps_source_dir / name / git_ref
        build_dir = deps_build_dir / name / git_ref

        if install_dir.exists() and not force_rebuild:
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

        if force_rebuild:
            rmtree(build_dir)
        build_dir.mkdir(parents=True, exist_ok=True)
        tool_args = ["-D" + d for d in defs.split()]
        if args.toolchain is not None:
            tool_args += ["--toolchain", args.toolchain]
        if build_tool == 'cmake':
            run(["cmake", "-G", "Ninja",
                 "-S", source_dir, "-B", build_dir,
                 "-DCMAKE_BUILD_TYPE=Release",
                 "-DCMAKE_POSITION_INDEPENDENT_CODE=ON",
                 f"-DCMAKE_INSTALL_PREFIX={install_dir}",
                 *tool_args])
            run(["cmake", "--build", build_dir])
            run(["cmake", "--install", build_dir])
        elif build_tool == 'meson':
            run(["meson", "setup", build_dir, source_dir,
                 "--reconfigure",
                 "--backend", "ninja",
                 "--buildtype", "release",
                 "--prefix", install_dir,
                 *tool_args])
            run(["ninja", "-C", build_dir])
            run(["ninja", "-C", build_dir, "install"])
        else:
            print(f"\nERROR: Unsupported build tool: {build_tool}", file=sys.stderr)
            exit(1)

        package_dir.unlink(missing_ok=True)
        package_dir.symlink_to(install_dir_rel, target_is_directory=True)


if __name__ == '__main__':
    try:
        main()
    except subprocess.CalledProcessError as e:
        print(f"\nERROR: Command failed (exit code {e.returncode})", file=sys.stderr)
        exit(1)
