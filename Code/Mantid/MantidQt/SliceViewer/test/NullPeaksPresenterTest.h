#ifndef SLICE_VIEWER_NULLPEAKSPRESENTER_TEST_H_
#define SLICE_VIEWER_NULLPEAKSPRESENTER_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidQtSliceViewer/NullPeaksPresenter.h"

using namespace MantidQt::SliceViewer;

class NullPeaksPresenterTest : public CxxTest::TestSuite
{

public:

  void test_construction()
  {
      TS_ASSERT_THROWS_NOTHING(NullPeaksPresenter p);
  }

  void test_is_peaks_presenter()
  {
    NullPeaksPresenter presenter;
    PeaksPresenter& base = presenter; // compile-time test for the is-a relationship
    UNUSED_ARG(base);
  }

  /* Test individual methods on the interface */

  void test_update_does_nothing()
  {
    NullPeaksPresenter presenter;
    TS_ASSERT_THROWS_NOTHING(presenter.update());
  }

  void test_updateWithSlicePoint_does_nothing()
  {
    NullPeaksPresenter presenter;
    TS_ASSERT_THROWS_NOTHING(presenter.updateWithSlicePoint(0));
  }

  void test_changeShownDim_does_nothing()
  {
    NullPeaksPresenter presenter;
    TS_ASSERT(!presenter.changeShownDim());
  }

  void test_isLabelOfFreeAxis_always_returns_false()
  {
    NullPeaksPresenter presenter;
    TS_ASSERT(!presenter.isLabelOfFreeAxis(""));
  }

  void test_presentedWorkspaces_is_empty()
  {
    NullPeaksPresenter presenter;
    SetPeaksWorkspaces workspaces = presenter.presentedWorkspaces();
    TS_ASSERT_EQUALS(0, workspaces.size());
  }

  void test_setForegroundColour_does_nothing()
  {
    NullPeaksPresenter presenter;
    TS_ASSERT_THROWS_NOTHING(presenter.setForegroundColour(Qt::black));
  }

  void test_setBackgroundColour_does_nothing()
  {
    NullPeaksPresenter presenter;
    TS_ASSERT_THROWS_NOTHING(presenter.setBackgroundColour(Qt::black));
  }

  void test_setShown_does_nothing()
  {
    NullPeaksPresenter presenter;
    TS_ASSERT_THROWS_NOTHING(presenter.setShown(true));
    TS_ASSERT_THROWS_NOTHING(presenter.setShown(false));
  }

};

#endif