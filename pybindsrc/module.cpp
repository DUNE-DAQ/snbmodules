/**
 * @file module.cpp
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "registrators.hpp"

#include "pybind11/pybind11.h"
#include "pybind11/stl.h"

namespace py = pybind11;

namespace dunedaq::snbmodules::python {

PYBIND11_MODULE(_daq_snbmodules_py, m)
{

  m.doc() = "C++ implementation of the snbmodules modules";

  // You'd want to change renameme to the name of a function which
  // you'd like to have a python binding to

  register_renameme(m);
}

} // namespace dunedaq::snbmodules::python
