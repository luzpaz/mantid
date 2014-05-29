#ifndef MANTID_ALGORITHMS_STITCH1DTEST_H_
#define MANTID_ALGORITHMS_STITCH1DTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/Stitch1D.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"
#include <algorithm>
#include <boost/assign/list_of.hpp>
#include <boost/tuple/tuple.hpp>

using namespace Mantid::API;
using namespace boost::assign;
using Mantid::Algorithms::Stitch1D;
using Mantid::MantidVec;

class Stitch1DTest: public CxxTest::TestSuite
{
private:

  template<typename T>
  class LinearSequence
  {
  private:
    const T m_start;
    const T m_step;
    T m_stepNumber;
  public:
    LinearSequence(const T& start, const T& step) :
        m_start(start), m_step(step), m_stepNumber(T(0))
    {
    }

    T operator()()
    {
      T result = m_start + (m_step * m_stepNumber);
      m_stepNumber += 1;
      return result;
    }

  };

  MatrixWorkspace_sptr create1DWorkspace(MantidVec& xData, MantidVec& yData, MantidVec& eData)
  {
    auto createWorkspace = AlgorithmManager::Instance().create("CreateWorkspace");
    createWorkspace->setChild(true);
    createWorkspace->initialize();
    createWorkspace->setProperty("UnitX", "1/q");
    createWorkspace->setProperty("DataX", xData);
    createWorkspace->setProperty("DataY", yData);
    createWorkspace->setProperty("NSpec", 1);
    createWorkspace->setProperty("DataE", eData);
    createWorkspace->setPropertyValue("OutputWorkspace", "dummy");
    createWorkspace->execute();
    MatrixWorkspace_sptr outWS = createWorkspace->getProperty("OutputWorkspace");
    return outWS;
  }

  MatrixWorkspace_sptr create1DWorkspace(MantidVec& xData, MantidVec& yData)
  {
    auto createWorkspace = AlgorithmManager::Instance().create("CreateWorkspace");
    createWorkspace->setChild(true);
    createWorkspace->initialize();
    createWorkspace->setProperty("UnitX", "1/q");
    createWorkspace->setProperty("DataX", xData);
    createWorkspace->setProperty("DataY", yData);
    createWorkspace->setProperty("NSpec", 1);
    createWorkspace->setPropertyValue("OutputWorkspace", "dummy");
    createWorkspace->execute();
    MatrixWorkspace_sptr outWS = createWorkspace->getProperty("OutputWorkspace");
    return outWS;
  }

  MatrixWorkspace_sptr a;
  MatrixWorkspace_sptr b;
  MantidVec x;
  MantidVec e;
  typedef boost::tuple<MatrixWorkspace_sptr, double> ResultType;

  MatrixWorkspace_sptr make_arbitrary_point_ws()
  {
    auto x = MantidVec(3);
    const double xstart = -1;
    const double xstep = 0.2;
    LinearSequence<MantidVec::value_type> sequenceX(xstart, xstep);
    std::generate(x.begin(), x.end(), sequenceX);

    auto y = MantidVec(3);
    const double ystart = 1;
    const double ystep = 1;
    LinearSequence<MantidVec::value_type> sequenceY(ystart, ystep);
    std::generate(y.begin(), y.end(), sequenceY);

    auto e = MantidVec(3, 1);

    return create1DWorkspace(x, y, e);
  }

