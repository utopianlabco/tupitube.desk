#!/bin/bash

BASE_PATH="/home/xtingray/tupitube"
# Use a cleaner directory structure for building
BUILD_DIR="${BASE_PATH}/build/tupitube-0.2.23-1"

# 1. Clean up old build artifacts
rm -rf "${BUILD_DIR}"
rm -f "${BASE_PATH}"/debian/*.xz
rm lintian*txt # This removes lintian files in the current working directory, maybe move this path

# 2. Copy the source code (including your custom debian folder)
cp -rv "${BASE_PATH}/sources/tupitube.desk" "${BUILD_DIR}"

# 3. Change into the build directory
cd "${BUILD_DIR}"

# 4. Create the source tarball (dpkg-buildpackage needs this)
# Assuming version 0.2.23 (from the directory name)
cd ..
tar -czf tupitube_0.2.23.orig.tar.gz tupitube-0.2.23-1
cd tupitube-0.2.23-1

# 5. Build the package using your existing, correct rules and control files
dpkg-buildpackage -us -uc
