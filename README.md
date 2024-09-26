# verlet

Simple physics approximation using [Verlet Integration](https://en.wikipedia.org/wiki/Verlet_integration) approach.

Inspired by [this video](https://www.youtube.com/watch?v=lS_qeBy3aQI). Almost exactly the same but I wanted to play with it myself :).

You can see the demo here: https://www.youtube.com/watch?v=vgMczxau7VM and here https://www.youtube.com/watch?v=vgMczxau7VM.

It is also possible to generate video that results into some predefined image (verlet_video project): https://youtu.be/NFWb60gZgKY

# Building

## OpenCV

All dependencies except OpenCV will be cloned by python script. But OpenCV was too big to include it to my project so I add it as system dependency. You can simply install dev package with your package manager and cmake should me able to find it. On Windows I prefer to build it myself and here is the python script if you are will to do so (simply replace paths to ones you like):

<details>
  <summary>Build OpenCV on Windows</summary>

```python
import subprocess
from pathlib import Path


def main():
    opencv_dir = Path("C:/data/soft/portable/opencv")
    repo_url = "https://github.com/opencv/opencv"
    repo_tag = "4.10.0"
    repo_dir = opencv_dir / "repo"
    build_dir = opencv_dir / "build"
    install_dir = opencv_dir / "install"

    # Clone
    if not repo_dir.exists():
        subprocess.check_call(
            args=[
                "git",
                "clone",
                "--depth",
                "1",
                "--branch",
                repo_tag,
                "--single-branch",
                repo_url,
                repo_dir.as_posix(),
            ]
        )

    # Generate project file
    subprocess.check_call(
        args=[
            "cmake",
            *("-G", "Visual Studio 17 2022", "-A", "x64"),
            *("-S", repo_dir.as_posix()),
            *("-B", build_dir.as_posix()),
            "-DBUILD_PERF_TESTS:BOOL=OFF",
            "-DBUILD_TESTS:BOOL=OFF",
            "-DBUILD_DOCS:BOOL=OFF",
            "-DWITH_CUDA:BOOL=OFF",
            "-DBUILD_EXAMPLES:BOOL=OFF",
            "-DINSTALL_CREATE_DISTRIB:BOOL=ON",
            "-DWITH_FFMPEG:BOOL=ON",
            "-DBUILD_opencv_java:BOOL=OFF",
            "-DBUILD_opencv_python3:BOOL=OFF",
            "-DBUILD_WITH_STATIC_CRT=OFF", # <- Change this if you have CRT mismatch link error
            "-DBUILD_SHARED_LIBS:BOOL=OFF",  # <- opencv libs will be static
            f"-DCMAKE_INSTALL_PREFIX={install_dir.as_posix()}",
        ]
    )

    def build_config(config: str):
        subprocess.check_call(
            args=[
                "cmake",
                *("--build", build_dir.as_posix()),
                *("--target", "install"),
                *("--config", config),
            ]
        )

    build_config("debug")
    build_config("release")

    print(f"install directory: {install_dir.as_posix()}")


if __name__ == "__main__":
    main()
```
</details>

## Clone/Generate/Build

```bash
git clone https://github.com/Sunday111/verlet
cd verlet
git submodule update --init
python ./deps/yae/scripts/make_project_files.py --project_dir=. # Get dependencies and generate CMake files
# You can skip `-DOpenCV_DIR` if OpenCV is discoverable on system level.
cmake -S . -B ./build -DOpenCV_DIR="C:/data/soft/portable/opencv/install" -DCMAKE_BUILD_TYPE=Release
cmake --build ./build
```

# Compress the resulting video

Use ffpeg to compress the video because it tends to be way to big. I prefer using av1 (on Windows) because it super fast and works really well with video that have a lot of particles.

```Powershell
./ffmpeg -i output.mp4 -c:v av1_nvenc -b:v 4M output_av1.mp4
```