  MatrixWorkspace_sptr make_arbitrary_histogram_ws()
  {
    auto x = MantidVec(3);
    const double xstart = -1;
    const double xstep = 0.2;
    LinearSequence<MantidVec::value_type> sequenceX(xstart, xstep);
    std::generate(x.begin(), x.end(), sequenceX);

    auto y = MantidVec(2);
    const double ystart = 1;
    const double ystep = 1;
    LinearSequence<MantidVec::value_type> sequenceY(ystart, ystep);
    std::generate(y.begin(), y.end(), sequenceY);

    auto e = MantidVec(2, 1);

    return create1DWorkspace(x, y, e);
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static Stitch1DTest *createSuite()
  {
    return new Stitch1DTest();
  }
  static void destroySuite(Stitch1DTest *suite)
  {
    delete suite;
  }

  Stitch1DTest()
  {
    FrameworkManager::Instance();

    e = MantidVec(10, 0);
    x = MantidVec(11);
    const double xstart = -1;
    const double xstep = 0.2;
    LinearSequence<MantidVec::value_type> sequence(xstart, xstep);
    std::generate(x.begin(), x.end(), sequence);

    MantidVec y = boost::assign::list_of(0)(0)(0)(3)(3)(3)(3)(3)(3)(3).convert_to_container<MantidVec>();

    // Pre-canned workspace to stitch
    a = create1DWorkspace(x, y, e);

    y = boost::assign::list_of(2)(2)(2)(2)(2)(2)(2)(0)(0)(0).convert_to_container<MantidVec>();
    // Another pre-canned workspace to stitch
    b = create1DWorkspace(x, y, e);

  }

  ResultType do_stitch1D(MatrixWorkspace_sptr& lhs, MatrixWorkspace_sptr& rhs,
      const double& startOverlap, const double& endOverlap, const MantidVec& params)
  {
    Stitch1D alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("LHSWorkspace", lhs);
    alg.setProperty("RHSWorkspace", rhs);
    alg.setProperty("StartOverlap", startOverlap);
    alg.setProperty("EndOverlap", endOverlap);
    alg.setProperty("Params", params);
    alg.setPropertyValue("OutputWorkspace", "dummy_value");
    alg.execute();
    MatrixWorkspace_sptr stitched = alg.getProperty("OutputWorkspace");
    double scaleFactor = alg.getProperty("OutScaleFactor");
    return ResultType(stitched, scaleFactor);
  }

  ResultType do_stitch1D(MatrixWorkspace_sptr& lhs, MatrixWorkspace_sptr& rhs, const MantidVec& params)
  {
    Stitch1D alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("LHSWorkspace", lhs);
    alg.setProperty("RHSWorkspace", rhs);
    alg.setProperty("Params", params);
    alg.setPropertyValue("OutputWorkspace", "dummy_value");
    alg.execute();
    MatrixWorkspace_sptr stitched = alg.getProperty("OutputWorkspace");
    double scaleFactor = alg.getProperty("OutScaleFactor");
    return ResultType(stitched, scaleFactor);
  }

  void test_Init()
  {
    Stitch1D alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize())
    TS_ASSERT( alg.isInitialized())
  }

  void test_startoverlap_greater_than_end_overlap_throws()
  {
    TSM_ASSERT_THROWS("Should have thrown with StartOverlap < x max",
        do_stitch1D(this->a, this->b, this->x.back(), this->x.front(), MantidVec(1, 0.2)),
        std::runtime_error&);
  }

  void test_lhsworkspace_must_be_histogram()
  {
    auto lhs_ws = make_arbitrary_point_ws();
    auto rhs_ws = make_arbitrary_histogram_ws();
    TSM_ASSERT_THROWS("LHS WS must be histogram", do_stitch1D(lhs_ws, rhs_ws, -1, 1, MantidVec(1, 0.2)),
        std::invalid_argument&);
  }

  void test_rhsworkspace_must_be_histogram()
  {
    auto lhs_ws = make_arbitrary_histogram_ws();
    auto rhs_ws = make_arbitrary_point_ws();
    TSM_ASSERT_THROWS("RHS WS must be histogram", do_stitch1D(lhs_ws, rhs_ws, -1, 1, MantidVec(1, 0.2)),
        std::invalid_argument&);
  }

