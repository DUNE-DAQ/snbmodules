# SNBmodules
Welcome to the snbmodules wiki!

This library uses external libraries such as Bittorent or Rclone that need to be intalled.

1. Clone this repo into the sourcecode/ directory and add the library name to the dbt-build-order.cmake

2. Source the environment and install external libraries

3. Add to env.sh file the export of the libraries and source again

4. Build and Configure the test environment

5. Start new transfers and enjoy~

# TODO

- Have default values for parameters (for now every parameters are mandatory)
- See important parameters for RClone
- Script for configuration generation
- Auto start RClone HTTP server on source client (Uploader)
    - For now, we can start the server with rclone serve, see [Install local server section](#run-local-server)

# Wiki

Please follow the wiki pages for detailed instructions and informations about:
- [Installation](https://github.com/DUNE-DAQ/snbmodules/wiki/Installation)
- [Configuration](https://github.com/DUNE-DAQ/snbmodules/wiki/Configuration)
- [Transfers](https://github.com/DUNE-DAQ/snbmodules/wiki/Transfers)
- [Technical Documentation](https://github.com/DUNE-DAQ/snbmodules/wiki/Technical-Documentation)
