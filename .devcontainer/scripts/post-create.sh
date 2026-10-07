#!/usr/bin/env bash
set -euo pipefail

duetscreen_workspace="${DUETSCREEN_WORKSPACE:-/workspaces/DuetScreen}"
buildroot_workspace="${BUILDROOT_WORKSPACE:-/workspaces/buildroot-duetscreen}"

cd "${duetscreen_workspace}"

# A venv created on the host (e.g. by scripts/install_prerequisites.sh) bakes in
# absolute paths that do not resolve inside the container, so validate an
# existing env/ before reusing it and rebuild it if its interpreter is broken.
if [[ -d env ]] && ! env/bin/python3 -c '' >/dev/null 2>&1; then
    echo "Existing env/ is not usable inside the container; recreating it"
    rm -rf env
fi

if [[ ! -d env ]]; then
    python3 -m venv env
fi

source env/bin/activate
pip install --upgrade pip
pip install -r requirements.txt

if [[ -d "${buildroot_workspace}" ]]; then
    echo "Optional buildroot mount detected at ${buildroot_workspace}"
    if [[ -f "${buildroot_workspace}/.config" ]]; then
        echo "Buildroot already has a .config; leaving it untouched"
    # Never clobber an existing configuration, and never fail container creation
    # if the defconfig is missing on the checked out buildroot branch.
    elif ! make -C "${buildroot_workspace}" duet3d_duetscreen_defconfig; then
        echo "WARNING: 'make duet3d_duetscreen_defconfig' failed in ${buildroot_workspace}." >&2
        echo "         Check out the 'master' branch there and rerun it manually." >&2
    fi
else
    echo "WARNING: no buildroot mount at ${buildroot_workspace}." >&2
    echo "         The T113 build, deploy and remote debug tasks will fail until you" >&2
    echo "         clone https://github.com/Duet3D/buildroot-duetscreen as a sibling" >&2
    echo "         of this repository and rebuild the devcontainer." >&2
fi
