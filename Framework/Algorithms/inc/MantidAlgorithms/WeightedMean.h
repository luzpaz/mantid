#ifndef MANTID_ALGORITHMS_WEIGHTEDMEAN_H_
#define MANTID_ALGORITHMS_WEIGHTEDMEAN_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/BinaryOperation.h"

namespace Mantid {
namespace Algorithms {
/** An algorithm to calculate the weighted mean of two workspaces.

    Required Properties:
    <UL>
    <LI> InputWorkspace1 - The name of the first input workspace.</LI>
    <LI> InputWorkspace2 - The name of the second input workspace. </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the
   result.</LI>
    </UL>

    @author Robert Dalgliesh, ISIS, RAL
    @date 12/1/2010

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class DLLExport WeightedMean : public BinaryOperation {
public:
  const std::string name() const override { return "WeightedMean"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "An algorithm to calculate the weighted mean of two workspaces.";
  }

  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"Mean"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Arithmetic"; }

private:
  // Overridden BinaryOperation methods
  void performBinaryOperation(const MantidVec &lhsX, const MantidVec &lhsY,
                              const MantidVec &lhsE, const MantidVec &rhsY,
                              const MantidVec &rhsE, MantidVec &YOut,
                              MantidVec &EOut) override;
  void performBinaryOperation(const MantidVec &lhsX, const MantidVec &lhsY,
                              const MantidVec &lhsE, const double rhsY,
                              const double rhsE, MantidVec &YOut,
                              MantidVec &EOut) override;
  bool
  checkCompatibility(const API::MatrixWorkspace_const_sptr lhs,
                     const API::MatrixWorkspace_const_sptr rhs) const override;
  std::string checkSizeCompatibility(
      const API::MatrixWorkspace_const_sptr lhs,
      const API::MatrixWorkspace_const_sptr rhs) const override;

  /// The name of the first input workspace property for BinaryOperation
  std::string inputPropName1() const override { return "InputWorkspace1"; }
  /// The name of the second input workspace property for BinaryOperation
  std::string inputPropName2() const override { return "InputWorkspace2"; }
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_WEIGHTEDMEAN_H_*/
