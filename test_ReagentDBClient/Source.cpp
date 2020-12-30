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
	rdbClient.CUDRequest("PA", methods::PUT, cat, data);
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
	rdbClient.CUDRequest("PA", methods::PUT, cat, data);
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
	// check them, they should be missing one entry dated "2020-10-28T16:24:00-08:00",
	bool missing_entry = TRUE;
	for (auto d : updates) {
		if (d["date"] == "2020-10-28T16:24:00-08:00") {
			missing_entry = FALSE;
		}
	}
	ASSERT_TRUE(missing_entry);
}

TEST(SyncTest, CheckForReagentsUpdates) {
	ReagentDBClient rdbClient = ReagentDBClient(SERVER);
	njson client_data;
	client_data["autostainer_sn"] = "autostainer2";
	client_data["last_sync_reagent"] = "2020-12-02T11:53:04-0700";
	std::vector<std::string> paths;
	paths.push_back("reagents");
	paths.push_back("api");
	paths.push_back("reagent");
	paths.push_back("database_to_client_sync");

	njson updates = rdbClient.PostGeneric(paths, client_data);

	bool missing_entry = TRUE;
	for (auto d : updates) {
		if (d["date"] == "2020-10-28T16:24:00-08:00") {
			missing_entry = FALSE;
		}
	}
	ASSERT_TRUE(missing_entry);
}

TEST(SyncTest, SendChangeLogPA) {
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
	rdbClient.CUDRequest("PA", methods::PUT, "PA002", data);
}

TEST(SyncTest, SendChangeLogReagent) {
	ReagentDBClient rdbClient = ReagentDBClient(SERVER);
	njson data;
	data = {
		{"autostainer_sn", "autostainer2" },
		{"reagent_sn" , "add_delta_reag002"},
		{"reag_name" , "test_reagentDBClient Reagent 1"},
		{"catalog" , "PA002"},
		{"size" , "S"},
		{"log" , "1234"},
		{"vol" , 3000},
		{"vol_cur" , 5000},
		{"sequence" , 0 },
		{"mfg_date" , "2020-03-17"},
		{"exp_date" , "2022-03-19" },
		{"date" , "2021-01-11T13:49:05-08:00"},
		{"factory" , true },
		{"r_type" , "AR"},
		{"operation" , "CREATE"},
		{"executor" , NULL }
	};
	rdbClient.ClientToDatabaseSync(data, "REAGENT");

}

TEST(SyncTest, SendInitialSyncPA) {
	ReagentDBClient rdbClient = ReagentDBClient(SERVER);
	njson data_arr = njson::array();

	for (int i = 0; i < 5; i++) {
		njson data;
		data["fullname"] = "initial_sync_PA_" + std::to_string(i);
		data["source"] = "test_reagentDBClient";
		data["catalog"] = "TEST-000" + std::to_string(i);
		data["alias"] = "alias";
		data["volume"] = 1000 + i;
		data["incub"] = 20;
		data["ar"] = "NO";
		data["description"] = "";
		data["factory"] = 0;
		data["autostainer_sn"] = "autostainer1";
		data["date"] = "2020-12-29T21:36:58";
		data_arr.push_back(data);
	}

	std::vector<std::string> paths;
	paths.push_back("reagents");
	paths.push_back("api");
	paths.push_back("pa");
	paths.push_back("initial_sync");

	njson all_pa = rdbClient.GetPAList();
	njson missing_data = rdbClient.PostGeneric(paths, data_arr);
	// expect that there are 5 more PA in the database
	EXPECT_EQ(missing_data.size() - all_pa.size(), 5);

	// now clean up and delete the the PA's
	for (int i = 0; i < 5; i++) {
		rdbClient.DeletePA("TEST-000" + std::to_string(i));
	}
}

