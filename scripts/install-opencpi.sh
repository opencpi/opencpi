#!/bin/sh
set -e
./scripts/install-prerequisites.sh
./scripts/build-opencpi.sh
./scripts/test-opencpi.sh
