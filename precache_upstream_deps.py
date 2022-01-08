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
            yield name, info['upstream']


def main():
    for name, upstream in upstream_requirements():
        repo, project, git_ref = upstream.split(' ')
        url = f"https://{repo}/{project}.git"
        print(f"* {name} ({url})")
        with tempfile.TemporaryDirectory() as temp_dir:
            print(f"Cloning into {temp_dir}")
            git = Git(folder=temp_dir)
            git.clone(url, git_ref, shallow=True)
            run(["conan", "export", temp_dir])


if __name__ == '__main__':
    main()
