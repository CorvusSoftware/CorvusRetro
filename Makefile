# CorvusRetro - CorvusSoftware
# Copyright (C) 2026 CorvusSoftware
# SPDX-License-Identifier: AGPL-3.0-only
CMAKE ?= cmake
SANITIZE ?= OFF
# Native regression tests exercise all official CorvusAPI engines. Android base APK remains core-free.
TEST_CORES ?= gb,gbc,gba
.PHONY: test clean
test:
	"$(CMAKE)" -S app/src/main/cpp -B build/native -DCMAKE_BUILD_TYPE=Release -DCORVUS_SANITIZE=$(SANITIZE) -DCORVUS_OFFICIAL_CORES=$(TEST_CORES)
	"$(CMAKE)" --build build/native --parallel 2
	"$(CMAKE)" -E chdir build/native "$(CMAKE)" --build . --target test -- ARGS=--output-on-failure
clean:
	rm -rf build
