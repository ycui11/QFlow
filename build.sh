#!/usr/bin/env bash
# Configure, build, and test the QFlow C++ core.
#
# Usage:
#   bash build.sh                 # configure + build + run tests
#   bash build.sh --clear         # wipe build/ first
#   bash build.sh --release       # Release build
#   bash build.sh --no-test       # skip ctest
#   bash build.sh --run           # also run ./build/qflow_cli
#   bash build.sh --clear --run

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${ROOT}/build"
BUILD_TYPE="Debug"
CLEAR=0
RUN=0
RUN_TESTS=1

# Prefer Homebrew CMake on macOS when it is not already on PATH.
export PATH="/opt/homebrew/bin:/usr/local/bin:${PATH}"

usage() {
  cat <<'EOF'
Usage: bash build.sh [options]

Options:
  --clear      Remove the build/ directory before configuring
  --debug      CMAKE_BUILD_TYPE=Debug (default)
  --release    CMAKE_BUILD_TYPE=Release
  --no-test    Skip running ctest after the build
  --run        Run ./build/qflow_cli after a successful build
  -h, --help   Show this help
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --clear) CLEAR=1 ;;
    --debug) BUILD_TYPE="Debug" ;;
    --release) BUILD_TYPE="Release" ;;
    --no-test) RUN_TESTS=0 ;;
    --run) RUN=1 ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage >&2
      exit 1
      ;;
  esac
  shift
done

if ! command -v cmake >/dev/null 2>&1; then
  echo "error: cmake not found. Install with: brew install cmake" >&2
  exit 1
fi

if [[ "${CLEAR}" -eq 1 ]]; then
  echo "==> clearing ${BUILD_DIR}"
  rm -rf "${BUILD_DIR}"
fi

# First configure with Python bindings may download pybind11 (needs network).
# Disable with: QFLOW_BUILD_PYTHON=OFF bash build.sh
CMAKE_ARGS=(-S "${ROOT}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE="${BUILD_TYPE}")
if [[ -n "${QFLOW_BUILD_PYTHON:-}" ]]; then
  CMAKE_ARGS+=(-DQFLOW_BUILD_PYTHON="${QFLOW_BUILD_PYTHON}")
fi

echo "==> configuring (${BUILD_TYPE})"
cmake "${CMAKE_ARGS[@]}"

echo "==> building"
cmake --build "${BUILD_DIR}"

if [[ "${RUN_TESTS}" -eq 1 ]]; then
  echo "==> testing"
  ctest --test-dir "${BUILD_DIR}" --output-on-failure
fi

if [[ "${RUN}" -eq 1 ]]; then
  echo "==> running qflow_cli"
  "${BUILD_DIR}/qflow_cli"
fi

echo "==> done"
