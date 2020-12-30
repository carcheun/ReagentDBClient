#include "gtest/gtest.h"
#include "ReagentDBClient.h"
#include <Windows.h>
#include <iostream>
#include <future>         // std::async, std::future
#include <thread> 

std::string SERVER = "http://localhost:8000/";
/**
 Note: In order to run these tests on the server, it is best to run
 the server in as a test server, where it will create it's own mini
 pre-populated database. Useage:
		 python manage.py testserver path/to/fixture/folder
 Then it is safe to run the tests created here
 */

std::string path_to_fixtures;
int main(int argc, char **argv)
{
	::testing::InitGoogleTest();
	// expect input file of database fixtures
	for (int i = 1; i < argc; ++i) {
		printf("arg %2d = %s\n", i, argv[i]);
	}

	if (argc > 1) {
		path_to_fixtures = argv[2];
		SERVER = argv[1];
	}
	else {
		std::cout << "test_ReagentDBClient [server] [path_to_fixtures_folder]";
		return 0;
	}

	return RUN_ALL_TESTS();
}

TEST(PATests, ReturnsListOfPAs) {
	// read from file the fixture
	njson fixture_data;
	std::ifstream fixture(path_to_fixtures + "\\test_pa.json");
	fixture >> fixture_data;
	fixture.close();

	ReagentDBClient rdbClient = ReagentDBClient(SERVER);
	njson data = rdbClient.GetPAList();

	ASSERT_EQ(data.size(), fixture_data.size());

	for (int i = 0; i < data.size(); i++) {
		int ncnt = 0;
		for (auto d : data) {
			if (fixture_data[i]["fields"]["catalog"] == d["catalog"]) {
				EXPECT_EQ(fixture_data[i]["fields"]["fullname"], d["fullname"]);
				EXPECT_EQ(fixture_data[i]["fields"]["source"], d["source"]);
				EXPECT_EQ(fixture_data[i]["fields"]["catalog"], d["catalog"]);
				EXPECT_EQ(fixture_data[i]["fields"]["volume"], d["volume"]);
				EXPECT_EQ(fixture_data[i]["fields"]["incub"], d["incub"]);
				EXPECT_EQ(fixture_data[i]["fields"]["ar"], d["ar"]);
				EXPECT_EQ(fixture_data[i]["fields"]["description"], d["description"]);
				EXPECT_EQ(fixture_data[i]["fields"]["date"], d["date"]);
				EXPECT_EQ(fixture_data[i]["fields"]["is_factory"], d["is_factory"]);
				data.erase(ncnt);
				break;
			}
			ncnt++;
		}
	}
}

TEST(PATests, ReturnSinglePA) {
	ReagentDBClient rdbClient = ReagentDBClient(SERVER);
	njson fixture_data;
	std::ifstream fixture(path_to_fixtures + "\\test_pa.json");
	fixture >> fixture_data;
	fixture.close();

	std::string catalog = fixture_data[0]["fields"]["catalog"];
	njson data = rdbClient.GetPAList(catalog);

	// expect single item and full name to correspond to thing
	EXPECT_EQ(data["fullname"], fixture_data[0]["fields"]["fullname"]);
	EXPECT_EQ(data["source"], fixture_data[0]["fields"]["source"]);
	EXPECT_EQ(data["catalog"], fixture_data[0]["fields"]["catalog"]);
	EXPECT_EQ(data["volume"], fixture_data[0]["fields"]["volume"]);
	EXPECT_EQ(data["incub"], fixture_data[0]["fields"]["incub"]);
	EXPECT_EQ(data["ar"], fixture_data[0]["fields"]["ar"]);
	EXPECT_EQ(data["description"], fixture_data[0]["fields"]["description"]);
	EXPECT_EQ(data["is_factory"], fixture_data[0]["fields"]["is_factory"]);
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

	ReagentDBClient rdbClient = ReagentDBClient(SERVER);
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

	// delete what was added
	rdbClient.DeletePA(cat);
}

