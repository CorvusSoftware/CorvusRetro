# CorvusRetro - CorvusSoftware
# Copyright (C) 2026 CorvusSoftware
# SPDX-License-Identifier: AGPL-3.0-only
CC ?= cc
CMAKE ?= cmake
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -pedantic -O2
.PHONY: test jni-test core-jni-test clean
test: build/core_test
	./build/core_test
build/core_test: core/src/corvus_core.c core/include/corvus_core.h tests/core_test.c
	mkdir -p build
	$(CC) $(CFLAGS) -Icore/include core/src/corvus_core.c tests/core_test.c -o $@
jni-test:
	test -n "$(JAVA_HOME)"
	mkdir -p build/java
	$(CC) -std=c11 -Wall -Wextra -Werror -fPIC -shared -Icore/include -I"$(JAVA_HOME)/include" -I"$(JAVA_HOME)/include/linux" core/src/corvus_core.c app/src/main/jni/bridge.c -o build/libcorvusretro.so
	"$(JAVA_HOME)/bin/javac" -d build/java app/src/main/java/com/corvussoftware/corvusretro/NativeBridge.java tests/java/com/corvussoftware/corvusretro/NativeBridgeTest.java
	"$(JAVA_HOME)/bin/java" -Xcheck:jni -Djava.library.path=build -cp build/java com.corvussoftware.corvusretro.NativeBridgeTest
core-jni-test:
	test -n "$(JAVA_HOME)"
	"$(CMAKE)" -S app/src/main/cpp -B build/core-native -DCMAKE_BUILD_TYPE=Release
	"$(CMAKE)" --build build/core-native --parallel 2
	mkdir -p build/core-java
	"$(JAVA_HOME)/bin/javac" -d build/core-java app/src/main/java/com/corvussoftware/corvusretro/NativeCore.java tests/java/com/corvussoftware/corvusretro/CorvusCoreTest.java
	"$(JAVA_HOME)/bin/java" -Xcheck:jni -Djava.library.path=build/core-native -cp build/core-java com.corvussoftware.corvusretro.CorvusCoreTest
clean:
	rm -rf build
