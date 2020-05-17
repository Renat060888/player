#ifndef DATASOURCE_READER_TEST_H
#define DATASOURCE_READER_TEST_H

#include <gtest/gtest.h>
#include <microservice_common/storage/database_manager_base.h>

class TestDatasourceReader : public ::testing::Test
{
public:
    TestDatasourceReader();


protected:
    static void SetUpTestCase();
    static void TearDownTestCase();

    static DatabaseManagerBase * m_database;
};

#endif // DATASOURCE_READER_TEST_H
