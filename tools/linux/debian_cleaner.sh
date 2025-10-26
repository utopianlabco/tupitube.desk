#!/bin/bash

# This command lists all modified files in a machine-readable format.
# The `cut -c4-` part removes the "M  " prefix from each line.
git status --porcelain | grep "^ M" | cut -c4- | xargs git checkout --

# Optional: Add an echo statement to confirm the script has finished.
echo "Reverted the listed modified files."
