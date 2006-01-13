/*! 
* \file food_supply_sector.cpp
* \ingroup CIAM
* \brief FoodSupplySector class source file.
* \author James Blackwood
* \date $Date$
* \version $Revision$
*/

#include "util/base/include/definitions.h"
#include <string>
#include <xercesc/dom/DOMNode.hpp>
#include "util/base/include/xml_helper.h"

#include "sectors/include/food_supply_sector.h"
#include "sectors/include/food_supply_subsector.h"
#include "util/base/include/model_time.h"
#include "marketplace/include/marketplace.h"
#include "containers/include/iinfo.h"
#include "containers/include/scenario.h"

using namespace std;
using namespace xercesc;

extern Scenario* scenario;

/*! \brief Default constructor.
* \author James Blackwood
*/
FoodSupplySector::FoodSupplySector( std::string& regionName ): SupplySector( regionName ) {
	calPrice = 0;
    mSectorType = "Agriculture"; //Default sector type for ag production sectors
}

//! Default destructor
FoodSupplySector::~FoodSupplySector( ) {
}

/*! \brief Parses any attributes specific to derived classes
* \author Josh Lurz, James Blackwood
* \param nodeName The name of the curr node. 
* \param curr pointer to the current node in the XML input tree
*/
bool FoodSupplySector::XMLDerivedClassParse( const string& nodeName, const DOMNode* curr ){
	if ( nodeName == FoodSupplySubsector::getXMLNameStatic() ) {
		parseContainerNode( curr, subsec, subSectorNameMap, new FoodSupplySubsector( regionName, name ) );
	}
	else if ( nodeName == "calPrice" ) {
		calPrice = XMLHelper<double>::getValue( curr );
	}
    else if( nodeName == "market" ){
		mMarketName = XMLHelper<string>::getValueString( curr );
	}
    else if( !SupplySector::XMLDerivedClassParse( nodeName, curr ) ) {
		return false;
	}
	return true;
}

/*! \brief XML output stream for derived classes
*
* Function writes output due to any variables specific to derived classes to XML
*
* \author Steve Smith, Josh Lurz
* \param out reference to the output stream
* \param tabs A tabs object responsible for printing the correct number of tabs. 
*/
void FoodSupplySector::toInputXMLDerived( ostream& out, Tabs* tabs ) const {
    SupplySector::toInputXMLDerived( out, tabs );
    XMLWriteElement( calPrice, "calPrice", out, tabs );
    XMLWriteElement( mMarketName, "market", out, tabs );
}	

void FoodSupplySector::toDebugXMLDerived( const int period, std::ostream& out, Tabs* tabs ) const {
    SupplySector::toDebugXMLDerived( period, out, tabs );
    XMLWriteElement( calPrice, "calPrice", out, tabs );
    XMLWriteElement( mMarketName, "market", out, tabs );
}

/*! \brief Perform any initializations specific to this sector
*
* Is called AFTER sector, subsector, and technology initializations
*
* \author James Blackwood
*/
void FoodSupplySector::completeInit( const IInfo* aRegionInfo,
                                     DependencyFinder* aDependencyFinder,
                                     ILandAllocator* aLandAllocator )
{
    if ( !aLandAllocator ) {
        ILogger& mainLog = ILogger::getLogger( "main_log" );
        mainLog.setLevel( ILogger::ERROR );
        mainLog << "LandAllocator not read in." << endl;
    }

    // If there is a calprice, initialize marketplace with this
    if ( calPrice >= 0 ) {
        const Modeltime* modeltime = scenario->getModeltime();
        const double CVRT90 = 2.212; // 1975 $ to 1990 $
        for( int per = 0; per < modeltime->getmaxper(); ++per ){
            sectorprice[ per ] = calPrice / CVRT90;
        }
    }

    if( mMarketName.empty() ){
        ILogger& mainLog = ILogger::getLogger( "main_log" );
        mainLog.setLevel( ILogger::WARNING );
        mainLog << "Market name for sector " << name << " was not set. Defaulting to regional market." << endl;
        mMarketName = regionName;
    }
    SupplySector::completeInit( aRegionInfo, aDependencyFinder, aLandAllocator );
}

/*! \brief Get the XML node name for output to XML.
*
* This public function accesses the private constant string, XML_NAME.
* This way the tag is always consistent for both read-in and output and can be easily changed.
* This function may be virtual to be overriden by derived class pointers.
* \author Josh Lurz, James Blackwood
* \return The constant XML_NAME.
*/
const string& FoodSupplySector::getXMLName() const {
	return getXMLNameStatic();
}

/*! \brief Get the XML node name in static form for comparison when parsing XML.
*
* This public function accesses the private constant string, XML_NAME.
* This way the tag is always consistent for both read-in and output and can be easily changed.
* The "==" operator that is used when parsing, required this second function to return static.
* \note A function cannot be static and virtual.
* \author Josh Lurz, James Blackwood
* \return The constant XML_NAME as a static.
*/
const string& FoodSupplySector::getXMLNameStatic() {
	const static string XML_NAME = "FoodSupplySector";
	return XML_NAME;
}

//! Create markets
void FoodSupplySector::setMarket() {
	Marketplace* marketplace = scenario->getMarketplace();
	const Modeltime* modeltime = scenario->getModeltime();
	const int maxper = modeltime->getmaxper();
	const double CVRT90 = 2.212; // 1975 $ to 1990 $
	// name is resource name
	if ( marketplace->createMarket( regionName, mMarketName, name, IMarketType::NORMAL ) ) {
		vector <double> tempCalPrice( maxper, calPrice / CVRT90 ); // market prices are in $1975
		marketplace->setPriceVector( name, regionName, tempCalPrice );
		for( int per = 1; per < modeltime->getmaxper(); ++per ){
			marketplace->setMarketToSolve( name, regionName, per );
		}
		for( int per = 0; per < modeltime->getmaxper(); ++per ){
			marketplace->getMarketInfo( name, regionName, per, true )->setDouble( "calPrice", calPrice );
		}
	}
}