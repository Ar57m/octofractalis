<div align="center">
  <img src="https://github.com/user-attachments/assets/6a681bfb-1f57-4a78-b437-2895dbd4ac5f" height="128" alt="Octo Fractalis Logo"/>



  <h1>Octo Fractalis</h1>
  <p>
    High-dimensional fractal viewer for Complex, Quaternion, and Octonion spaces
  </p>
</div>

---

## ✨ Overview

**Octo Fractalis** is a C++20 fractal viewer designed for exploring Mandelbrot and Julia sets across multiple number systems:

- Complex (2D)
- Quaternion (4D)
- Octonion (8D)

It supports **custom equations with arbitrary operators and functions**, and can run either as an interactive UI or a lightweight CLI.

Tested on:
- Windows 11 (MSVC)
- Linux (Debian / Ubuntu)
- Android (CLI mode on Termux)

---

## 🚀 Features

- **Number Spaces:** Complex, Quaternion, Octonion
- **Fractal Types:** Mandelbrot & Julia (fully customizable equations)
- **Fast CPU Rendering:** OpenMP + SIMD (AVX2)
- **Dual Modes:**
  - UI (SDL3 + Dear ImGui + lodepng + stb_image.h)
  - CLI (fast and minimal + lodepng + stb_image.h)

---

## 🖼️ Preview

### UI (Work in Progress)
<div align="center">
  <img height="220" src="https://github.com/user-attachments/assets/5a01b252-a7f5-42e1-8744-87e41a4dc478" alt="UI preview"/>
</div>

### Fractals (Examples)
<div align="center">

| Mandelbrot | Julia |
|-----------|--------|
| <img src="https://github.com/user-attachments/assets/e95b62f9-6d14-43a9-9ea7-a4dc8d0c4854" height="220"/> | <img src="https://github.com/user-attachments/assets/a47a0065-5d0f-4c57-b82b-9a18229db6c4" height="220"/> |

</div>

---

## 📥 Prebuilt Binaries

Precompiled builds are available in the Releases section:

👉 https://github.com/Ar57m/octofractalis/releases

### Downloads

- **Windows (x64)** → `octofractalis-win-x64.zip`
- **Linux (x64)** → `octofractalis-linux-x64.tar.gz`

> These builds are compiled for 64-bit systems.  
> If they don't run on your machine, try building from source.

---

## 🚀 Quick Start (No Build Required)

1. Download the archive for your platform  
2. Extract it  
3. Run
---

## 📦 Installation

### 1. Clone the Repository

> ⚠️ This project uses submodules (ImGui). You **must** include them if you want to compile the UI mode.

```bash
git clone --recurse-submodules https://github.com/Ar57m/octofractalis
cd octofractalis
````

If you already cloned:

```bash
git submodule update --init --recursive
```

---

## 🔧 Dependencies

### Required

* C++20 compiler

  * GCC / Clang (Linux / Termux)
  * MSVC (2019 tested on Windows)
* OpenMP support

### UI Only

* SDL3

#### Windows

Place the prebuilt SDL3 lib folder inside:

```
libs/
```

#### Linux

The build script will automatically compile SDL3 if not found.

But make sure you have:

```bash
sudo apt install build-essential cmake
```

---

## 🧱 Building

### 🖥️ Windows (MSVC)

Run inside a **Developer Command Prompt**:

#### UI version using double

```bat
build.bat
```

#### CLI version using double

```bat
build.bat --cli
```

---

### 🐧 Linux (Debian / Ubuntu)

```bash
chmod +x build.sh
```

#### UI version

```bash
./build.sh
```

#### CLI version

```bash
./build.sh --cli
```

---

### 📱 Android (Termux)

CLI mode is recommended:

```bash
pkg install build-essential cmake

chmod +x build.sh
./build.sh --cli
```

---

## ⚙️ Build Flags

| Flag       | Description                           |
| ---------- | ------------------------------------- |
| `--cpu`    | CPU mode (default)                    |
| `--cli`    | No UI binary               |
| `--float`  | 32-bit precision (faster, less depth) |
| `--double` | 64-bit precision (default)            |
| `--share`  | Portable binary (no AVX2, runs on older CPUs) |

---

## ▶️ Running

### UI

```bash
./octofractalis      # Linux
octofractalis.exe    # Windows
```

### CLI

```bash
./octofractalis-cli
octofractalis-cli.exe
```

CLI help:

```bash
./octofractalis-cli -h
octofractalis-cli.exe -h
```

---

## 🧠 Notes

* Deep zooms are limited by floating-point precision
* `float` = faster, less stable
* `double` = slower, much deeper zoom
* AVX2 is enabled by default (disable with `--share`)

---

## 💸 Support

If you find this project useful:

```
0x190ac445974a989a87dd223f212a76ca0090c804
```
