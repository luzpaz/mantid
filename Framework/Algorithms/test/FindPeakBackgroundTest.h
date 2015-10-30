#ifndef MANTID_ALGORITHMS_FindPeakBackground_H_
#define MANTID_ALGORITHMS_FindPeakBackgroundTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/FindPeakBackground.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

#include <cmath>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

using namespace std;

using Mantid::Algorithms::FindPeakBackground;

class FindPeakBackgroundTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FindPeakBackgroundTest *createSuite() {
    return new FindPeakBackgroundTest();
  }
  static void destroySuite(FindPeakBackgroundTest *suite) { delete suite; }

  void test_Calculation() {
    // 1. Generate input workspace
    MatrixWorkspace_sptr inWS = generateTestWorkspace();

    // 2. Create
    Algorithms::FindPeakBackground alg;

    alg.initialize();
    TS_ASSERT(alg.isInitialized());

    alg.setProperty("InputWorkspace", inWS);
    alg.setProperty("OutputWorkspace", "Signal");
    alg.setProperty("WorkspaceIndex", 0);

    alg.execute();
    TS_ASSERT(alg.isExecuted());

    Mantid::API::ITableWorkspace_sptr peaklist =
        boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve("Signal"));

    TS_ASSERT(peaklist);
    TS_ASSERT_EQUALS(peaklist->rowCount(), 1);
    TS_ASSERT_DELTA(peaklist->Int(0, 1), 4, 0.01);
    TS_ASSERT_DELTA(peaklist->Int(0, 2), 19, 0.01);
    TS_ASSERT_DELTA(peaklist->Double(0, 3), 1.2, 0.01);
    TS_ASSERT_DELTA(peaklist->Double(0, 4), 0.04, 0.01);
    TS_ASSERT_DELTA(peaklist->Double(0, 5), 0.0, 0.01);

    // Clean
    AnalysisDataService::Instance().remove("Signal");

    return;
  }

  /** Generate a workspace for test
   */
  MatrixWorkspace_sptr generateTestWorkspace() {
    vector<double> data;
    data.push_back(1);
    data.push_back(2);
    data.push_back(1);
    data.push_back(1);
    data.push_back(9);
    data.push_back(11);
    data.push_back(13);
    data.push_back(20);
    data.push_back(24);
    data.push_back(32);
    data.push_back(28);
    data.push_back(48);
    data.push_back(42);
    data.push_back(77);
    data.push_back(67);
    data.push_back(33);
    data.push_back(27);
    data.push_back(20);
    data.push_back(9);
    data.push_back(2);

    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        WorkspaceFactory::Instance().create("Workspace2D", 1, data.size(),
                                            data.size()));

    MantidVec &vecX = ws->dataX(0);
    MantidVec &vecY = ws->dataY(0);
    MantidVec &vecE = ws->dataE(0);

    for (size_t i = 0; i < data.size(); ++i) {
      vecX[i] = static_cast<double>(i);
      vecY[i] = data[i];
      vecE[i] = sqrt(data[i]);
    }

    return ws;
  }

  //--------------------------------------------------------------------------------------------
  /** Test on a spectrum without peak
    */
  void test_FindBackgroundOnFlat() {
    // Add workspace
    MatrixWorkspace_sptr testws = generate2SpectraTestWorkspace();
    AnalysisDataService::Instance().addOrReplace("Test2Workspace", testws);

    // Set up algorithm
    Algorithms::FindPeakBackground alg;

    alg.initialize();
    TS_ASSERT(alg.isInitialized());

    alg.setProperty("InputWorkspace", "Test2Workspace");
    alg.setProperty("OutputWorkspace", "Signal3");
    alg.setProperty("WorkspaceIndex", 0);

    // Execute
    alg.execute();
    TS_ASSERT(alg.isExecuted());

    // Check result
    ITableWorkspace_sptr outws = boost::dynamic_pointer_cast<ITableWorkspace>(
        AnalysisDataService::Instance().retrieve("Signal3"));
    TS_ASSERT(outws);
    if (!outws)
      return;

    TS_ASSERT_EQUALS(outws->rowCount(), 1);
    if (outws->rowCount() < 1)
      return;

    int ipeakmin = outws->Int(0, 1);
    int ipeakmax = outws->Int(0, 2);
    TS_ASSERT(ipeakmin >= ipeakmax);

    // Clean
    AnalysisDataService::Instance().remove("Signal3");
    AnalysisDataService::Instance().remove("Test2Workspace");

    return;
  }

  //--------------------------------------------------------------------------------------------
  /** Test on a spectrum without peak
    */
  void test_FindBackgroundOnSpec1() {
    // Add workspace
    MatrixWorkspace_sptr testws = generate2SpectraTestWorkspace();
    AnalysisDataService::Instance().addOrReplace("Test2Workspace", testws);

    // Set up algorithm
    Algorithms::FindPeakBackground alg;

    alg.initialize();
    TS_ASSERT(alg.isInitialized());

    alg.setProperty("InputWorkspace", "Test2Workspace");
    alg.setProperty("OutputWorkspace", "Signal2");
    alg.setProperty("WorkspaceIndex", 1);

    // Execute
    alg.execute();
    TS_ASSERT(alg.isExecuted());

    // Check result
    ITableWorkspace_sptr outws = boost::dynamic_pointer_cast<ITableWorkspace>(
        AnalysisDataService::Instance().retrieve("Signal2"));
    TS_ASSERT(outws);
    if (!outws)
      return;

    TS_ASSERT_EQUALS(outws->rowCount(), 1);
    if (outws->rowCount() < 1)
      return;

    TS_ASSERT_EQUALS(outws->rowCount(), 1);
    TS_ASSERT_DELTA(outws->Int(0, 1), 4, 0.01);
    TS_ASSERT_DELTA(outws->Int(0, 2), 19, 0.01);
    TS_ASSERT_DELTA(outws->Double(0, 3), 1.2, 0.01);
    TS_ASSERT_DELTA(outws->Double(0, 4), 0.04, 0.01);
    TS_ASSERT_DELTA(outws->Double(0, 5), 0.0, 0.01);

    // Clean
    AnalysisDataService::Instance().remove("Signal2");
    AnalysisDataService::Instance().remove("Test2Workspace");

    return;
  }

  //--------------------------------------------------------------------------------------------
  /** Generate a workspace with 2 spectra for test
   */
  MatrixWorkspace_sptr generate2SpectraTestWorkspace() {
    vector<double> data;
    data.push_back(1);
    data.push_back(2);
    data.push_back(1);
    data.push_back(1);
    data.push_back(9);
    data.push_back(11);
    data.push_back(13);
    data.push_back(20);
    data.push_back(24);
    data.push_back(32);
    data.push_back(28);
    data.push_back(48);
    data.push_back(42);
    data.push_back(77);
    data.push_back(67);
    data.push_back(33);
    data.push_back(27);
    data.push_back(20);
    data.push_back(9);
    data.push_back(2);

    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        WorkspaceFactory::Instance().create("Workspace2D", 2, data.size(),
                                            data.size()));

    // Workspace index = 0
    MantidVec &vecX = ws->dataX(0);
    MantidVec &vecY = ws->dataY(0);
    MantidVec &vecE = ws->dataE(0);
    for (size_t i = 0; i < data.size(); ++i) {
      vecX[i] = static_cast<double>(i);
      vecY[i] = 0.0;
      vecE[i] = 1.0;
    }

    // Workspace index = 1
    MantidVec &vecX1 = ws->dataX(1);
    MantidVec &vecY1 = ws->dataY(1);
    MantidVec &vecE1 = ws->dataE(1);
    for (size_t i = 0; i < data.size(); ++i) {
      vecX1[i] = static_cast<double>(i);
      vecY1[i] = data[i];
      vecE1[i] = sqrt(data[i]);
    }

    return ws;
  }
};

#endif /* MANTID_ALGORITHMS_FindPeakBackgroundTEST_H_ */