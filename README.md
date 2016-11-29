# mesh_param
Mesh parameterization using libigl

## Compile

Compile this project using the standard cmake routine:

    mkdir build
    cd build
    cmake ..
    make

This should find and build the dependencies and create a `mesh_param` binary.

## Run

From within the `build` directory just issue:

    ./mesh_param ../models/piggy.obj

A glfw app should launch displaying the piggy with its parameterization..

## Dependencies

The only dependencies are stl, eigen, [libigl](libigl.github.io/libigl/) and
the dependencies of the `igl::viewer::Viewer` (mandatory: glfw and
opengl, optional: nanogui and nanovg).

If you clone the repository recursively, this should be taken care of:

    git clone --recursive https://github.com/xionluhnis/mesh_param.git

## Generating parameterization

    ./mesh_param ../models/piggy.obj all

will creates the following files in `models`:
 * `piggy.obj-arap.obj` - the as-rigid-as-possible parameterization

## License

See [libigl](libigl.github.io/libigl/) for its own license.
