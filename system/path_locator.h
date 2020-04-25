#ifndef PATH_LOCATOR_H
#define PATH_LOCATOR_H

#include <string>

class PathLocator
{
public:
    static PathLocator & singleton(){
        static PathLocator instance;
        return instance;
    }

    void removePreviousSessionSocketFiles();

    const std::string getUniqueLockFile();
    const std::string getShellImitationDomainSocket();


private:
    PathLocator();
    ~PathLocator();

    PathLocator( const PathLocator & _inst ) = delete;
    PathLocator & operator=( const PathLocator & _inst ) = delete;


};
#define PATH_LOCATOR PathLocator::singleton()

#endif // PATH_LOCATOR_H