TEST(ReagentTest, AddReagent) {
	njson data;
	data["reag_name"] = "test_reagentDBClient Reagent 1";
	data["reagent_sn"] = "add_reag001";
	data["size"] = "S";
	data["log"] = "1234";
	data["vol_cur"] = 5000;
	data["vol"] = 3000;
	data["sequence"] = 0;
	data["mfg_date"] = "2020-03-17";
	data["exp_date"] = "2022-03-19";
	data["date"] = "2020-12-29T21:49:05Z";
	data["r_type"] = "AR";
	data["factory"] = true;
	data["catalog"] = "PA002";
	data["autostainer_sn"] = "autostainer2";
	data["in_use"] = false;

	ReagentDBClient rdbClient = ReagentDBClient(SERVER);
	rdbClient.CUDRequest("REAGENT", methods::POST, "", data);
	njson get = rdbClient.GetRequest("REAGENT", data["reagent_sn"]);
	EXPECT_NE(get["_error"], "404");
	EXPECT_EQ(get["reag_name"], data["reag_name"]);
	EXPECT_EQ(get["vol_cur"], data["vol_cur"]);

	rdbClient.CUDRequest("REAGENT", methods::DEL, data["reagent_sn"], NULL);
}

TEST(ReagentTest, DeleteReagent) {
	njson data;
	data["reag_name"] = "test_reagentDBClient updated";
	data["reagent_sn"] = "add_reag001";
	data["size"] = "S";
	data["log"] = "12345";
	data["vol_cur"] = 5000;
	data["vol"] = 3000;
	data["sequence"] = 0;
	data["mfg_date"] = "2020-04-15";
	data["exp_date"] = "2025-03-08";
	data["date"] = "2020-12-30T21:49:05Z";
	data["r_type"] = "AR";
	data["factory"] = true;
	data["catalog"] = "PA002";
	data["autostainer_sn"] = "autostainer2";
	data["in_use"] = false;

	ReagentDBClient rdbClient = ReagentDBClient(SERVER);
	rdbClient.CUDRequest("REAGENT", methods::POST, "", data);
	njson get = rdbClient.GetRequest("REAGENT", data["reagent_sn"]);
	EXPECT_NE(get["_error"], "404");

	rdbClient.CUDRequest("REAGENT", methods::DEL, data["reagent_sn"], NULL);
	get = rdbClient.GetRequest("REAGENT", data["reagent_sn"]);
	EXPECT_EQ(get["_error"], "404");
}

TEST(ReagentTest, UpdateReagent) {
	njson data;
	data["reag_name"] = "test_reagentDBClient updated";
	data["reagent_sn"] = "REAG006";
	data["size"] = "S";
	data["log"] = "12345";
	data["vol_cur"] = 5000;
	data["vol"] = 3000;
	data["sequence"] = 0;
	data["mfg_date"] = "2020-04-15";
	data["exp_date"] = "2025-03-08";
	data["date"] = "2020-12-30T21:49:05Z";
	data["r_type"] = "AR";
	data["factory"] = true;
	data["catalog"] = "PA002";
	data["autostainer_sn"] = "autostainer2";
	data["in_use"] = false;

	ReagentDBClient rdbClient = ReagentDBClient(SERVER);
	rdbClient.CUDRequest("REAGENT", methods::PUT, data["reagent_sn"], data);
	njson get = rdbClient.GetRequest("REAGENT", data["reagent_sn"]);
	EXPECT_EQ(get["reag_name"], data["reag_name"]);
	EXPECT_EQ(get["vol_cur"], data["vol_cur"]);
	EXPECT_EQ(get["size"], data["size"]);
	EXPECT_EQ(get["mfg_date"], data["mfg_date"]);
	EXPECT_EQ(get["exp_date"], data["exp_date"]);
	EXPECT_EQ(get["catalog"], data["catalog"]);

	data["reag_name"] = "Test Reagent 6";
	data["reagent_sn"] = "REAG006";
	data["size"] = "M";
	data["log"] = "1234";
	data["vol_cur"] = 120;
	data["vol"] = 3000;
	data["sequence"] = 0;
	data["mfg_date"] = "2019-10-02";
	data["exp_date"] = "2020-12-31";
	data["date"] = "2020-12-28T22:01:37Z";
	data["r_type"] = "AR";
	data["factory"] = true;
	data["catalog"] = "PA003";
	data["autostainer_sn"] = "autostainer2";
	data["in_use"] = false;

	rdbClient.CUDRequest("REAGENT", methods::PUT, data["reagent_sn"], NULL);
}

