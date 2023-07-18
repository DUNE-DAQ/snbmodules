cmake_minimum_required(VERSION 3.12)
project(snbmodules VERSION 0.1.0)

find_package(daq-cmake REQUIRED)

daq_setup_environment()

# ers: for exceptions and normal output
# TRACE: for low-level debugging statements
# Boost unit_test_framework: for the unit tests
# Boost program_options: to allow command line options with descriptions/default values

find_package(ers REQUIRED)     
find_package(TRACE REQUIRED)
find_package(Boost COMPONENTS unit_test_framework program_options REQUIRED)
find_package(Boost COMPONENTS iostreams REQUIRED)
find_package(iomanager REQUIRED)
find_package(appfwk REQUIRED)
find_package(logging REQUIRED)
find_package(hdf5libs REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(HighFive REQUIRED)
find_package(daqdataformats REQUIRED)
find_package(detdataformats REQUIRED)
find_package(readoutlibs REQUIRED)

set(linked_libraries
    iomanager::iomanager
    logging::logging
    ers::ers
    appfwk::appfwk
    hdf5libs::hdf5libs
    daqdataformats::daqdataformats
    detdataformats::detdataformats
    nlohmann_json::nlohmann_json
    HighFive
    readoutlibs::readoutlibs
    Boost::iostreams
)

option(WITH_LIBTORRENT_AS_PACKAGE "Libtorrent externals as a dunedaq package" OFF)
# set(LIBTORRENT_INC ${LIBTORRENT_INC})
# set(LIBTORRENT_LIB ${LIBTORRENT_LIB})

if(WITH_LIBTORRENT_AS_PACKAGE)
  set(LIBTORRENT_DEPENDENCIES 
  )
else()
  set(LIBTORRENT_DEPENDENCIES
    $ENV{LIBTORRENT_LIB}/libtorrent-rasterbar.so
  )
endif()

option(WITH_RCLIB_AS_PACKAGE "RClone externals as a dunedaq package" OFF)
# RCLIB_INC = ${RCLIB_INC}
# RCLIB_LIB = ${RCLIB_LIB}

if(WITH_RCLIB_AS_PACKAGE)
  set(RCLIB_DEPENDENCIES 
  )
else()
  set(RCLIB_DEPENDENCIES
    $ENV{RCLIB_LIB}/librclone.so
  )
endif()

set(tests
    create_file_metadata_simple_test
    client_test
    ip_format_test
    file_metadata_test
    transfer_metadata_test
    session_test
    client_available_test
    notification_test
    bittorent_test
    bittorent_resume_test
    filetransform_file_to_file_test
)

set(apps
    bookkeeper_app
    transfer_client_app
)

set(INCLUDE_DIR "include/snbmodules/")

set(sources_client
    transfer_client.cpp
    transfer_session.cpp
)

set(includes_client
    transfer_client.hpp
    transfer_session.hpp
)

set(sources_interface
)

set(includes_interface
	transfer_interface_abstract.hpp
    transfer_interface_bittorrent.hpp
    transfer_interface_SCP.hpp
    transfer_interface_RClone.hpp
)

set(sources_bookkeeper
    bookkeeper.cpp
)

set(includes_bookkeeper
    bookkeeper.hpp
)

set(sources_common
    group_metadata.cpp
    transfer_metadata.cpp
    notification_interface.cpp
    iomanager_wrapper.cpp
    transfer_interface_bittorrent.cpp
)

set(includes_common
    metadata_abstract.hpp
    group_metadata.hpp
    transfer_metadata.hpp
    protocols_enum.hpp
    status_enum.hpp
    ip_format.hpp
    magic_enum.hpp
    natural_sort.hpp
    notification_interface.hpp
    iomanager_wrapper.hpp
    errors_declaration.hpp
)

list(TRANSFORM sources_client PREPEND client/)
list(TRANSFORM includes_client PREPEND client/)
list(TRANSFORM sources_interface PREPEND transfer_interface/)
list(TRANSFORM includes_interface PREPEND transfer_interface/)
list(TRANSFORM sources_bookkeeper PREPEND bookkeeper/)
list(TRANSFORM includes_bookkeeper PREPEND bookkeeper/)
list(TRANSFORM sources_common PREPEND common/)
list(TRANSFORM includes_common PREPEND common/)

##############################################################################
# Note that when the ers library is linked in, it uses a "::" because
# it's in an external package, not a local CMake target. The semantics
# are <namespace>::<shared library / executable>
daq_add_library(${sources_client} ${sources_interface} ${sources_bookkeeper} ${sources_common} LINK_LIBRARIES ${LIBTORRENT_DEPENDENCIES} ${RCLIB_DEPENDENCIES} ${linked_libraries})

if(WITH_LIBTORRENT_AS_PACKAGE)
  # All good
else()
  # Hack target
  target_include_directories(snbmodules PUBLIC $ENV{LIBTORRENT_INC})

endif()
if(WITH_RCLIB_AS_PACKAGE)
  # All good
else()
  # Hack target
  target_include_directories(snbmodules PUBLIC $ENV{RCLIB_INC})

endif()
##############################################################################
# Plugins

daq_add_plugin(SNBFileTransfer duneDAQModule LINK_LIBRARIES ${Boost_PROGRAM_OPTIONS_LIBRARY} snbmodules)
daq_add_plugin(SNBTransferBookkeeper duneDAQModule LINK_LIBRARIES ${Boost_PROGRAM_OPTIONS_LIBRARY} snbmodules)

##############################################################################
# Integration tests

# foreach(t IN LISTS tests)
#     daq_add_application( ${t} ${t}.cpp TEST LINK_LIBRARIES ${Boost_PROGRAM_OPTIONS_LIBRARY} snbmodules )
# endforeach()

##############################################################################
# Apps

# foreach(app IN LISTS apps)
#     daq_add_application( ${app} ${app}.cpp LINK_LIBRARIES ${Boost_PROGRAM_OPTIONS_LIBRARY} snbmodules )
# endforeach()

##############################################################################
# daq_add_unit_test(ValueWrapper_test)

##############################################################################

# Could add "toylibrary_test_program" to the argument list here, but
# presumably end users wouldn't care about an integration test program
# and just want the shared object library installed

daq_install()