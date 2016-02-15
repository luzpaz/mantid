#ifndef MANTID_ALGORITHMS_MONITOREFFICIENCYCORUSER_H_
#define MANTID_ALGORITHMS_MONITOREFFICIENCYCORUSER_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {

class DLLExport MonitorEfficiencyCorUser : public API::Algorithm {
public:
  /// (Empty) Constructor
  MonitorEfficiencyCorUser(); // : Mantid::API::Algorithm() {}
  /// Virtual destructor
  ~MonitorEfficiencyCorUser() override;
  /// Algorithm's name
  const std::string name() const override { return "MonitorEfficiencyCorUser"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "This algorithm normalises the counts by the monitor counts with "
           "additional efficiency correction according to the formula set in "
           "the instrument parameters file.";
  }

  /// Algorithm's version
  int version() const override { return 1; }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "CorrectionFunctions\\NormalisationCorrections";
  }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  double calculateFormulaValue(const std::string &, double);
  std::string getValFromInstrumentDef(const std::string &);
  void applyMonEfficiency(const size_t numberOfChannels, const MantidVec &yIn,
                          const MantidVec &eIn, const double coeff,
                          MantidVec &yOut, MantidVec &eOut);

  /// The user selected (input) workspace
  API::MatrixWorkspace_const_sptr m_inputWS;
  /// The output workspace, maybe the same as the input one
  API::MatrixWorkspace_sptr m_outputWS;
  /// stores the incident energy of the neutrons
  double m_Ei;
  /// stores the total count of neutrons from the monitor
  int m_monitorCounts;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MONITOREFFICIENCYCORUSER_H_*/
