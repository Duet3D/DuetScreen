#!/usr/bin/env bash
set -euo pipefail

workspace_dir="${1:-$(pwd)}"
workspace_dir="$(cd "${workspace_dir}" && pwd)"
parent_dir="$(cd "${workspace_dir}/.." && pwd)"
buildroot_dir="${parent_dir}/buildroot-duetscreen"
override_file="${workspace_dir}/.devcontainer/docker-compose.buildroot.yml"

if [[ -d "${buildroot_dir}" ]]; then
    cat > "${override_file}" <<EOF
services:
  dev:
    volumes:
      - type: bind
        source: "${buildroot_dir}"
        target: /workspaces/buildroot-duetscreen
EOF
    echo "Devcontainer: binding ${buildroot_dir} to /workspaces/buildroot-duetscreen"
else
    cat > "${override_file}" <<'EOF'
services:
  dev: {}
EOF
    echo "Devcontainer: no ../buildroot-duetscreen folder found; skipping optional mount"
fi