TEST(ReagentTest, UpdateReagentNonExistant) {
	njson data;
	data["reag_name"] = "test_reagentDBClient updated";
	data["reagent_sn"] = "add_reag001";
	data["size"] = "S";
	data["log"] = "12345";
	data["vol_cur"] = 5000;
	data["vol"] = 3000;
	data["sequence"] = 0;
	data["mfg_date"] = "2020-04-15";
	data["exp_date"] = "2025-03-08";
	data["date"] = "2020-12-30T21:49:05Z";
	data["r_type"] = "AR";
	data["factory"] = true;
	data["catalog"] = "PA002";
	data["autostainer_sn"] = "autostainer2";
	data["in_use"] = false;

	ReagentDBClient rdbClient = ReagentDBClient(SERVER);
	rdbClient.CUDRequest("REAGENT", methods::PUT, data["reagent_sn"], data);
	njson get = rdbClient.GetRequest("REAGENT", data["reagent_sn"]);
	EXPECT_EQ(get["reag_name"], nullptr);
	EXPECT_EQ(get["vol_cur"], nullptr);
	EXPECT_EQ(get["size"], nullptr);
	EXPECT_EQ(get["mfg_date"], nullptr);
	EXPECT_EQ(get["exp_date"], nullptr);
	EXPECT_EQ(get["catalog"], nullptr);
	
	rdbClient.CUDRequest("REAGENT", methods::DEL, data["reagent_sn"], NULL);
}

TEST(ReagentTest, GetReagent) {
	std::string sn = "REAG005";
	ReagentDBClient rdbClient = ReagentDBClient(SERVER);
	njson get = rdbClient.GetRequest("REAGENT", sn);

	EXPECT_EQ(get["reag_name"], "Test Reagent 5");
	EXPECT_EQ(get["vol_cur"], 100);
	EXPECT_EQ(get["size"], "S");
	EXPECT_EQ(get["mfg_date"], "2020-12-01");
	EXPECT_EQ(get["exp_date"], "2030-12-01");
	EXPECT_EQ(get["catalog"], "PA001");
	EXPECT_EQ(get["r_type"], "AR");
}

TEST(ReagentTest, GetReagentList) {
	ReagentDBClient rdbClient = ReagentDBClient(SERVER);
	njson data = rdbClient.GetRequest("REAGENT", "");

	// expecting all the reagents in fixture to return
	njson fixture_data;
	std::ifstream fixture(path_to_fixtures + "\\test_reagent.json");
	fixture >> fixture_data;
	fixture.close();

	ASSERT_EQ(data.size(), fixture_data.size());

	for (int i = 0; i < data.size(); i++) {
		int ncnt = 0;
		for (auto d : data) {
			if (fixture_data[i]["fields"]["reagent_sn"] == d["reagent_sn"]) {
				EXPECT_EQ(fixture_data[i]["fields"]["size"], d["size"]);
				EXPECT_EQ(fixture_data[i]["fields"]["reag_name"], d["reag_name"]);
				EXPECT_EQ(fixture_data[i]["fields"]["catalog"], d["catalog"]);
				EXPECT_EQ(fixture_data[i]["fields"]["log"], d["log"]);
				EXPECT_EQ(fixture_data[i]["fields"]["vol_cur"], d["vol_cur"]);
				EXPECT_EQ(fixture_data[i]["fields"]["vol"], d["vol"]);
				EXPECT_EQ(fixture_data[i]["fields"]["sequence"], d["sequence"]);
				EXPECT_EQ(fixture_data[i]["fields"]["exp_date"], d["exp_date"]);
				EXPECT_EQ(fixture_data[i]["fields"]["mfg_date"], d["mfg_date"]);
				EXPECT_EQ(fixture_data[i]["fields"]["r_type"], d["r_type"]);
				EXPECT_EQ(fixture_data[i]["fields"]["factory"], d["factory"]);
				data.erase(ncnt);
				break;
			}
			ncnt++;
		}
	}
}