TEST(PATests, UpdateSinglePA) {
	std::string alias = "CYN2";
	std::string cat = "PA003";
	int incub = 20;
	std::string ar = "High PH";
	std::string description = "PA from test_ReagentDBClient";

	njson data = {
		{"alias", alias},
		{"catalog", cat},
		{"incub", incub},
		{"ar", ar},
		{"description", description}
	};

	ReagentDBClient rdbClient = ReagentDBClient(SERVER);
	rdbClient.PutPA(data, cat);
	njson retData = rdbClient.GetPAList(cat);

	// make sure _error key does not exists
	ASSERT_FALSE(rdbClient.keyExists(retData, "_error"));

	// after data is added check to see if it matches data returned
	EXPECT_EQ(retData["alias"], alias);
	EXPECT_EQ(retData["catalog"], cat);
	EXPECT_EQ(retData["incub"], incub);
	EXPECT_EQ(retData["ar"], ar);
	EXPECT_EQ(retData["description"], description);

	data = {
		{"alias", "PA3"},
		{"catalog", cat},
		{"incub", 30},
		{"ar", "High PH"},
		{"description", ""}
	};
	// change it back so test is independent
	rdbClient.PutPA(data, cat);
}

TEST(PATests, DeleteSinglePA) {
	// add the PA then delete it
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

	ReagentDBClient rdbClient = ReagentDBClient(SERVER);
	njson retData = rdbClient.AddPA(data);

	njson ret = rdbClient.DeletePA(cat);
	EXPECT_EQ(ret["status_code"], 204);
	retData = rdbClient.GetPAList(cat);
	EXPECT_TRUE(rdbClient.keyExists(retData, "_error"));
	EXPECT_EQ(retData["_error"], "404");
}


TEST(SyncTest, CheckForPAUpdates) {
	// check if there are any updates for the client
	// settings.ini records latest timestamp sync
	ReagentDBClient rdbClient = ReagentDBClient(SERVER);
	njson client_data;
	client_data["autostainer_sn"] = "autostainer2";
	client_data["last_sync"] = "2020-12-02T11:53:04-0700";
	std::vector<std::string> paths;
	paths.push_back("reagents");
	paths.push_back("api");
	paths.push_back("pa");
	paths.push_back("database_to_client_sync");

	njson updates = rdbClient.PostGeneric(paths, client_data);
	// so at least 1 delta will be missing as it is too old
	ASSERT_EQ(updates.size(), updates.size()-1);
}

