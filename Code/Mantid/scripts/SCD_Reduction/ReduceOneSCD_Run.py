# File: ReduceOneSCD_Run.py
#
# Version 2.0, modified to work with Mantid's new python interface.
#
# This script will reduce one SCD run.  The configuration file name and
# the run to be processed must be specified as the first two command line 
# parameters.  This script is intended to be run in parallel using the 
# ReduceSCD_Parallel.py script, after this script and configuration file has 
# been tested to work properly for one run. This script will load, find peaks,
# index and integrate either found or predicted peaks for the specified run.  
# Either sphere integration or the Mantid PeakIntegration algorithms are 
# currently supported, but it may be updated to support other integration 
# methods.  Users should make a directory to hold the output of this script, 
# and must specify that output directory in the configuration file that 
# provides the parameters to this script.
#
# NOTE: All of the parameters that the user must specify are listed with 
# instructive comments in the sample configuration file: ReduceSCD.config.
#
import os
import sys
import shutil
import time
import ReduceDictionary
sys.path.append("/opt/mantidnightly/bin")
#sys.path.append("/opt/Mantid/bin")

from mantid.simpleapi import *
from mantid.api import *

print "API Version"
print apiVersion()

start_time = time.time()

#
# Get the config file name and the run number to process from the command line
#
if (len(sys.argv) < 3):
  print "You MUST give the config file name and run number on the command line"
  exit(0)

config_file_name = sys.argv[1]
run              = sys.argv[2]

#
# Load the parameter names and values from the specified configuration file 
# into a dictionary and set all the required parameters from the dictionary.
#
params_dictionary = ReduceDictionary.LoadDictionary( config_file_name )

instrument_name           = params_dictionary.get('instrument_name', "TOPAZ")
calibration_file_1        = params_dictionary.get('calibration_file_1', None)
calibration_file_2        = params_dictionary.get('calibration_file_2', None)
data_directory            = params_dictionary.get('data_directory', None)
output_directory          = params_dictionary.get('output_directory', None)
min_tof                   = params_dictionary.get('min_tof', "400") 
max_tof                   = params_dictionary.get('max_tof', "16666") 
min_monitor_tof           = params_dictionary.get('min_monitor_tof', "1000") 
max_monitor_tof           = params_dictionary.get('max_monitor_tof', "12500") 
monitor_index             = params_dictionary.get('monitor_index', "0") 
cell_type                 = params_dictionary.get('cell_type', None) 
centering                 = params_dictionary.get('centering', None)
num_peaks_to_find         = params_dictionary.get('num_peaks_to_find', "500")
min_d                     = params_dictionary.get('min_d', "4")
max_d                     = params_dictionary.get('max_d', "8")
max_Q                     = params_dictionary.get('max_Q', "30")
tolerance                 = params_dictionary.get('tolerance', "0.12")
integrate_predicted_peaks = params_dictionary.get('integrate_predicted_peaks', False)
min_pred_wl               = params_dictionary.get('min_pred_wl', "0.25")
max_pred_wl               = params_dictionary.get('max_pred_wl', "3.5")
min_pred_dspacing         = params_dictionary.get('min_pred_dspacing', "0.2")
max_pred_dspacing         = params_dictionary.get('max_pred_dspacing', "2.5")

use_sphere_integration    = params_dictionary.get('use_sphere_integration', True)
use_cylinder_integration    = params_dictionary.get('use_cylinder_integration', False)
use_ellipse_integration   = params_dictionary.get('use_ellipse_integration', False)
use_fit_peaks_integration = params_dictionary.get('use_fit_peaks_integration', False)

peak_radius               = params_dictionary.get('peak_radius', "0.18")
bkg_inner_radius          = params_dictionary.get('bkg_inner_radius', "0.18")
bkg_outer_radius          = params_dictionary.get('bkg_outer_radius', "0.23")
integrate_if_edge_peak    = params_dictionary.get('integrate_if_edge_peak', True)

cylinder_length           = params_dictionary.get('cylinder_length', "0")
cylinder_percent_bkg      = params_dictionary.get('cylinder_percent_bkg', "0")
cylinder_int_option       = params_dictionary.get('cylinder_int_option', "GaussianQuadrature")
cylinder_profile_fit      = params_dictionary.get('cylinder_profile_fit', "Gaussian")

rebin_step                = params_dictionary.get('rebin_step', "-0.004")
preserve_events           = params_dictionary.get('preserve_events', True) 
use_ikeda_carpenter       = params_dictionary.get('use_ikeda_carpenter', False)
n_bad_edge_pixels         = params_dictionary.get('n_bad_edge_pixels', "10")

rebin_params = min_tof + ',' + rebin_step + ',' + max_tof

