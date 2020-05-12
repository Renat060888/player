#ifndef DATASOURCE_READER_TEST_H
#define DATASOURCE_READER_TEST_H

#include <gtest/gtest.h>
#include <microservice_common/storage/database_manager_base.h>

class DatasourceReaderTest : public ::testing::Test
{
public:
    DatasourceReaderTest();


protected:
    static void SetUpTestCase();
    static void TearDownTestCase();

    static DatabaseManagerBase * m_database;
};

#endif // DATASOURCE_READER_TEST_H
