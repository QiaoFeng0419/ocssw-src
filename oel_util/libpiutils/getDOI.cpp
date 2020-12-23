#include <productInfo.h>

#include <stdlib.h>
#include <stdio.h>
#include <genutils.h>
#include <string>
#include <ctype.h>
#include <algorithm>

#include <pugixml.hpp>

using namespace std;
using namespace pugi;


extern "C" const char* getDOI(const char* platform, const char* sensor,
        const char* level, const char* suite, const char* version) {

    static string doiStr;

    xml_document rootNode;
    xml_node dacNode;
    string dacStr;
        
    char *dataRoot;
    if ((dataRoot = getenv("OCDATAROOT")) == NULL) {
        printf("-E- OCDATAROOT environment variable is not defined.\n");
        exit(EXIT_FAILURE);
    }
    string doiXMLFileName = (string) dataRoot + "/common/doi.xml";

    xml_parse_result xmlResult = rootNode.load_file(doiXMLFileName.c_str());

    if (!xmlResult) {
        printf("-E- %s Line %d: Can not load %s.  %s\n", __FILE__, __LINE__,
                doiXMLFileName.c_str(), xmlResult.description());
        exit(EXIT_FAILURE);
    }

    dacNode = rootNode.child("EID");
    if (!dacNode) {
        printf("-E- %s Line %d: could not find EID tag in XML file = %s\n",
                __FILE__, __LINE__, doiXMLFileName.c_str());
        exit(EXIT_FAILURE);
    }

    dacStr = dacNode.attribute("name").value();
   
    string platformStr = platform;
    transform(platformStr.begin(), platformStr.end(), platformStr.begin(), ::toupper);

    // OK time to add the exceptions
    if (platformStr == "ADEOS")
        platformStr = "ADEOS-I";
    else if (platformStr == "SUOMI-NPP")
        platformStr = "NPP";

    // search for the requested platform name
    xml_node platformNode = dacNode.find_child_by_attribute("platform", "name", platformStr.c_str());
    if (!platformNode) // see if platform name is found
        return NULL;

    // fix sensor name
    string sensorStr = sensor;
    transform(sensorStr.begin(), sensorStr.end(), sensorStr.begin(), ::toupper);

    // search for the requested sensor name
    xml_node sensorNode = platformNode.find_child_by_attribute("sensor", "name", sensorStr.c_str());
    if (!sensorNode)
        return NULL;

    // fix the level name    
    string levelStr = level;
    transform(levelStr.begin(), levelStr.end(), levelStr.begin(), ::toupper);

    // search for the requested sensor name
    xml_node levelNode = sensorNode.find_child_by_attribute("level", "name", levelStr.c_str());
    if (!levelNode)
        return NULL;

    // fix the suite name
    string suiteStr = suite;
    transform(suiteStr.begin(), suiteStr.end(), suiteStr.begin(), ::toupper);

    // search for the requested sensor name
    xml_node suiteNode = levelNode.find_child_by_attribute("suite", "name", suiteStr.c_str());
    if (!suiteNode)
        return NULL;

    // fix the version name
    string versionStr = version;
    size_t pos = versionStr.find_first_of('.');
    if (pos != string::npos)
        versionStr.erase(pos);

    // search for the requested version name
    xml_node versionNode = suiteNode.find_child_by_attribute("version", "name", versionStr.c_str());
    if (!versionNode)
        return NULL;

    // see if there is an alias
    xml_node aliasNode = levelNode.child("alias");
    if (aliasNode) {
        doiStr = aliasNode.child_value();
        // left trim
        doiStr.erase(doiStr.begin(), find_if(doiStr.begin(), doiStr.end(),
                not1(ptr_fun<int, int>(std::isspace))));
        // right trim
        doiStr.erase(find_if(doiStr.rbegin(), doiStr.rend(),
                not1(ptr_fun<int, int>(std::isspace))).base(), doiStr.end());
    } else {
        doiStr = dacStr + "/" + platformStr + "/" + sensorStr + "/" + levelStr +
                "/" + suiteStr + "/" + versionStr;
    }

    return doiStr.c_str();
}