  void test_stitching_uses_suppiled_params()
  {
    auto ret = do_stitch1D(this->b, this->a, -0.4, 0.4,
        boost::assign::list_of<double>(-0.8)(0.2)(1.0).convert_to_container<MantidVec>());

    MantidVec xValues = ret.get<0>()->readX(0); // Get the output workspace and look at the limits.

    double xMin = xValues.front();
    double xMax = xValues.back();

    TS_ASSERT_EQUALS(xMin, -0.8);
    TS_ASSERT_EQUALS(xMax, 1.0);
  }

  void test_stitching_determines_params()
  {
    MantidVec x1 = boost::assign::list_of(-1.0)(-0.8)(-0.6)(-0.4)(-0.2)(0.0)(0.2)(0.4)(0.6)(0.8).convert_to_container<MantidVec>();
    MantidVec x2 = boost::assign::list_of(0.4)(0.6)(0.8)(1.0)(1.2)(1.4)(1.6).convert_to_container<MantidVec>();

    MatrixWorkspace_sptr ws1 = create1DWorkspace(x1, boost::assign::list_of(1)(1)(1)(1)(1)(1)(1)(1)(1).convert_to_container<MantidVec>());
    MatrixWorkspace_sptr ws2 = create1DWorkspace(x2, boost::assign::list_of(1)(1)(1)(1)(1)(1).convert_to_container<MantidVec>());
    double demanded_step_size = 0.2;
    auto ret = do_stitch1D(ws1,ws2,0.4,1.0,boost::assign::list_of(demanded_step_size).convert_to_container<MantidVec>());

    //Check the ranges on the output workspace against the param inputs.
    MantidVec out_x_values = ret.get<0>()->readX(0);
    double x_min = *std::min_element(out_x_values.begin(), out_x_values.end());
    double x_max = *std::max_element(out_x_values.begin(), out_x_values.end());
    double step_size = out_x_values[1] - out_x_values[0];

    TS_ASSERT_EQUALS(x_min, -1);
    TS_ASSERT_DELTA(x_max - demanded_step_size, 1.4, 0.000001);
    TS_ASSERT_DELTA(step_size, demanded_step_size, 0.000001);
  }

  void test_stitching_determines_start_and_end_overlap()
  {
    MantidVec x1 = boost::assign::list_of(-1.0)(-0.8)(-0.6)(-0.4)(-0.2)(0.0)(0.2)(0.4).convert_to_container<MantidVec>();
    MantidVec x2 = boost::assign::list_of(-0.4)(-0.2)(0.0)(0.2)(0.4)(0.6)(0.8)(1.0).convert_to_container<MantidVec>();
    MatrixWorkspace_sptr ws1 = create1DWorkspace(x1, boost::assign::list_of(1)(1)(1)(3)(3)(3)(3).convert_to_container<MantidVec>());
    MatrixWorkspace_sptr ws2 = create1DWorkspace(x2, boost::assign::list_of(1)(1)(1)(1)(3)(3)(3).convert_to_container<MantidVec>());

    auto ret = do_stitch1D(ws1,ws2,boost::assign::list_of(-1.0)(0.2)(1.0).convert_to_container<MantidVec>());

    MantidVec stitched_y = ret.get<0>()->readY(0);
    MantidVec stitched_x = ret.get<0>()->readX(0);
    std::vector<size_t> overlap_indexes = std::vector<size_t>();
    for (size_t itr = 0; itr < stitched_y.size(); ++itr)
    {
      if (stitched_y[itr] >= 1.0009 && stitched_y[itr] <= 3.0001)
      {
        overlap_indexes.push_back(itr);
      }
    }

    double start_overlap_determined = stitched_x[overlap_indexes[0]];
    double end_overlap_determined = stitched_x[overlap_indexes[overlap_indexes.size()-1]];
    TS_ASSERT_DELTA(start_overlap_determined, -0.4, 0.000000001);
    TS_ASSERT_DELTA(end_overlap_determined, 0.2, 0.000000001);
  }
};

#endif /* MANTID_ALGORITHMS_STITCH1DTEST_H_ */
