#ifndef MANTID_PYTHONINTERFACE_PROPERTYWITHVALUEEXPORTER_H_
#define MANTID_PYTHONINTERFACE_PROPERTYWITHVALUEEXPORTER_H_

/*
    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
   National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

#include "MantidKernel/PropertyWithValue.h"
#include "MantidPythonInterface/kernel/Converters/ContainerDtype.h"

#ifndef Q_MOC_RUN
#include <boost/python/bases.hpp>
#include <boost/python/class.hpp>
#include <boost/python/return_by_value.hpp>
#include <boost/python/return_value_policy.hpp>
#endif

// Call the dtype helper function
template <typename HeldType>
std::string dtype(Mantid::Kernel::PropertyWithValue<HeldType> &self) {
  // Check for the special case of a string
  if (std::is_same<HeldType, std::string>::value) {
    std::stringstream ss;
    std::string val = self.value();
    ss << "S" << val.size();
    std::string ret_val = ss.str();
    return ret_val;
  }

  return Mantid::PythonInterface::Converters::dtype(self);
}

namespace Mantid {
namespace PythonInterface {

/**
 * A helper struct to export PropertyWithValue<> types to Python.
 */
template <typename HeldType,
          typename ValueReturnPolicy = boost::python::return_by_value>
struct PropertyWithValueExporter {
  static void define(const char *pythonClassName) {
    using namespace boost::python;
    using namespace Mantid::Kernel;

    class_<PropertyWithValue<HeldType>, bases<Property>, boost::noncopyable>(
        pythonClassName, no_init)
        .add_property("value",
                      make_function(&PropertyWithValue<HeldType>::operator(),
                                    return_value_policy<ValueReturnPolicy>()))
        .def("dtype", &dtype<HeldType>, arg("self"));
  }
};
} // namespace PythonInterface
} // namespace Mantid

#endif /* MANTID_PYTHONINTERFACE_PROPERTYWITHVALUEEXPORTER_H_ */
