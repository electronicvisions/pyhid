#include <genpybind.h>

GENPYBIND_MANUAL({ parent.attr("__variant__") = "pybind11"; })

#include "pyhid/hid_libusb.hpp"
