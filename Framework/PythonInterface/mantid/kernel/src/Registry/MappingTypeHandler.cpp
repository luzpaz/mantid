#include "MantidPythonInterface/kernel/Registry/MappingTypeHandler.h"
#include "MantidPythonInterface/kernel/Registry/PropertyWithValueFactory.h"

#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/PropertyManagerProperty.h"
#include "MantidKernel/PropertyWithValue.h"

#include <boost/make_shared.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/extract.hpp>

using boost::python::dict;
using boost::python::extract;
using boost::python::len;
using boost::python::object;

namespace Mantid {
using Kernel::Direction;
using Kernel::PropertyManager;
using Kernel::PropertyWithValue;
namespace PythonInterface {
namespace Registry {

/**
 * Checks if the boost python input object is of the correct type. In this case
 * a dict.
 * @param obj: the object to check
 * @retruns true if the input is a dict type else false
 */
const bool
MappingTypeHandler::isCorrectType(const boost::python::api::object &obj) {
  return PyObject_TypeCheck(obj.ptr(), &PyDict_Type);
}

/**
 * Sets the named property in the PropertyManager by extracting a new
 * PropertyManager from the Python object
 * @param alg A pointer to the PropertyManager containing the named property
 * @param name The name of the property to update
 * @param mapping The new value of the property
 */
void MappingTypeHandler::set(Kernel::IPropertyManager *alg,
                             const std::string &name,
                             const boost::python::api::object &mapping) const {
  if (!PyObject_TypeCheck(mapping.ptr(), &PyDict_Type)) {
    throw std::invalid_argument("Property " + name + " expects a dictionary");
  }
  auto cppvalue = boost::make_shared<PropertyManager>();
  dict pydict(mapping);
  object iterkeys(pydict.iterkeys()), itervalues(pydict.itervalues());
  auto length = len(pydict);
  for (ssize_t i = 0; i < length; ++i) {
    const auto pykey = iterkeys.attr("next")();
    const auto pyvalue = itervalues.attr("next")();
    const std::string cppkey = extract<std::string>(pykey)();
    cppvalue->declareProperty(
        PropertyWithValueFactory::create(cppkey, pyvalue, Direction::Input));
  }
  alg->setProperty(name, cppvalue);
}

/**
 * Always throws a runtime_error. Use the PropertyManagerProperty directly
 * @param name The name of the property
 * @param defaultValue A default value for the property.
 * @param validator A python object pointing to a validator instance, which
 * can be None.
 * @param direction The direction of the property
 * @returns A pointer to a newly constructed property instance
 */
std::unique_ptr<Kernel::Property> MappingTypeHandler::create(
    const std::string &name, const boost::python::api::object &defaultValue,
    const boost::python::api::object &, const unsigned int direction) const {
  // We follow the same steps as above. Create a property manager and populate
  // it
  // with some sub-values
  auto cppvalue = boost::make_shared<Mantid::Kernel::PropertyManager>();
  dict pydict(defaultValue);
  object iterkeys(pydict.iterkeys()), itervalues(pydict.itervalues());
  auto length = len(pydict);
  for (ssize_t i = 0; i < length; ++i) {
    const auto pykey = iterkeys.attr("next")();
    const auto pyvalue = itervalues.attr("next")();
    const std::string cppkey = extract<std::string>(pykey)();
    cppvalue->declareProperty(
        PropertyWithValueFactory::create(cppkey, pyvalue, direction));
  }
  // Wrap the property manager in a PropertyManagerProperty instance.
  std::unique_ptr<Kernel::Property> valueProp =
      Kernel::make_unique<Kernel::PropertyManagerProperty>(name, cppvalue,
                                                           direction);
  return valueProp;
}
}
}
}
