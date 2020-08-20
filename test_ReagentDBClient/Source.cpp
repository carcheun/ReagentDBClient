#include "gtest/gtest.h"
#include "ReagentDBClient.h"
#include <iostream>

const std::string SERVER = "http://localhost:8000";
const int PORT = 8000;

/**
 Note: In order to run these tests on the server, it is best to run
 the server in as a test server, where it will create it's own mini
 pre-populated database. Useage:
		 python manage.py testserver path/to/fixture.json
 Then it is safe to run the tests created here
 */

std::string path_to_PA_fixture;

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);

	// expect input file of database fixtures
	for (int i = 1; i < argc; ++i) {
		printf("arg %2d = %s\n", i, argv[i]);
	}

	if (argc > 1) {
		path_to_PA_fixture = argv[1];
	}

	return RUN_ALL_TESTS();
}

TEST(HelperTests, CheckKeysExist) {
	ReagentDBClient rdbClient = ReagentDBClient(SERVER, PORT);
	njson data = {
		{"MY_KEY_1", 1234},
		{"MY_KEY_2", "1234"}
	};

	ASSERT_TRUE(rdbClient.keyExists(data, "MY_KEY_1"));
	ASSERT_TRUE(rdbClient.keyExists(data, "MY_KEY_2"));
	ASSERT_FALSE(rdbClient.keyExists(data, "MY_KEY_3"));

	ASSERT_EQ(data["MY_KEY_1"], 1234);
	ASSERT_EQ(data["MY_KEY_2"], "1234");
}

// TODO: finish this test
TEST(PATests, ReturnsListOfPAs) {
	// read from file the fixture
	std::ifstream fixture(path_to_PA_fixture);
	njson fixture_data;
	fixture >> fixture_data;
	fixture.close();

	ReagentDBClient rdbClient = ReagentDBClient(SERVER, PORT);
	njson data = rdbClient.GetPAList();

	ASSERT_EQ(data.size(), fixture_data.size());

	int i = 0;
	for (auto d : data) {
		// mass ez compare
		//EXPECT_EQ(fixture_data[i]["fields"].dump(), d.dump());
		EXPECT_EQ(fixture_data[i]["fields"]["fullname"], d["fullname"]);
		EXPECT_EQ(fixture_data[i]["fields"]["source"], d["source"]);
		EXPECT_EQ(fixture_data[i]["fields"]["catalog"], d["catalog"]);
		EXPECT_EQ(fixture_data[i]["fields"]["volume"], d["volume"]);
		EXPECT_EQ(fixture_data[i]["fields"]["incub"], d["incub"]);
		EXPECT_EQ(fixture_data[i]["fields"]["ar"], d["ar"]);
		EXPECT_EQ(fixture_data[i]["fields"]["description"], d["description"]);
		EXPECT_EQ(fixture_data[i]["fields"]["factory"], d["factory"]);
		EXPECT_EQ(fixture_data[i]["fields"]["date"], d["date"]);
		EXPECT_EQ(fixture_data[i]["fields"]["time"], d["time"]);
		EXPECT_EQ(fixture_data[i]["fields"]["is_factory"], d["is_factory"]);
		++i;
	}
}

TEST(PATests, ReturnSinglePA) {
	ReagentDBClient rdbClient = ReagentDBClient(SERVER, PORT);

	std::string catalog = "MAB-0662";
	njson data = rdbClient.GetPAList(catalog);

	// expect single item and full name to correspond to thing
	EXPECT_EQ(data["fullname"], "IDH1 R132H");
	EXPECT_EQ(data["source"], "pa_fixture.json");
	EXPECT_EQ(data["catalog"], "MAB-0662");
	EXPECT_EQ(data["volume"], 5000);
	EXPECT_EQ(data["incub"], 60);
	EXPECT_EQ(data["ar"], "Low PH");
	EXPECT_EQ(data["description"], "Dummy Data");
	EXPECT_EQ(data["factory"], 1);
	EXPECT_EQ(data["is_factory"], true);
}

