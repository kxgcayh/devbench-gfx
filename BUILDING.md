# Building

Uses the macOS cross-build in the toolkit's `docs/skse-cross-compile-macos.md`.

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=<winsdk>/toolchain/clang-cl-msvc.cmake \
  "-DCMAKE_PREFIX_PATH=<CommonLibSSE-NG>/build-install;<vcpkg>/installed/x64-windows-cross"
cmake --build build
clang++ -std=c++20 -Isrc test/path_test.cpp -o /tmp/pt && /tmp/pt   # host self-check
```

The output is `build/DevBenchGFx.dll`. `src/DevBenchAPI.*` are vendored from DevBench v1.32.0 and need no other DevBench source.

Package with `python3 <skyrim-workspace>/tools/mod-release/release.py . package`.
