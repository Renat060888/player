
#include "config_reader.h"

using namespace std;

ConfigReader::ConfigReader()
{

}


bool ConfigReader::initDerive( const SIninSettings & _settings ){

    m_parameters.baseParams = AConfigReader::m_parameters;
    return true;
}

bool ConfigReader::parse( const std::string & _filePath ){



    return true;
}

bool ConfigReader::createCommandsFromConfig( const std::string & _content ){



    return true;
}

std::string ConfigReader::getConfigExampleDerive(){

}