TEST(SyncTest, SendChangeLog) {
	// some data to send
	ReagentDBClient rdbClient = ReagentDBClient(SERVER);
	njson data;
	// create
	data = {
		{"alias", "pa change add"},
		{"autostainer_sn", "autostainer2"},
		{"fullname", "test PA create"},
		{"catalog", "PA_DELTA01"},
		{"incub", 15},
		{"volume", 3000},
		{"ar", "NO"},
		{"is_factory", true},
		{"date", "2020-12-02T21:36:58-08:00"},
		{"operation", "CREATE"}
	};
	rdbClient.ClientToDatabaseSync(data, "PA");
	// edit
	data = {
		{"catalog", "PA002"},
		{"fullname", "test PA 2 updated"},
		{"autostainer_sn", "autostainer2"},
		{"alias", "PA 2" },
		{"source", "admin"},
		{"volume", 3000},
		{"incub", 45},
		{"ar", "High PH"},
		{"description", ""},
		{"is_factory", true},
		{"date", "2020-12-02T21:36:58-08:00"},
		{"operation", "UPDATE" }
	};
	rdbClient.ClientToDatabaseSync(data, "PA");

	njson ret = rdbClient.GetPAList();
	EXPECT_EQ(ret.size(), 4);
	bool PA002 = FALSE;
	bool PA_DELTA01 = FALSE;
	bool DELETED_PA = TRUE;
	for (auto d : ret) {
		if (d["catalog"] == "PA002") {
			EXPECT_EQ(d["fullname"], (std::string)"test PA 2 updated");
			EXPECT_EQ(d["ar"], (std::string)"High PH");
			EXPECT_EQ(d["alias"], (std::string)"PA 2");
			PA002 = TRUE;
		}
		else if (d["catalog"] == "PA_DELTA01") {
			PA_DELTA01 = TRUE;
			EXPECT_EQ(d["volume"], 3000);
		}
	}

	// delete
	data = {
		{"catalog", "PA_DELTA01"},
		{"date", "2020-12-31T21:36:58-08:00"},
		{"autostainer_sn", "autostainer2"},
		{"operation", "DELETE" }
	};
	rdbClient.ClientToDatabaseSync(data, "PA");
	ret = rdbClient.GetPAList();
	for (auto d : ret) {
		if (d["catalog"] == "PA_DELTA01") {
			DELETED_PA = FALSE;
		}
	}

	EXPECT_TRUE(PA002);
	EXPECT_TRUE(PA_DELTA01);
	EXPECT_TRUE(DELETED_PA);

	// old create
	data = {
		{"alias", "pa change create"},
		{"autostainer_sn", "autostainer2"},
		{"fullname", "test PA create newer, failed"},
		{"catalog", "PA001"},
		{"incub", 15},
		{"volume", 2000},
		{"ar", "High PH"},
		{"is_factory", true},
		{"date", "2020-12-29T21:36:58-08:00"},
		{"operation", "CREATE"}
	};
	rdbClient.ClientToDatabaseSync(data, "PA");
	// edit
	data = {
		{"catalog", "PA002"},
		{"fullname", "test PA 2 updated failed"},
		{"autostainer_sn", "autostainer2"},
		{"alias", "PA 2" },
		{"source", "failed update"},
		{"volume", 5000},
		{"incub", 45},
		{"ar", "Low PH"},
		{"description", ""},
		{"is_factory", true},
		{"date", "2020-12-00T21:36:58-08:00"},
		{"operation", "UPDATE" }
	};
	rdbClient.ClientToDatabaseSync(data, "PA");

	EXPECT_EQ(ret.size(), 4);
	bool CREATE_PA = FALSE;
	PA002 = FALSE;
	for (auto d : ret) {
		if (d["catalog"] == "PA001") {
			EXPECT_NE(d["fullname"], (std::string)"test PA create newer, failed");
			EXPECT_NE(d["ar"], (std::string)"High PH");
			EXPECT_NE(d["alias"], (std::string)"pa change create");
			CREATE_PA = TRUE;
		}
		else if (d["catalog"] == "PA002") {
			EXPECT_EQ(d["volume"], 3000);
			EXPECT_NE(d["fullname"], (std::string)"test PA 2 updated failed");
			EXPECT_NE(d["ar"], (std::string)"Low PH");
			EXPECT_EQ(d["source"], (std::string)"admin");
			PA002 = TRUE;
		}
	}
	EXPECT_TRUE(CREATE_PA);
	EXPECT_TRUE(PA002);

	// change PA002 back
	data = {
		{"catalog", "PA002"},
		{"fullname", "test PA 2"},
		{"autostainer_sn", "autostainer2"},
		{"alias", "PA 2" },
		{"source", "admin"},
		{"volume", 3000},
		{"incub", 45},
		{"ar", "Low PH"},
		{"description", ""},
		{"is_factory", true},
		{"date", "2020-12-29T21:36:58-08:00"},
		{"operation", "UPDATE" }
	};
	rdbClient.ClientToDatabaseSync(data, "PA");
}

TEST(GenericsTest, SendInitialSync) {
	ReagentDBClient rdbClient = ReagentDBClient(SERVER);
	njson data_arr = njson::array();

	for (int i = 0; i < 5; i++) {
		njson data;
		data["fullname"] = "hello" + std::to_string(i);
		data["source"] = "sauce" + std::to_string(i);
		data["catalog"] = "CYN-000" + std::to_string(i);
		data["alias"] = "my alias";
		data["volume"] = 1000;
		data["incub"] = 20;
		data["ar"] = "NO";
		data["description"] = "hello" + std::to_string(i);
		data["date"] = "200918T1529";
		data["factory"] = 0;
		data_arr.push_back(data);
	}

	std::vector<std::string> paths;
	paths.push_back("reagents");
	paths.push_back("api");
	paths.push_back("pa");
	paths.push_back("initial_sync");

	njson missing_data = rdbClient.PostGeneric(paths, data_arr);
	//std::cout << missing_data.dump() << std::endl;
	// TODO: finish test
}

