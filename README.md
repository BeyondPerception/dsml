# DSML: Distributed State Management Library

## Build Information

This project uses CMake to generate build files. All you need is `cmake`, `make` (or your build system of choice), `opencv2` (for the demo), and a working `c++` compiler.

Execute the following commands to compile the system:

```
mkdir build
cd build
cmake ../
make dsml         # To compile DSML
make test         # To compile the unit tests
make video_demo   # To compile the video demo
make process_demo # To compile the process demo
```

Execute the following commands to run the files:

```
./test         # To run the unit tests
./video_demo   # To run the video demo
./process_demo # To run the process demo
```
