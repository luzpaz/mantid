//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/FuncMinimizers/BFGS_Minimizer.h"

#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/FuncMinimizerFactory.h"

#include "MantidKernel/Logger.h"

namespace Mantid {
namespace CurveFitting {
namespace FuncMinimisers {
namespace {
/// static logger object
Kernel::Logger g_log("BFGS_Minimizer");
} // namespace

DECLARE_FUNCMINIMIZER(BFGS_Minimizer, BFGS)

/// Return a concrete type to initialize m_gslSolver
/// gsl_multimin_fdfminimizer_vector_bfgs2
const gsl_multimin_fdfminimizer_type *BFGS_Minimizer::getGSLMinimizerType() {
  return gsl_multimin_fdfminimizer_vector_bfgs2;
}

} // namespace FuncMinimisers
} // namespace CurveFitting
} // namespace Mantid
