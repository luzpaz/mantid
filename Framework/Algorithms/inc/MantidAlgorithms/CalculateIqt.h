#ifndef MANTID_ALGORITHMS_CALCULATEIQT_H_
#define MANTID_ALGORITHMS_CALCULATEIQT_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {
/** Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
    National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and / or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at : <https://github.com/mantidproject/mantid>
    Code Documentation is available at : <http://doxygen.mantidproject.org>
*/
class DLLExport CalculateIqt : public API::Algorithm {
public:

  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override;
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
  std::map<std::string, std::string> validateInputs() override;
  std::string rebinParamsAsString();
  API::MatrixWorkspace_sptr calculateIntermediateWorkspace(API::MatrixWorkspace_sptr workspace, std::string rebinParams);
  MatrixWorkspace_sptr calculateIqt(MatrixWorkspace_sptr workspace, MatrixWorkspace_sptr resolutionWorkspace, std::string rebinParams)

};

} // namespace Algorithms
} // namespace Mantid
#endif // MANTID_ALGORITHMS_MONTECARLOABSORPTION_H_