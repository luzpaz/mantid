/*  A system-wide file to contain, e.g., useful system-dependent macros

    @author Russell Taylor, Tessella Support Services plc
    @date 26/10/2007
    
    Copyright &copy; 2007 ???RAL???

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
*/
#ifndef MANTID_SYSTEM_H_
#define MANTID_SYSTEM_H_

#ifdef _WIN32
  #pragma warning( disable: 4251 )
  #define DLLExport __declspec( dllexport )
#else
  #define DLLExport
#endif

namespace Mantid {

  /** This class is simply used in the subscription of classes into the various
   *  factories in Mantid. The fact that the constructor takes an int means that
   *  the comma operator can be used to make a call to the factories' subscribe
   *  method in the first part.
   */
  class RegistrationHelper
  {
  public:
    /// Constructor. Does nothing.
    /// @param Takes an int
    RegistrationHelper(int) 
    { 
      // Does nothing 
    }
  };

} // namespace Mantid

#endif /*MANTID_SYSTEM_H_*/
