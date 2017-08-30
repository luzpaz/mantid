#include "MantidAlgorithms/SpecularReflectionCalculateTheta2.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"

#include <cmath>

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;

namespace {
const std::string multiDetectorAnalysis = "MultiDetectorAnalysis";
const std::string lineDetectorAnalysis = "LineDetectorAnalysis";
const std::string pointDetectorAnalysis = "PointDetectorAnalysis";
}

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SpecularReflectionCalculateTheta2)

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string SpecularReflectionCalculateTheta2::name() const {
  return "SpecularReflectionCalculateTheta";
}

/// Algorithm's version for identification. @see Algorithm::version
int SpecularReflectionCalculateTheta2::version() const { return 2; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string SpecularReflectionCalculateTheta2::category() const {
  return "Reflectometry";
}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SpecularReflectionCalculateTheta2::init() {
  declareProperty(
      make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "",
                                                      Direction::Input),
      "An Input workspace to calculate the specular relection theta on.");
  this->initCommonProperties();
  declareProperty(make_unique<PropertyWithValue<double>>(
                      "TwoTheta", Mantid::EMPTY_DBL(), Direction::Output),
                  "Calculated two theta scattering angle in degrees.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SpecularReflectionCalculateTheta2::exec() {

  const double twoTheta = calculateTwoTheta();

  std::stringstream strstream;
  strstream << "Recalculated two theta as: " << twoTheta;

  this->g_log.information(strstream.str());

  this->setProperty("TwoTheta", twoTheta);
}

} // namespace Algorithms
} // namespace Mantid
