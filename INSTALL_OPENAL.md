# Installing OpenAL for Phase 7

## Ubuntu/Debian

```bash
sudo apt-get update
sudo apt-get install -y libopenal-dev libsndfile1-dev
```

## After Installation

Reconfigure CMake to detect OpenAL:
```bash
cd build
cmake ..
cmake --build .
```

## Verification

Check if OpenAL is installed:
```bash
pkg-config --modversion openal
```

Should output a version number like `1.19.1` or similar.

## Note

Phase 7 audio system files have been created but will be marked as optional until OpenAL is installed. The engine will build without audio support if OpenAL is not found.
