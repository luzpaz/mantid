"""*WIKI* 

Stitches single histogram [[MatrixWorkspace|Matrix Workspaces]] together outputting a stitched Matrix Workspace. This algorithm is a wrapper over [[Stitch1D]].

The algorithm expects  pairs of StartOverlaps and EndOverlaps values. The order in which these are provided determines the pairing. 
There should be N entries in each of these StartOverlaps and EndOverlaps lists, where N = 1 -(No of workspaces to stitch). 
StartOverlaps and EndOverlaps are in the same units as the X-axis for the workspace and are optional.

The workspaces must be histogrammed. Use [[ConvertToHistogram]] on workspaces prior to passing them to this algorithm.
*WIKI*"""
#from mantid.simpleapi import *

from mantid.simpleapi import *
from mantid.api import *
from mantid.kernel import *
import numpy as np

class Stitch1DMany(PythonAlgorithm):

    def category(self):
        return "Reflectometry\\ISIS;PythonAlgorithms"

    def name(self):
        return "Stitch1D"

    def PyInit(self):
    
        input_validator = StringMandatoryValidator()
    
        self.declareProperty(name="InputWorkspaces", defaultValue="", direction=Direction.Input, validator=input_validator, doc="Input workspaces")
        self.declareProperty(WorkspaceProperty("OutputWorkspace", "", Direction.Output), "Output stitched workspace")
        
        self.declareProperty(FloatArrayProperty(name="StartOverlaps", values=[]), doc="Overlap in Q.")
        self.declareProperty(FloatArrayProperty(name="EndOverlaps", values=[]), doc="End overlap in Q.")
        self.declareProperty(FloatArrayProperty(name="Params", validator=FloatArrayMandatoryValidator()), doc="Rebinning Parameters. See Rebin for format.")
        self.declareProperty(name="ScaleRHSWorkspace", defaultValue=True, doc="Scaling either with respect to workspace 1 or workspace 2.")
        self.declareProperty(name="UseManualScaleFactor", defaultValue=False, doc="True to use a provided value for the scale factor.")
        self.declareProperty(name="ManualScaleFactor", defaultValue=1.0, doc="Provided value for the scale factor.")
        self.declareProperty(name="OutScaleFactor", defaultValue=-2.0, direction = Direction.Output, doc="The actual used value for the scaling factor.")
        
    def __workspace_from_split_name(self, list_of_names, index):
        return mtd[list_of_names[index].strip()]
        
    def __workspaces_from_split_name(self, list_of_names):
        workspaces = list()
        for name in list_of_names:
            workspaces.append(mtd[name])
        return workspaces
            
    '''
    If the property value has been provided, use that. If default, then create an array of Nones so that Stitch1D can still be correctly looped over.
    '''
    def __input_or_safe_default(self, property_name, n_entries):
        property = self.getProperty(property_name)
        property_value = property.value
        if property.isDefault:
            property_value = list()
            for i in range(0, n_entries):
                property_value.append(None)
        return property_value
                
       
    def __do_stitch_workspace(self, lhs_ws, rhs_ws, start_overlap, end_overlap, params, scale_rhs_ws, use_manual_scale_factor, manual_scale_factor):     
        out_name = lhs_ws.name() + rhs_ws.name()
        out_ws, scale_factor = Stitch1D(LHSWorkspace=lhs_ws, RHSWorkspace=rhs_ws, StartOverlap=start_overlap, EndOverlap=end_overlap, 
                                        Params=params, ScaleRHSWorkspace=scale_rhs_ws, UseManualScaleFactor=use_manual_scale_factor, ManualScaleFactor=manual_scale_factor, OutputWorkspace=out_name)
        return (out_ws, scale_factor)
            
    def PyExec(self):
    
        inputWorkspaces = self.getProperty("InputWorkspaces").value
        inputWorkspaces = inputWorkspaces.split(',')
        numberOfWorkspaces = len(inputWorkspaces)
        
        startOverlaps = self.__input_or_safe_default('StartOverlaps', numberOfWorkspaces-1)
        endOverlaps = self.__input_or_safe_default('EndOverlaps', numberOfWorkspaces-1)
        
        # Just forward the other properties on.
        scaleRHSWorkspace = self.getProperty('ScaleRHSWorkspace').value
        useManualScaleFactor = self.getProperty('UseManualScaleFactor').value
        manualScaleFactor = self.getProperty('ManualScaleFactor').value
        params = self.getProperty("Params").value
        
        
        if not numberOfWorkspaces > 1:
            raise ValueError("Too few workspaces to stitch")
        if not (len(startOverlaps) == len(endOverlaps)):
            raise ValueError("StartOverlaps and EndOverlaps are different lengths")
        if not (len(startOverlaps) == (numberOfWorkspaces- 1)):
            raise ValueError("Wrong number of StartOverlaps, should be %i not %i" % (numberOfWorkspaces - 1, startOverlaps))
        
        scaleFactor = None
        
        # Iterate forward through the workspaces
        if scaleRHSWorkspace:
            lhsWS = self.__workspace_from_split_name(inputWorkspaces, 0)   
            print "SCALER RHS"
            if isinstance(lhsWS, WorkspaceGroup):
                print "WORKSPACE GROUP"
                workspace_groups = self.__workspaces_from_split_name(inputWorkspaces)
                
                group_separator = ""
                group_workspaces = ""
                # TODO. VERIFY THAT ALL INPUT WORKSPACES ARE GROUP WORKSPACES
                print "TOTAL GROUP SIZE", lhsWS.size()
                for i in range(lhsWS.size()):
                    print "i", i
                    
                    
                    to_process = ""
                    out_name = ""
                    separator = ""
                    print "NUMBER OF WORKSPACES", numberOfWorkspaces
                    for j in range(0, numberOfWorkspaces, 1):
                        print "j", j
                        
                        to_process += separator + workspace_groups[j][i].name()
                        out_name += workspace_groups[j][i].name()
                        separator=","
                    out_name += ("_" + str(i+1))
                        
                    
                    startOverlaps = self.getProperty("StartOverlaps").value
                    endOverlaps = self.getProperty("EndOverlaps").value
                    stitched, scaleFactor = Stitch1DMany(InputWorkspaces=to_process, OutputWorkspace=out_name, StartOverlaps=startOverlaps, EndOverlaps=endOverlaps, 
                                                         Params=params, ScaleRHSWorkspace=scaleRHSWorkspace, UseManualScaleFactor=useManualScaleFactor,  
                                                         ManualScaleFactor=manualScaleFactor)
                        
                    #lhsWS, scaleFactor = self.__do_stitch_workspace(lhsWS, rhsWS, startOverlaps[j-1], endOverlaps[j-1], params, scaleRHSWorkspace,  useManualScaleFactor, manualScaleFactor)
                    
                    group_workspaces += group_separator + out_name
                    group_separator = ","
                    
                out_group = GroupWorkspaces(InputWorkspaces=group_workspaces)
                self.setProperty('OutputWorkspace', out_group)    
            else:
                # TODO. VERIFY THAT ALL INPUT WORKSPACES ARE NOT GROUP WORKSPACES
                for i in range(1, numberOfWorkspaces, 1):
                    rhsWS = self.__workspace_from_split_name(inputWorkspaces, i)
                    #lhsWS, scaleFactor = Stitch1D(LHSWorkspace=lhsWS, RHSWorkspace=rhsWS, StartOverlap=startOverlaps[i-1], EndOverlap=endOverlaps[i-1], Params=params, ScaleRHSWorkspace=scaleRHSWorkspace, UseManualScaleFactor=useManualScaleFactor,  ManualScaleFactor=manualScaleFactor)
                    lhsWS, scaleFactor = self.__do_stitch_workspace(lhsWS, rhsWS, startOverlaps[i-1], endOverlaps[i-1], params, scaleRHSWorkspace,  useManualScaleFactor, manualScaleFactor)
                self.setProperty('OutputWorkspace', lhsWS)
                DeleteWorkspace(lhsWS)
            
        # Iterate backwards through the workspaces.
        else:
            rhsWS = self.__workspace_from_split_name(inputWorkspaces, -1) 
            for i in range(0, numberOfWorkspaces-1, 1):
                lhsWS = self.__workspace_from_split_name(inputWorkspaces, i)
                rhsWS, scaleFactor = Stitch1D(LHSWorkspace=lhsWS, RHSWorkspace=rhsWS, StartOverlap=startOverlaps[i-1], EndOverlap=endOverlaps[i-1], Params=params, ScaleRHSWorkspace=scaleRHSWorkspace, UseManualScaleFactor=useManualScaleFactor,  ManualScaleFactor=manualScaleFactor)            
            self.setProperty('OutputWorkspace', rhsWS)
            DeleteWorkspace(rhsWS)
        
        self.setProperty('OutScaleFactor', scaleFactor)
        return None
        

#############################################################################################

AlgorithmFactory.subscribe(Stitch1DMany)