ellipse_region_radius     = params_dictionary.get('ellipse_region_radius', "0.45")
ellipse_size_specified    = params_dictionary.get('ellipse_size_specified', False)

#
# Get the fully qualified input run file name, either from a specified data 
# directory or from findnexus
#
if data_directory is not None:
  full_name = data_directory + "/" + instrument_name + "_" + run + "_event.nxs"
else:
  temp_buffer = os.popen("findnexus --event -i "+instrument_name+" "+str(run) )
  full_name = temp_buffer.readline()
  full_name=full_name.strip()
  if not full_name.endswith('nxs'):
    print "Exiting since the data_directory was not specified and"
    print "findnexus failed for event NeXus file: " + instrument_name + " " + str(run)
    exit(0)

print "\nProcessing File: " + full_name + " ......\n"

#
# Name the files to write for this run
#
run_niggli_matrix_file = output_directory + "/" + run + "_Niggli.mat"
run_niggli_integrate_file = output_directory + "/" + run + "_Niggli.integrate"

#
# Load the run data and find the total monitor counts
#
event_ws = LoadEventNexus( Filename=full_name, 
                           FilterByTofMin=min_tof, FilterByTofMax=max_tof )

#
# Load calibration file(s) if specified.  NOTE: The file name passed in to LoadIsawDetCal
# can not be None.  TOPAZ has one calibration file, but SNAP may have two.
#
if (calibration_file_1 is not None ) or (calibration_file_2 is not None):
  if (calibration_file_1 is None ):
    calibration_file_1 = ""
  if (calibration_file_2 is None ):
    calibration_file_2 = ""
  LoadIsawDetCal( event_ws, 
                  Filename=calibration_file_1, Filename2=calibration_file_2 )  

monitor_ws = LoadNexusMonitors( Filename=full_name )

integrated_monitor_ws = Integration( InputWorkspace=monitor_ws, 
                                     RangeLower=min_monitor_tof, RangeUpper=max_monitor_tof, 
                                     StartWorkspaceIndex=monitor_index, EndWorkspaceIndex=monitor_index )

monitor_count = integrated_monitor_ws.dataY(0)[0]
print "\n", run, " has calculated monitor count", monitor_count, "\n"


minVals= "-"+max_Q +",-"+max_Q +",-"+max_Q
maxVals = max_Q +","+max_Q +","+ max_Q 
#
# Make MD workspace using Lorentz correction, to find peaks 
#
MDEW = ConvertToMD( InputWorkspace=event_ws, QDimensions="Q3D",
                    dEAnalysisMode="Elastic", QConversionScales="Q in A^-1",
   	            LorentzCorrection='1', MinValues=minVals, MaxValues=maxVals,
                    SplitInto='2', SplitThreshold='50',MaxRecursionDepth='14' )
#
# Find the requested number of peaks.  Once the peaks are found, we no longer
# need the weighted MD event workspace, so delete it.
#
distance_threshold = 0.9 * 6.28 / float(max_d)
peaks_ws = FindPeaksMD( MDEW, MaxPeaks=num_peaks_to_find, 
                        PeakDistanceThreshold=distance_threshold )
AnalysisDataService.remove( MDEW.getName() )

#
# Find a Niggli UB matrix that indexes the peaks in this run
#
FindUBUsingFFT( PeaksWorkspace=peaks_ws, MinD=min_d, MaxD=max_d, Tolerance=tolerance )
IndexPeaks( PeaksWorkspace=peaks_ws, Tolerance=tolerance )

#
# Save UB and peaks file, so if something goes wrong latter, we can at least 
# see these partial results
#
SaveIsawUB( InputWorkspace=peaks_ws,Filename=run_niggli_matrix_file )
SaveIsawPeaks( InputWorkspace=peaks_ws, AppendFile=False,
               Filename=run_niggli_integrate_file )

#
# Get complete list of peaks to be integrated and load the UB matrix into
# the predicted peaks workspace, so that information can be used by the
# PeakIntegration algorithm.
#
if integrate_predicted_peaks:
  print "PREDICTING peaks to integrate...."
  peaks_ws = PredictPeaks( InputWorkspace=peaks_ws,
                WavelengthMin=min_pred_wl, WavelengthMax=max_pred_wl,
                MinDSpacing=min_pred_dspacing, MaxDSpacing=max_pred_dspacing, 
                ReflectionCondition='Primitive' )
else:
  print "Only integrating FOUND peaks ...."
#
# Set the monitor counts for all the peaks that will be integrated
#
num_peaks = peaks_ws.getNumberPeaks()
for i in range(num_peaks):
  peak = peaks_ws.getPeak(i)
  peak.setMonitorCount( monitor_count )
    
