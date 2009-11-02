#ifndef MANTID_CURVEFITTING_LINEARBACKGROUND_H_
#define MANTID_CURVEFITTING_LINEARBACKGROUND_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFunction.h"

namespace Mantid
{
  namespace CurveFitting
  {
    /**
    Provide linear function interface to IFunction.
    I.e. the function: A0+A1*x.

    LinearBackground parameters:
    <UL>
    <LI> A0 - coefficient for constant term (default 0.0)</LI>
    <LI> A1 - coefficient for linear term (default 0.0)</LI>
    </UL>

    @author Anders Markvardsen, ISIS, RAL
    @date 23/10/2009

    Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class DLLExport LinearBackground : public API::IFunction
    {
    public:
      /// Destructor
      virtual ~LinearBackground() {};

      /// overwrite IFunction base class methods
      virtual void init();
      virtual void function(double* out, const double* xValues, const int& nData);
      virtual void functionDeriv(API::Jacobian* out, const double* xValues, const int& nData);


    };

  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_LINEARBACKGROUND_H_*/
