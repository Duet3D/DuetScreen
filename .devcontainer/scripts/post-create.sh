#!/usr/bin/env bash
set -euo pipefail

duetscreen_workspace="${DUETSCREEN_WORKSPACE:-/workspaces/DuetScreen}"
buildroot_workspace="${BUILDROOT_WORKSPACE:-/workspaces/buildroot-duetscreen}"

cd "${duetscreen_workspace}"

if [[ ! -d env ]]; then
    python3 -m venv env
fi

source env/bin/activate
pip install --upgrade pip
pip install -r requirements.txt

if [[ -d "${buildroot_workspace}" ]]; then
    echo "Optional buildroot mount detected at ${buildroot_workspace}"
else
    echo "Optional buildroot mount not detected. T113 build tasks will require ../buildroot-duetscreen on the host."
fi
