#!/usr/bin/env python3

import yaml
import tempfile
from pathlib import Path
from subprocess import run
from conans.tools import Git

script_dir = Path(__file__).parent


def requirements():
    with open(script_dir.joinpath('conandata.yml'), newline='') as conandata:
        return yaml.safe_load(conandata)['requirements']


def upstream_requirements():
    for name, info in requirements().items():
        if 'upstream' in info:
            yield name, info['upstream'], info.get('cmake_defs', '')


def main():
    for name, upstream, cmake_defs in upstream_requirements():
        repo, project, git_ref = upstream.split(' ')
        url = f"https://{repo}/{project}.git"
        print(f"* {name} ({url})")
        package_dir = script_dir.joinpath(".deps").joinpath(name)
        if package_dir.exists():
            print(f"Already installed at {package_dir}")
        else:
            with tempfile.TemporaryDirectory() as temp_dir:
                source_dir = Path(temp_dir).joinpath("source")
                source_dir.mkdir()
                build_dir = Path(temp_dir).joinpath("build")
                build_dir.mkdir()
                print(f"Cloning into {source_dir}")
                git = Git(folder=source_dir)
                git.clone(url, git_ref, shallow=True)

                run(["cmake", "-S", source_dir, "-B", build_dir,
                     "-DCMAKE_BUILD_TYPE=Release",
                     f"-DCMAKE_INSTALL_PREFIX={package_dir}"
                     ] + ['-D' + d for d in cmake_defs.split()])
                run(["cmake", "--build", build_dir])
                run(["cmake", "--install", build_dir])


if __name__ == '__main__':
    main()
