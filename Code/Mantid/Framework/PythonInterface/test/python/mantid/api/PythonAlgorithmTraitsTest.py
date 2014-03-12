"""Defines tests for the traits within Python algorithms
such as name, version etc.
"""

import unittest
import testhelpers
import types

from mantid.kernel import Direction
from mantid.api import (PythonAlgorithm, AlgorithmProxy, Algorithm, IAlgorithm, 
                        AlgorithmManager, AlgorithmFactory)

########################### Test classes #####################################

class TestPyAlgDefaultAttrs(PythonAlgorithm):
    def PyInit(self):
        pass
    
    def PyExec(self):
        pass

class TestPyAlgOverriddenAttrs(PythonAlgorithm):
    
    def version(self):
        return 2
    
    def category(self):
        return "BestAlgorithms"
    
    def isRunning(self):
        return True
    
    def PyInit(self):
        pass
    
    def PyExec(self):
        pass
    
class TestPyAlgIsRunningReturnsNonBool(PythonAlgorithm):

    def isRunning(self):
        return 1
    
    def PyInit(self):
        pass
    
    def PyExec(self):
        pass

###############################################################################

class PythonAlgorithmTest(unittest.TestCase):
        
    _registered = None
    
    def setUp(self):
        if self.__class__._registered is None:
            self.__class__._registered = True
            AlgorithmFactory.subscribe(TestPyAlgDefaultAttrs)
            AlgorithmFactory.subscribe(TestPyAlgOverriddenAttrs)
            AlgorithmFactory.subscribe(TestPyAlgIsRunningReturnsNonBool)
            
    def test_managed_alg_is_descendent_of_AlgorithmProxy(self):
        alg = AlgorithmManager.create("TestPyAlgDefaultAttrs")
        self.assertTrue(isinstance(alg, AlgorithmProxy))
        self.assertTrue(isinstance(alg, IAlgorithm))

    def test_unmanaged_alg_is_descendent_of_PythonAlgorithm(self):
        alg = AlgorithmManager.createUnmanaged("TestPyAlgDefaultAttrs")
        self.assertTrue(isinstance(alg, PythonAlgorithm))
        self.assertTrue(isinstance(alg, Algorithm))
        self.assertTrue(isinstance(alg, IAlgorithm))
        
    def test_alg_with_default_attrs(self):
        testhelpers.assertRaisesNothing(self,AlgorithmManager.createUnmanaged, "TestPyAlgDefaultAttrs")
        alg = AlgorithmManager.createUnmanaged("TestPyAlgDefaultAttrs")
        testhelpers.assertRaisesNothing(self,alg.initialize)
       
        self.assertEquals(alg.name(), "TestPyAlgDefaultAttrs")
        self.assertEquals(alg.version(), 1)
        self.assertEquals(alg.category(), "PythonAlgorithms")
        self.assertEquals(alg.isRunning(), False)

    def test_alg_with_overridden_attrs(self):
        testhelpers.assertRaisesNothing(self,AlgorithmManager.createUnmanaged, "TestPyAlgOverriddenAttrs")
        alg = AlgorithmManager.createUnmanaged("TestPyAlgOverriddenAttrs")
        self.assertEquals(alg.name(), "TestPyAlgOverriddenAttrs")
        self.assertEquals(alg.version(), 2)
        self.assertEquals(alg.category(), "BestAlgorithms")
        self.assertEquals(alg.isRunning(), True)

    # --------------------------- Failure cases --------------------------------------------
    def test_isRunning_returning_non_bool_raises_error(self):
        alg = AlgorithmManager.createUnmanaged("TestPyAlgIsRunningReturnsNonBool")
        self.assertRaises(RuntimeError, alg.isRunning)

if __name__ == '__main__':
    unittest.main()
