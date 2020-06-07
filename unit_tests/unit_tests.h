#ifndef UNIT_TESTS_H
#define UNIT_TESTS_H


class UnitTests
{
public:
    UnitTests( int _argc, char ** _argv );

    bool run();

private:
    void insertTestData();


};

#endif // UNIT_TESTS_H
