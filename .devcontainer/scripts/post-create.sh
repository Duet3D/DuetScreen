#!/usr/bin/env bash
set -euo pipefail

duetscreen_workspace="${DUETSCREEN_WORKSPACE:-/workspaces/DuetScreen}"
buildroot_workspace="${BUILDROOT_WORKSPACE:-/workspaces/buildroot-duetscreen}"

cd "${duetscreen_workspace}"

# Install the Python requirements into the image's /opt/venv (already on PATH, see
# the Dockerfile). It is root-owned, so install through sudo; this also keeps it
# working when the container user's UID is remapped to match the host.
sudo /opt/venv/bin/pip install --root-user-action=ignore -r requirements.txt

# The `Run LVGL Tests` task needs LVGL's own Python prerequisites (pypng, lz4, ...).
# Read them from the submodule so they track LVGL updates.
lvgl_pip_requirements=libraries/lvgl/scripts/prerequisites-pip.txt
if [[ -f "${lvgl_pip_requirements}" ]]; then
    sudo /opt/venv/bin/pip install --root-user-action=ignore -r "${lvgl_pip_requirements}"
else
    echo "WARNING: ${lvgl_pip_requirements} not found; is the lvgl submodule checked out?" >&2
    echo "         The LVGL test task will fail until it is and the container is rebuilt." >&2
fi

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