TEST(PATests, AddSinglePA) {
	std::string alias = "CYN";
	std::string cat = "CYN-1014";
	int incub = 60;
	std::string ar = "Low PH";
	int factory = 1;

	njson data = {
		{"alias", alias},
		{"catalog", cat},
		{"incub", incub},
		{"ar", ar},
		{"factory", factory}
	};

	ReagentDBClient rdbClient = ReagentDBClient(SERVER, PORT);
	njson retData = rdbClient.AddPA(data);

	// make sure _error key does not exists
	ASSERT_FALSE(rdbClient.keyExists(retData, "_error"))
		<< "=====" << std::endl
		<< "Error from ReagentDBClient::AddPA()" << std::endl
		<< "_error: " << retData["_error"] << std::endl
		<< "=====";

	// after data is added check to see if it matches data returned
	EXPECT_EQ(retData["alias"], alias);
	EXPECT_EQ(retData["catalog"], cat);
	EXPECT_EQ(retData["incub"], incub);
	EXPECT_EQ(retData["ar"], ar);
	EXPECT_EQ(retData["incub"], 60);
	EXPECT_EQ(retData["factory"], factory);
}

TEST(PATests, UpdateSinglePA) {
	std::string alias = "CYN2";
	std::string cat = "CYN-1014";
	int incub = 20;
	std::string ar = "High PH";
	int factory = 1;
	std::string description = "PA from test_ReagentDBClient";

	njson data = {
		{"alias", alias},
		{"catalog", cat},
		{"incub", incub},
		{"ar", ar},
		{"factory", factory},
		{"description", description}
	};

	ReagentDBClient rdbClient = ReagentDBClient(SERVER, PORT);
	rdbClient.PutPA(data, cat);
	njson retData = rdbClient.GetPAList(cat);

	// make sure _error key does not exists
	ASSERT_FALSE(rdbClient.keyExists(retData, "_error"))
		<< "=====" << std::endl
		<< "Error from ReagentDBClient::PutPA()" << std::endl
		<< "_error: " << retData["_error"] << std::endl
		<< "=====";

	// after data is added check to see if it matches data returned
	EXPECT_EQ(retData["alias"], alias);
	EXPECT_EQ(retData["catalog"], cat);
	EXPECT_EQ(retData["incub"], incub);
	EXPECT_EQ(retData["ar"], ar);
	EXPECT_EQ(retData["factory"], factory);
	EXPECT_EQ(retData["description"], description);
}

TEST(PATests, UpdateSinglePACatalogNumber) {
	std::string alias = "CYN2";
	std::string newCat = "CYN-9999";
	std::string cat = "CYN-1014";
	int incub = 20;
	std::string ar = "High PH";
	int factory = 1;
	std::string description = "PA from test_ReagentDBClient";

	njson data = {
		{"alias", alias},
		{"catalog", newCat},
		{"incub", incub},
		{"ar", ar},
		{"factory", factory},
		{"description", description}
	};

	ReagentDBClient rdbClient = ReagentDBClient(SERVER, PORT);
	rdbClient.PutPA(data, cat);

	njson retData = rdbClient.GetPAList(newCat);

	// make sure _error key does not exists
	ASSERT_FALSE(rdbClient.keyExists(retData, "_error"))
		<< "=====" << std::endl
		<< "Error from ReagentDBClient::PutPA()" << std::endl
		<< "_error: " << retData["_error"] << std::endl
		<< "=====";

	// after data is added check to see if it matches data returned
	EXPECT_EQ(retData["alias"], alias);
	EXPECT_EQ(retData["catalog"], newCat);
	EXPECT_EQ(retData["incub"], incub);
	EXPECT_EQ(retData["ar"], ar);
	EXPECT_EQ(retData["factory"], factory);
}

TEST(PATests, DeleteSinglePA) {
	std::string cat = "CYN-9999";

	ReagentDBClient rdbClient = ReagentDBClient(SERVER, PORT);
	int statusCode = rdbClient.DeletePA(cat);

	EXPECT_EQ(statusCode, 204);

	// try looking for it
	njson retData = rdbClient.GetPAList(cat);
	EXPECT_TRUE(rdbClient.keyExists(retData, "_error"));
	EXPECT_EQ(retData["_error"], "404");
}