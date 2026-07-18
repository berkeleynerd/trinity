# Repository Guidelines

## Project Structure & Module Organization

`trinityal/` contains the low-level rendering abstraction and Metal/DX backends. `trinity/` contains the higher-level EVE scene, resource, lighting, and render-driver code. Executable demonstrations live in `samples/` (`hello_triangle`, `animated_model`, and `eve_scene_probe`), while import and conversion utilities live in `tools/`. Shader compiler code and Python tests are under `shadercompiler/`. Keep generated files in `.cmake-build-*`; third-party source belongs in the pinned `vendor/` submodules. Architecture notes and macOS status are tracked in `docs/`.

## Build, Test, and Development Commands

Initialize dependencies once:

```sh
git submodule update --init --recursive
./vendor/github.com/microsoft/vcpkg/bootstrap-vcpkg.sh
```

Configure the supported Apple Silicon development build:

```sh
CMAKE_GENERATOR="Ninja Multi-Config" cmake --preset arm64-osx-debug \
  -DBUILD_METAL=ON -DBUILD_SHADER_COMPILER=ON \
  -DBUILD_TESTING=OFF -DWITH_GRANNY=OFF
```

Build everything with `cmake --build .cmake-build-arm64-osx-debug --config Debug --target all`. For quicker iteration, build a focused target such as `TrinityALEveSceneProbe_metal`, then run its `run_TrinityALEveSceneProbe_metal` target. See `docs/macos-metal-bringup.md` for current sample commands and required feature flags.

## Coding Style & Naming Conventions

Format C, C++, and Objective-C++ with the root `.clang-format`. Use four-column tab indentation, Allman braces, and attached pointer stars (`Type* value`). Preserve existing include ordering. Follow established names: PascalCase for types and public methods, `m_` for members, and uppercase snake case for compile-time constants. Keep changes scoped; avoid broad modernization in legacy modules.

## Testing Guidelines

TrinityAL uses GoogleTest sources in `trinityal/tests/`; shader compiler tests use pytest in `shadercompiler/python/shadercompiler/test/`. The TrinityAL GPU conformance suite is wired as a macOS ctest target `TrinityALTest_metal` (configure with `-DBUILD_TESTING=ON`; run `trinityal/tests/run_metal_suite.sh` or `ctest --test-dir <build> -R TrinityALTest_metal`). Runtime scope trims live in one place — the `_METAL_GTEST_EXCLUDES` ledger in `trinityal/tests/CMakeLists.txt`, each entry justified; see `docs/macos-metal-bringup.md` for the current baseline and known backend gaps. Apple sample work may still use finite-frame smoke runs for engine-level changes. Test the smallest affected target, run relevant `--frames N` captures, then build `all`. Treat warnings as failures; presets set `CMAKE_COMPILE_WARNING_AS_ERROR=ON`.

## Commit & Pull Request Guidelines

Recent commits use concise imperative subjects, for example `Add an open glTF-to-CMF model pipeline`. Keep each commit coherent and explain behavioral or asset-pipeline decisions in its body. Pull requests should summarize scope, list build/test commands, link issues when applicable, and include screenshots or captures for rendering changes.

## Asset and Configuration Safety

Do not commit EVE client payloads. Resolve assets from the local SharedCache and stage them only in ignored build directories; commit manifests, logical resource paths, and tooling instead. Keep `WITH_GRANNY=OFF` for the public macOS path unless a change explicitly targets licensed Granny integration.
