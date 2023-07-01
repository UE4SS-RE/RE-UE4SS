#!/usr/bin/env python3

import subprocess
import os

# change dir to repo root
os.chdir(os.path.dirname(__file__))

build_dir = 'docs'

# build release as the root
subprocess.run(['mdbook', 'build', os.path.join('versions', 'release'), '-d', os.path.abspath(build_dir)])

for version in os.listdir('versions'):
    version_build_dir = os.path.join(build_dir, version)
    subprocess.run(['mdbook', 'build', os.path.join('versions', version), '-d', os.path.abspath(version_build_dir)])
