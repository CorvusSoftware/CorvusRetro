# CorvusRetro - CorvusSoftware
# Copyright (C) 2026 CorvusSoftware
# SPDX-License-Identifier: AGPL-3.0-only
CMAKE ?= cmake
.PHONY: test core-jni-test clean
test: core-jni-test
core-jni-test:
	test -n "$(JAVA_HOME)"
	"$(CMAKE)" -S app/src/main/cpp -B build/core-native -DCMAKE_BUILD_TYPE=Release
	"$(CMAKE)" --build build/core-native --parallel 2
	mkdir -p build/core-java
	"$(JAVA_HOME)/bin/javac" -d build/core-java app/src/main/java/com/corvussoftware/corvusretro/NativeCore.java tests/java/com/corvussoftware/corvusretro/CorvusCoreTest.java
	"$(JAVA_HOME)/bin/java" -Xcheck:jni -Djava.library.path=build/core-native -cp build/core-java com.corvussoftware.corvusretro.CorvusCoreTest
clean:
	rm -rf build
