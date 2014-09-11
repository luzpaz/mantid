#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataHandling/SaveParameterFile.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ParameterMap.h"

#include <boost/lexical_cast.hpp>
#include <boost/tuple/tuple.hpp>

#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/DOMWriter.h>
#include <Poco/DOM/Element.h>
#include <Poco/XML/XMLWriter.h>

#include <fstream>

namespace Mantid
{
namespace DataHandling
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(SaveParameterFile)

  using namespace Kernel;
  using namespace API;
  using namespace Geometry;

  using namespace Poco;

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  SaveParameterFile::SaveParameterFile()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  SaveParameterFile::~SaveParameterFile()
  { }

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string SaveParameterFile::name() const { return "SaveParameterFile";};

  /// Algorithm's version for identification. @see Algorithm::version
  int SaveParameterFile::version() const { return 1;};

  /// Algorithm's category for identification. @see Algorithm::category
  const std::string SaveParameterFile::category() const { return "DataHandling\\Instrument";}

  //----------------------------------------------------------------------------------------------

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void SaveParameterFile::init()
  {
    declareProperty(new WorkspaceProperty<>("Workspace","",Direction::Input,boost::make_shared<InstrumentValidator>()),
    "Workspace to save the instrument parameters from.");

    std::vector<std::string> exts;
    exts.push_back(".xml");

    declareProperty(new API::FileProperty("Filename","", API::FileProperty::Save, exts),
    "The name of the file into which the instrument parameters will be saved.");

    declareProperty("LocationParameters", false, "Save the location parameters used to calibrate the instrument.", Direction::Input);
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void SaveParameterFile::exec()
  {
    const MatrixWorkspace_const_sptr ws = getProperty("Workspace");
    const bool saveLocationParams = getProperty("LocationParameters");
    const std::string filename = getProperty("Filename");

    const Instrument_const_sptr instrument = ws->getInstrument();
    const ParameterMap_sptr params = instrument->getParameterMap();

    //Map of component ids to their respective XML Elements
    std::map<ComponentID,XML::AutoPtr<XML::Element> > compMap;

    //Set up the XML document
    XML::AutoPtr<XML::Document> xmlDoc = new XML::Document;
    XML::AutoPtr<XML::Element> rootElem = xmlDoc->createElement("parameter-file");
    rootElem->setAttribute("instrument", instrument->getName());
    rootElem->setAttribute("valid-from", instrument->getValidFromDate().toISO8601String());
    xmlDoc->appendChild(rootElem);

    //Vector of tuples: (componentid, paramname, paramtype, paramvalue)
    std::vector<boost::tuple<ComponentID, std::string, std::string, std::string> > toSave;

    //Build a list of parameters to save;
    for(auto paramsIt = params->begin(); paramsIt != params->end(); ++paramsIt)
    {
      const ComponentID cID = (*paramsIt).first;
      const std::string pName = (*paramsIt).second->name();
      const std::string pType = (*paramsIt).second->type();
      const std::string pValue = (*paramsIt).second->asString();


      if(pName == "x"          || pName == "y"          || pName == "z"          ||
         pName == "r-position" || pName == "t-position" || pName == "p-position" ||
         pName == "rotx"       || pName == "roty"       || pName == "rotz"        )
      {
        g_log.warning() << "The parameter name '" << pName << "' is reserved and has not been saved. "
                        << "Please contact the Mantid team for more information.";
        continue;
      }

      //If it isn't a position or rotation parameter, we can just add it to the list to save directly and move on.
      if(pName != "pos" && pName != "rot")
      {
        toSave.push_back(boost::make_tuple(cID, pName, pType, pValue));
      }

      if(saveLocationParams)
      {
        const IComponent* baseComp = cID->getBaseComponent();

        //Check if the position has been changed by a parameter
        //If so, check each axis and add the relevant adjustment parameters to the to-save list.
        const V3D basePos = baseComp->getPos();
        const V3D absPos = cID->getPos();
        const V3D posDiff = absPos - basePos;

        if(posDiff.X() != 0)
          toSave.push_back(boost::make_tuple(cID, "x", "double", Strings::toString<double>(absPos.X())));
        if(posDiff.Y() != 0)
          toSave.push_back(boost::make_tuple(cID, "y", "double", Strings::toString<double>(absPos.Y())));
        if(posDiff.Z() != 0)
          toSave.push_back(boost::make_tuple(cID, "z", "double", Strings::toString<double>(absPos.Z())));

        //Check if the rotation has been changed by a parameter
        //If so, convert to Euler (XYZ order) and output each component that differs
        const Quat baseRot = baseComp->getRotation();
        const Quat absRot = cID->getRotation();

        if(baseRot != absRot)
        {
          //Euler rotation components are not independent so write them all out to be safe.
          std::vector<double> absEuler = absRot.getEulerAngles("XYZ");
          toSave.push_back(boost::make_tuple(cID, "rotx", "double", Strings::toString<double>(absEuler[0])));
          toSave.push_back(boost::make_tuple(cID, "roty", "double", Strings::toString<double>(absEuler[1])));
          toSave.push_back(boost::make_tuple(cID, "rotz", "double", Strings::toString<double>(absEuler[2])));
        }
      }
    }

    //Iterate through all the parameters we want to save and build an XML
    //document out of them.
    for(auto paramsIt = toSave.begin(); paramsIt != toSave.end(); ++paramsIt)
    {
      //Component data
      const ComponentID cID = boost::get<0>(*paramsIt);
      const std::string cFullName = cID->getFullName();
      const IDetector* cDet = dynamic_cast<IDetector*>(cID);
      const detid_t cDetID = (cDet) ? cDet->getID() : 0;
      const std::string cDetIDStr = boost::lexical_cast<std::string>(cDetID);

      //Parameter data
      const std::string pName = boost::get<1>(*paramsIt);
      const std::string pType = boost::get<2>(*paramsIt);
      const std::string pValue = boost::get<3>(*paramsIt);

      //A component-link element
      XML::AutoPtr<XML::Element> compElem = 0;

      /* If an element already exists for a component with this name, re-use it.
       *
       * Why are we using an std::map and not simply traversing the DOM? Because
       * the interface for doing that is painful and horrible to use, and this is
       * probably faster (but negligably so in this case).
       *
       * And lastly, because Poco::XML::NodeList::length() segfaults.
       */
      auto compIt = compMap.find(cID);
      if(compIt != compMap.end())
      {
        compElem = (*compIt).second;
      }

      //One doesn't already exist? Make a new one.
      if(!compElem)
      {
        compElem = xmlDoc->createElement("component-link");
        rootElem->appendChild(compElem);
        compMap[cID] = compElem;
      }

      //Create the parameter and value elements
      XML::AutoPtr<XML::Element> paramElem = xmlDoc->createElement("parameter");
      XML::AutoPtr<XML::Element> valueElem = xmlDoc->createElement("value");

      //Set the attributes
      compElem->setAttribute("name", cFullName);

      //If there is a valid detector id, include it
      if(cDetID > 0)
      {
        compElem->setAttribute("id", cDetIDStr);
      }

      paramElem->setAttribute("name", pName);

      //For strings, we specify their type.
      if(pType == "string")
      {
        paramElem->setAttribute("type", "string");
      }

      valueElem->setAttribute("val", pValue);

      //Insert the elements into the document
      compElem->appendChild(paramElem);
      paramElem->appendChild(valueElem);
    }

    //Output the XMl document to the given file.
    XML::DOMWriter writer;
    writer.setOptions(XML::XMLWriter::PRETTY_PRINT | XML::XMLWriter::WRITE_XML_DECLARATION);
    std::ofstream file(filename.c_str(), std::ofstream::trunc);
    try
    {
      writer.writeNode(file, xmlDoc);
    }
    catch(Poco::Exception &e)
    {
      g_log.error() << "Error serializing XML for SaveParameterFile: " << e.displayText() << std::endl;
    }
    file.flush();
    file.close();

  }

} // namespace Algorithms
} // namespace Mantid
