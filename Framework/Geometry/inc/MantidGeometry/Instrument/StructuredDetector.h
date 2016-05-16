#ifndef STRUCTUREDDETECTOR_H
#define STRUCTUREDDETECTOR_H

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Objects/Object.h"
#include <string>
#include <vector>

namespace Mantid {
namespace Geometry {
/**
*  StructuredDetector is a type of CompAssembly, an assembly of components.
*  It is designed to be a more efficient way of defining instruments which
*  are topologically regular but geometrically irregular.
*
* @class StructuredDetector
* @brief Assembly of Detector objects with topological regularity and
geometrical irregularity
* @author Lamar Moore, ISIS
* @date 07-March-2016

Copyright &copy; 2016-17 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_GEOMETRY_DLL StructuredDetector : public CompAssembly,
                                               public IObjComponent {
public:
  /// String description of the type of component
  std::string type() const override { return "StructuredDetector"; }

  /// Matches name to Structured Detector
  static bool compareName(const std::string &proposedMatch);

  //! Empty constructor
  StructuredDetector();

  //! Constructor with a name and parent reference
  StructuredDetector(const std::string &name, IComponent *reference = nullptr);

  //! Parametrized constructor
  StructuredDetector(const StructuredDetector *base, const ParameterMap *map);

  /// Create all the detector pixels of this rectangular detector.
  void initialize(size_t xPixels, size_t yPixels, const std::vector<double> &x,
                  const std::vector<double> &y, bool isZBeam, detid_t idStart,
                  bool idFillByFirstY, int idStepByRow, int idStep = 1);

  //! Make a clone of the present component
  IComponent *clone() const override;

  boost::shared_ptr<Detector> getAtXY(const size_t x, const size_t y) const;

  detid_t getDetectorIDAtXY(const size_t X, const size_t Y) const;
  std::pair<size_t, size_t> getXYForDetectorID(const detid_t detectorID) const;

  std::vector<double> const &getXValues() const;
  std::vector<double> const &getYValues() const;

  void setColors(const std::vector<int> &r, const std::vector<int> &g,
                 const std::vector<int> &b) const;

  std::vector<int> const &getR() const;
  std::vector<int> const &getG() const;
  std::vector<int> const &getB() const;

  size_t xPixels() const;
  size_t yPixels() const;
  detid_t idStart() const;
  bool idFillByFirstY() const;
  int idStepByRow() const;
  int idStep() const;

  /// minimum detector id
  detid_t minDetectorID();
  /// maximum detector id
  detid_t maxDetectorID();

  boost::shared_ptr<const IComponent>
  getComponentByName(const std::string &cname, int nlevels = 0) const override;

  // This should inherit the getBoundingBox implementation from  CompAssembly
  // but
  // the multiple inheritance seems to confuse it so we'll explicityly tell it
  // that here
  using CompAssembly::getBoundingBox;

  // ------------ IObjComponent methods ----------------

  /// Does the point given lie within this object component?
  bool isValid(const Kernel::V3D &point) const override;

  /// Does the point given lie on the surface of this object component?
  bool isOnSide(const Kernel::V3D &point) const override;

  /// Checks whether the track given will pass through this Component.
  int interceptSurface(Track &track) const override;

  /// Finds the approximate solid angle covered by the component when viewed
  /// from the point given
  double solidAngle(const Kernel::V3D &observer) const override;
  /// Retrieve the cached bounding box
  void getBoundingBox(BoundingBox &assemblyBox) const override;

  /// Try to find a point that lies within (or on) the object
  int getPointInObject(Kernel::V3D &point) const override;

  // Rendering member functions
  /// Draws the objcomponent.
  void draw() const override;

  /// Draws the Object.
  void drawObject() const override;

  /// Initializes the ObjComponent for rendering, this function should be called
  /// before rendering.
  void initDraw() const override;

  /// Returns the shape of the Object
  const boost::shared_ptr<const Object> shape() const override;
  /// Returns the material of the detector
  const boost::shared_ptr<const Kernel::Material> material() const override {
    return boost::shared_ptr<const Kernel::Material>();
  }

  // ------------ End of IObjComponent methods ----------------
private:
  /// initialize members to bare defaults
  void init();

  void createDetectors();

  Detector *addDetector(CompAssembly *parent, const std::string &name, size_t x,
                        size_t y, detid_t id);
  /// Pointer to the base RectangularDetector, for parametrized
  /// instruments
  const StructuredDetector *m_base;
  /// Private copy assignment operator
  StructuredDetector &operator=(const ICompAssembly &);

  /// The number of pixels in the X (horizontal) direction;
  size_t m_xPixels;
  /// The number of pixels in the Y (vertical) direction;
  size_t m_yPixels;

  /// minimum detector id
  detid_t m_minDetId;
  /// maximum detector id
  detid_t m_maxDetId;

  /// IDs start here
  detid_t m_idStart;
  /// IDs are filled in Y fastest
  bool m_idFillByFirstY;
  /// Step size in ID in each row
  int m_idStepByRow;
  /// Step size in ID in each col
  int m_idStep;

  std::vector<double> m_xvalues;
  std::vector<double> m_yvalues;

  mutable std::vector<int> r;
  mutable std::vector<int> g;
  mutable std::vector<int> b;
};

MANTID_GEOMETRY_DLL std::ostream &operator<<(std::ostream &,
                                             const StructuredDetector &);

typedef boost::shared_ptr<StructuredDetector> StructuredDetector_sptr;
typedef boost::shared_ptr<const StructuredDetector>
    StructuredDetector_const_sptr;
} // namespace Geometry
} // namespace Mantid
#endif // STRUCTUREDDETECTOR_H