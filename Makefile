# CorvusRetro - CorvusSoftware
# Copyright (C) 2026 CorvusSoftware
# SPDX-License-Identifier: AGPL-3.0-only
CMAKE ?= cmake
SANITIZE ?= OFF
.PHONY: test clean
test:
	"$(CMAKE)" -S app/src/main/cpp -B build/native -DCMAKE_BUILD_TYPE=Release -DCORVUS_SANITIZE=$(SANITIZE)
	"$(CMAKE)" --build build/native --parallel 2
	"$(CMAKE)" -E chdir build/native "$(CMAKE)" --build . --target test -- ARGS=--output-on-failure
clean:
	rm -rf build
