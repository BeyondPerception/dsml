# DSML: Distributed State Management Library

## Build Information

This project uses CMake to generate build files. All you need is `cmake`, `make` (or your build system of choice), and a working `c++` compiler. You will also need `opencv2` and the `apriltag` library in order to build the video and process demos, respectively.

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

## Demo Information

After compiling the system, execute `./video_demo` and `./process_demo` to run the video and process demos, respectively. Press any key to start the  process demo, and then press any key to start the video demo. A video feed should appear and highlight any visible AprilTags with their appropriate locations and orientations.

When running across multiple computers, make sure to update the IP addresses in `process.cpp` and `video.cpp` accordingly. This is required to ensure that the correct variable owners are registered.

## API Information

The following methods are available to a `dsml::State` instance:

```
State(config, program_name, port = 0)
register_owner(variable_owner, owner_ip, owner_port)
get(var)
get(var, ret_value)
set(var, value)
wait(var)
wait_for(var, rel_time)
last_updated(var)
```