if use_sphere_integration:
#
# Integrate found or predicted peaks in Q space using spheres, and save 
# integrated intensities, with Niggli indexing.  First get an un-weighted 
# workspace to do raw integration (we don't need high resolution or 
# LorentzCorrection to do the raw sphere integration )
#
  MDEW = ConvertToMD( InputWorkspace=event_ws, QDimensions="Q3D",
                    dEAnalysisMode="Elastic", QConversionScales="Q in A^-1",
                    LorentzCorrection='0', MinValues=minVals, MaxValues=maxVals,
                    SplitInto='2', SplitThreshold='500',MaxRecursionDepth='10' )

  peaks_ws = IntegratePeaksMD( InputWorkspace=MDEW, PeakRadius=peak_radius,
                  CoordinatesToUse="Q (sample frame)",
	          BackgroundOuterRadius=bkg_outer_radius, 
                  BackgroundInnerRadius=bkg_inner_radius,
	          PeaksWorkspace=peaks_ws, 
                  IntegrateIfOnEdge=integrate_if_edge_peak )
elif use_cylinder_integration:
#
# Integrate found or predicted peaks in Q space using spheres, and save 
# integrated intensities, with Niggli indexing.  First get an un-weighted 
# workspace to do raw integration (we don't need high resolution or 
# LorentzCorrection to do the raw sphere integration )
#
  MDEW = ConvertToMD( InputWorkspace=event_ws, QDimensions="Q3D",
                    dEAnalysisMode="Elastic", QConversionScales="Q in A^-1",
                    LorentzCorrection='0', MinValues=minVals, MaxValues=maxVals,
                    SplitInto='2', SplitThreshold='500',MaxRecursionDepth='10' )

  peaks_ws = IntegratePeaksMD( InputWorkspace=MDEW, PeakRadius=peak_radius,
                  CoordinatesToUse="Q (sample frame)",
	              BackgroundOuterRadius=bkg_outer_radius, 
                  BackgroundInnerRadius=bkg_inner_radius,
	              PeaksWorkspace=peaks_ws, 
                  IntegrateIfOnEdge=integrate_if_edge_peak, 
                  Cylinder=use_cylinder_integration,CylinderLength=cylinder_length,
                  PercentBackground=cylinder_percent_bkg,
                  IntegrationOption=cylinder_int_option,
                  ProfileFunction=cylinder_profile_fit)

elif use_fit_peaks_integration:
  event_ws = Rebin( InputWorkspace=event_ws,
                    Params=rebin_params, PreserveEvents=preserve_events )
  peaks_ws = PeakIntegration( InPeaksWorkspace=peaks_ws, InputWorkspace=event_ws, 
                              IkedaCarpenterTOF=use_ikeda_carpenter,
                              MatchingRunNo=True,
                              NBadEdgePixels=n_bad_edge_pixels )

elif use_ellipse_integration:
  peaks_ws= IntegrateEllipsoids( InputWorkspace=event_ws, PeaksWorkspace = peaks_ws,
                                 RegionRadius = ellipse_region_radius,
                                 SpecifySize = ellipse_size_specified,
                                 PeakSize = peak_radius,
                                 BackgroundOuterSize = bkg_outer_radius,
                                 BackgroundInnerSize = bkg_inner_radius )

#
# Save the final integrated peaks, using the Niggli reduced cell.  
# This is the only file needed, for the driving script to get a combined
# result.
#
SaveIsawPeaks( InputWorkspace=peaks_ws, AppendFile=False, 
               Filename=run_niggli_integrate_file )

#
# If requested, also switch to the specified conventional cell and save the
# corresponding matrix and integrate file
#
if (not cell_type is None) and (not centering is None) :
  run_conventional_matrix_file = output_directory + "/" + run + "_" +    \
                                 cell_type + "_" + centering + ".mat"
  run_conventional_integrate_file = output_directory + "/" + run + "_" + \
                                    cell_type + "_" + centering + ".integrate"
  SelectCellOfType( PeaksWorkspace=peaks_ws, 
                    CellType=cell_type, Centering=centering, 
                    Apply=True, Tolerance=tolerance )
  SaveIsawPeaks( InputWorkspace=peaks_ws, AppendFile=False, 
                 Filename=run_conventional_integrate_file )
  SaveIsawUB( InputWorkspace=peaks_ws, Filename=run_conventional_matrix_file )

end_time = time.time()
print '\nReduced run ' + str(run) + ' in ' + str(end_time - start_time) + ' sec'
print 'using config file ' + config_file_name 

#
# Try to get this to terminate when run by ReduceSCD_Parallel.py, from NX session
#
sys.exit(0)

