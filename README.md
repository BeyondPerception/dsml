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
State()
~State()
register_owner()
get()
set()
wait()
wait_for()
last_updated()
```

However, there are only a few core methods that are fundamentally necessary:

```
State()
register_owner()
get()
set()
```

Here is a summary of what the core methods do:

- **State()**

    This is the constructor of a ``dmsl::State`` object. It takes in the path to a configuration file, the name to call the program, and the port on which to listen.
    
    Each line of the configuration file should represent a variable that is available to the program, and it should be formatted as follows:

    `[var_name] [var_type] [owner_program] [is_array]`

    The parameters should be separated by either spaces or tabs. The `[var_name]` parameter should be a string containing no spaces or tabs, representing the name of the variable. The `[var_type]` parameter should be one of the following supported types: `INT8`, `INT16`, `INT32`, `INT64`, `UINT8`, `UINT16`, `UINT32`, `UINT64`, `FLOAT`, `DOUBLE`, `STRING`. The `[owner_program]` parameter should be the name of the program that owns the variable. Finally, the `[is_array]` parameter should be either `true` or `false`, representing whether or not the variable is an array of the aforementioned type. Note that you cannot create an array of arrays (and thereby an array of strings either); if you need to do so, then you should represent your object as an array of bytes.

    The name to call the program must not contain spaces or tabs or else the variables associated with that program will not be available to other programs.

    The port on which to listen will be set to `0` by default if no parameter is given.

    This method returns a `dsml::State` object.

- **register_owner()**

    This method registers the owner of a variable. It takes in the name of the owner program, the IP address of the owner program, and the port of the owner program. This method is required for programs to correctly communicate with each other.

    This method returns `0` on success and `-1` on failure.

- **get()**

    This method gets the data currently stored for a variable. It takes in the name of the variable and requires angle brackets denoting the `c++` type of the variable. This method returns the data of the variable.

    If preferred, you can also pass in an additional return value parameter. In this case, the method will not return anything and instead update the return value parameter to be the data currently stored for the variable; no angle brackets are necessary.

- **set()**

    This method updates a variable with new data. It takes in the name of the variable and the new value of the variable.

    This method does not return anything.

Complete and more detailed descriptions of all of the methods can be found in the header file `dmsl.hpp`.
