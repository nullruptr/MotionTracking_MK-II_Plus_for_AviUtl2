# CMake を使うための Makefile (MSVC + Conan)
BUILD_DIR := build/build
BUILD_DIR_DEBUG := build-debug/build
COMPDB_CONAN_DIR := build/compiledb-conan
COMPDB_DIR := build/compiledb

compile:
	cd .. && PATH="/c/Program Files (x86)/Microsoft Visual Studio/Installer:$$PATH" \
		poetry run conan install . --output-folder=src/build -pr:h=profiles/msvc-release --build=missing
	cmake -S . -B $(BUILD_DIR) -G "Visual Studio 17 2022" -A x64 -T v143 \
		-DCMAKE_TOOLCHAIN_FILE=$(BUILD_DIR)/generators/conan_toolchain.cmake \
		-DCMAKE_POLICY_DEFAULT_CMP0091=NEW
	cmake --build $(BUILD_DIR) --config Release
	$(MAKE) compdb

# Debug 版のビルド (msvc-debug プロファイルを使用、build-debug/build に生成)
compile-debug:
	cd .. && PATH="/c/Program Files (x86)/Microsoft Visual Studio/Installer:$$PATH" \
		poetry run conan install . --output-folder=src/build-debug -pr:h=profiles/msvc-debug --build=missing
	cmake -S . -B $(BUILD_DIR_DEBUG) -G "Visual Studio 17 2022" -A x64 -T v143 \
		-DCMAKE_TOOLCHAIN_FILE=$(BUILD_DIR_DEBUG)/generators/conan_toolchain.cmake \
		-DCMAKE_POLICY_DEFAULT_CMP0091=NEW
	cmake --build $(BUILD_DIR_DEBUG) --config Debug

# clangd 用の compile_commands.json を生成
# Visual Studio ジェネレータは CMAKE_EXPORT_COMPILE_COMMANDS 非対応のため Ninja で別途生成する。
# conan_toolchain.cmake は CMAKE_GENERATOR_PLATFORM=x64 を強制設定し Ninja と非互換のため、
# Ninja 向けに generator=Ninja を指定した専用の toolchain を別 output-folder に生成する。
# NOTE: cl.exe が PATH に必要なため、"Developer PowerShell for VS 2022" から実行すること。
compdb:
	cd .. && poetry run conan install . --output-folder=src/build/compiledb-conan \
		-pr:h=profiles/msvc-release --build=missing \
		-c tools.cmake.cmaketoolchain:generator=Ninja
	cmake -E rm -rf $(COMPDB_DIR)
	cmake -S . -B $(COMPDB_DIR) -G Ninja \
		-DCMAKE_TOOLCHAIN_FILE=$(COMPDB_CONAN_DIR)/build/Release/generators/conan_toolchain.cmake \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	cmake -E copy $(COMPDB_DIR)/compile_commands.json ./compile_commands.json
