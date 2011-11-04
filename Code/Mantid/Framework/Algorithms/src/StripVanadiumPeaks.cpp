/*WIKI* 

* A list of vanadium peak positions in d-spacing is used for the central peak positions: 0.5044,0.5191,0.5350,0.5526,0.5936,0.6178,0.6453,0.6768,0.7134,0.7566,0.8089,0.8737,0.9571,1.0701,1.2356,1.5133,2.1401
** You can specify AlternativePeakPositions to use other value (e.g. in other units).

* The PeakWidthPercent value is used to estimate the width of the peak (as a percentage of the d-spacing value).
* The algorithm performs a simple linear fit of the background exluding the peak.
** It uses two use averaging regions of 1/2 width, centered at +- width/2 from the center, and interpolates the linear background from it.
** The values between the average regions are replaced with the interpolated linear background drawn as a straight line.

*WIKI*/

#include "MantidAlgorithms/StripVanadiumPeaks.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/TableRow.h"
#include "MantidKernel/VectorHelper.h"

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(StripVanadiumPeaks)

/// Sets documentation strings for this algorithm
void StripVanadiumPeaks::initDocs()
{
  this->setWikiSummary("This algorithm removes peaks (at vanadium d-spacing positions by default) out of a background by linearly interpolating over the expected peak positions. ");
  this->setOptionalMessage("This algorithm removes peaks (at vanadium d-spacing positions by default) out of a background by linearly interpolating over the expected peak positions.");
}


using namespace Kernel;
using namespace DataObjects;
using namespace API;
using namespace Kernel::VectorHelper;

StripVanadiumPeaks::StripVanadiumPeaks() : API::Algorithm() {}

void StripVanadiumPeaks::init()
{
  declareProperty(
    new WorkspaceProperty<>("InputWorkspace","",Direction::Input),
    "Name of the input workspace. If you use the default vanadium peak positions are used, the workspace must be in units of d-spacing." );
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
    "The name of the workspace to be created as the output of the algorithm.\n"
    "If the input workspace is an EventWorkspace, then the output must be different (and will be made into a Workspace2D)." );

  BoundedValidator<double> *min = new BoundedValidator<double>();
  min->setLower(1e-3);
  // The estimated width of a peak in terms of number of channels
  declareProperty("PeakWidthPercent", 1.0, min,
    "The estimated peak width as a percentage of the d-spacing of the center of the peak." );

  declareProperty("AlternativePeakPositions", "",
    "Optional: enter a comma-separated list of the expected X-position of the centre of the peaks. \n"
    "Only peaks near these positions will be fitted.\n"
    "If not entered, the default vanadium peak positions will be used.");

  BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
  mustBePositive->setLower(0);
  declareProperty("WorkspaceIndex",EMPTY_INT(),mustBePositive,
    "If set, peaks will only be removed from this spectrum (otherwise from all)");
}

void StripVanadiumPeaks::exec()
{
  // Retrieve the input workspace
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");

  // Check for trying to rewrite an EventWorkspace
  EventWorkspace_sptr inputEvent = boost::dynamic_pointer_cast<EventWorkspace>(inputWS);
  if (inputEvent && (getPropertyValue("InputWorkspace") == getPropertyValue("OutputWorkspace")))
  {
    throw std::invalid_argument("Cannot strip vanadium peaks in-place for an EventWorkspace. Please specify a different output workspace name, which will be a Workspace2D copy of the input EventWorkspace.");
  }


  // If WorkspaceIndex has been set it must be valid
  int singleIndex = getProperty("WorkspaceIndex");
  bool singleSpectrum = !isEmpty(singleIndex);
  if ( singleSpectrum && singleIndex >= static_cast<int>(inputWS->getNumberHistograms()) )
  {
    g_log.error() << "The value of WorkspaceIndex provided (" << singleIndex <<
                  ") is larger than the size of this workspace (" <<
                  inputWS->getNumberHistograms() << ")\n";
    throw Kernel::Exception::IndexError(singleIndex,inputWS->getNumberHistograms()-1,
      "StripVanadiumPeaks WorkspaceIndex property");
  }

  // Create an output workspace - same size as input one
  MatrixWorkspace_sptr outputWS = WorkspaceFactory::Instance().create(inputWS);
  // Copy the data over from the input to the output workspace
  const int nhists = static_cast<int>(inputWS->getNumberHistograms());
  Progress progress(this,0.0,1.0,nhists*2);
  for (int k = 0; k < nhists; ++k)
  {
    outputWS->dataX(k) = inputWS->readX(k);
    outputWS->dataY(k) = inputWS->readY(k);
    outputWS->dataE(k) = inputWS->readE(k);
    progress.report();
  }

  //Get the peak center positions
  std::string peakPositions = getPropertyValue("AlternativePeakPositions");
  if (peakPositions.length() == 0)
  {
    //Use the default Vanadium peak positions instead
    peakPositions = "0.5044,0.5191,0.5350,0.5526,0.5936,0.6178,0.6453,0.6768,0.7134,0.7566,0.8089,0.8737,0.9571,1.0701,1.2356,1.5133,2.1401";
    //Check for units
    if (inputWS->getAxis(0)->unit()->unitID() != "dSpacing")
      throw std::invalid_argument("Cannot strip using default Vanadium peak positions for an input workspace whose units are not d-spacing. Convert to d-spacing or specify your own alternative peak positions.");
  }
  std::vector<double> centers = Kernel::VectorHelper::splitStringIntoVector<double>(peakPositions);

  //Get the width percentage
  double widthPercent = getProperty("PeakWidthPercent");

  for (int k = 0; k < nhists; ++k)
  {
    if ((!singleSpectrum) || (singleIndex == k))
    {
      //Get the X and Y vectors
      MantidVec X = outputWS->dataX(k);
      MantidVec Y = outputWS->dataY(k);

      //Middle of each X bin
      MantidVec midX;
      convertToBinCentre(X, midX);
      double totY;

      //This'll be the output
      MantidVec outY = Y;

      //Strip each peak listed
      std::vector<double>::iterator it;
      for (it = centers.begin(); it != centers.end(); ++it)
      {
        //Peak center and width
        double center = *it;
        double width = center * widthPercent / 100.0;

        // Find the bin indices on both sides.
        //  We use averaging regions of 1/2 width, centered at +- width/2 from the center
        int L1 = getBinIndex(X, center - width * 0.75);
        int L2 = getBinIndex(X, center - width * 0.25);
        double leftX = (midX[L1] + midX[L2])/2;
        totY = 0;
        for (int i=L1; i<=L2; i++)
          totY += Y[i];
        double leftY = totY / (L2-L1+1);

        int R1 = getBinIndex(X, center + width * 0.25);
        int R2 = getBinIndex(X, center + width * 0.75);
        double rightX = (midX[R1] + midX[R2])/2;
        totY = 0;
        for (int i=R1; i<=R2; i++)
          totY += Y[i];
        double rightY = totY / (R2-R1+1);

        //Make a simple fit with these two points
        double slope = 1.0;
        if (fabs(rightX - leftX) > 0.0) //avoid divide by 0
          slope = (rightY - leftY) / (rightX - leftX);
        double abscissa = leftY - slope * leftX;

        //Now fill in the vector between the averaged areas
        for (int i=L2; i<=R1; i++)
        {
          outY[i] = midX[i] * slope + abscissa;
        }
      }

      //Save the output
      outputWS->dataY(k) = outY;
    }//if the spectrum is to be changed.
    progress.report();
  }//each spectrum

  //Save the output workspace
  setProperty("OutputWorkspace",outputWS);
}



} // namespace Algorithms
} // namespace Mantid
