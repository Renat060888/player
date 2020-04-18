#ifndef CONFIG_READER_H
#define CONFIG_READER_H

#include <microservice_common/system/a_config_reader.h>

class ConfigReader : public AConfigReader
{
public:
    struct SConfigParameters {
        AConfigReader::SConfigParameters baseParams;
    };

    static ConfigReader & singleton(){
        static ConfigReader instance;
        return instance;
    }

    const SConfigParameters & get(){ return m_parameters; }


private:
    ConfigReader();
    ~ConfigReader(){}

    ConfigReader( const ConfigReader & _inst ) = delete;
    ConfigReader & operator=( const ConfigReader & _inst ) = delete;

    virtual bool initDerive( const SIninSettings & _settings ) override;
    virtual bool parse( const std::string & _filePath ) override;
    virtual bool createCommandsFromConfig( const std::string & _content ) override;
    virtual std::string getConfigExampleDerive() override;


    // data
    SConfigParameters m_parameters;
};
#define CONFIG_READER ConfigReader::singleton()
#define CONFIG_PARAMS ConfigReader::singleton().get()

#endif // CONFIG_READER_H
