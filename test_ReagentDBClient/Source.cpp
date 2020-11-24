#include "gtest/gtest.h"
#include "ReagentDBClient.h"
#include <Windows.h>
#include <iostream>
#include <future>         // std::async, std::future
#include <thread> 

//const std::string SERVER = "http://localhost:8000";
const std::string SERVER = "http://xiaomingdesktops/";
/**
 Note: In order to run these tests on the server, it is best to run
 the server in as a test server, where it will create it's own mini
 pre-populated database. Useage:
		 python manage.py testserver path/to/fixture.json
 Then it is safe to run the tests created here
 */

std::string path_to_PA_fixture;
std::string path_to_PA_delta_fixture;

int main(int argc, char **argv)
{
	std::vector<std::string> paths;
	paths.push_back("reagents");
	paths.push_back("api");
	paths.push_back("pa");
	paths.push_back("Rb Gene");
	ReagentDBClient rdbClient = ReagentDBClient(SERVER);
	std::wstring alias = U("¦Â-Tubulin-¢ó");

	//njson ret = rdbClient.GetPAByAlias(alias);
	//std::cout << ret.dump();

	json::value v = rdbClient.JGetGeneric(paths);

	std::wcout << v.serialize();


	return 0;
	// expect input file of database fixtures
	for (int i = 1; i < argc; ++i) {
		printf("arg %2d = %s\n", i, argv[i]);
	}

	if (argc > 1) {
		path_to_PA_fixture = argv[1];
		path_to_PA_delta_fixture = argv[2];
	}
	else {
		std::cout << "test_ReagentDBClient [pa_fixture] [pa_delta_fixture]";
		return 0;
	}

	return RUN_ALL_TESTS();
}

njson getFromDB() {
	ReagentDBClient rdbClient = ReagentDBClient(SERVER);
	return rdbClient.GetPAList();
}

void getFromDB_ptr(njson * ptr) {
	ReagentDBClient rdbClient = ReagentDBClient(SERVER);
	*ptr = rdbClient.GetPAList();
	return;
}

TEST(SyncTest, CheckForPAUpdates) {
	// check if there are any updates for the client
	// settings.ini records latest timestamp sync
	ReagentDBClient rdbClient = ReagentDBClient(SERVER);
	njson client_data;
	client_data["autostainer_sn"] = "SN12345";
	client_data["last_sync"] = "200909T152923";		// 2020-09-09T15:29:23
	
	std::vector<std::string> paths;
	paths.push_back("reagents");
	paths.push_back("api");
	paths.push_back("pa");
	paths.push_back("database_to_client_sync");

	njson updates = rdbClient.PostGeneric(paths, client_data);

	std::cout << updates.dump() << std::endl;
}

TEST(SyncTest, SendChangeLog) {
	// 
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
	std::cout << missing_data.dump() << std::endl;
}


TEST(GenericsTest, SendDeltaTable) {
	// send json with changes to server
	// server implements
	ReagentDBClient rdbClient = ReagentDBClient(SERVER);
	std::ifstream fixture(path_to_PA_delta_fixture);
	njson fixture_data;
	fixture >> fixture_data;
	fixture.close();

	std::vector<std::string> paths;
	paths.push_back("reagents");
	paths.push_back("api");
	paths.push_back("pa");
	paths.push_back("sync_request");

	for (auto data : fixture_data) {
		data["fields"]["autostainer_sn"] = "SN12345";
		rdbClient.PostGeneric(paths, data["fields"]);
		// TODO: add assertions lol
	}
}


TEST(GenericsTest, RequestDeltaTable) {
	ReagentDBClient rdbClient = ReagentDBClient(SERVER);
	std::vector<std::string> paths;
	paths.push_back("reagents");
	paths.push_back("api");
	paths.push_back("pa");
	paths.push_back("recieve_sync");

	njson req;
	req["autostainer_sn"] = "SN12346";
	req["last_sync"] = "2020-08-12T15:09:29Z";

	njson data = rdbClient.PostGeneric(paths, req);

	std::cout << data.dump() << std::endl;
}


TEST(HelperTests, CheckKeysExist) {
	ReagentDBClient rdbClient = ReagentDBClient(SERVER);
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

TEST(DebugTest, TryAsync) {
	std::ifstream fixture(path_to_PA_fixture);
	njson fixture_data;
	fixture >> fixture_data;
	fixture.close();

	std::future<njson> fut = std::async(getFromDB);
	njson data = fut.get();

	ASSERT_EQ(data.size(), fixture_data.size());
}

TEST(DebugTest, TryThread) {
	std::ifstream fixture(path_to_PA_fixture);
	njson fixture_data;
	fixture >> fixture_data;
	fixture.close();

	njson data;
	std::thread t(getFromDB_ptr, &data);
	t.join();
	ASSERT_EQ(data.size(), fixture_data.size());
}

// TODO: finish this test
TEST(PATests, ReturnsListOfPAs) {
	// read from file the fixture
	std::ifstream fixture(path_to_PA_fixture);
	njson fixture_data;
	fixture >> fixture_data;
	fixture.close();

	ReagentDBClient rdbClient = ReagentDBClient(SERVER);
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
		EXPECT_EQ(fixture_data[i]["fields"]["date"], d["date"]);
		EXPECT_EQ(fixture_data[i]["fields"]["is_factory"], d["is_factory"]);
		++i;
	}
}

TEST(PATests, ReturnSinglePA) {
	ReagentDBClient rdbClient = ReagentDBClient(SERVER);

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
}

TEST(PATests, UpdateSinglePA) {
	std::string alias = "CYN2";
	std::string cat = "CYN-1014";
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
	EXPECT_EQ(retData["description"], description);
}

TEST(PATests, UpdateSinglePACatalogNumber) {
	std::string alias = "CYN2";
	std::string newCat = "CYN-9999";
	std::string cat = "CYN-1014";
	int incub = 20;
	std::string ar = "High PH";
	std::string description = "PA from test_ReagentDBClient";

	njson data = {
		{"alias", alias},
		{"catalog", newCat},
		{"incub", incub},
		{"ar", ar},
		{"description", description}
	};

	ReagentDBClient rdbClient = ReagentDBClient(SERVER);
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
}

TEST(PATests, DeleteSinglePA) {
	std::string cat = "CYN-9999";

	ReagentDBClient rdbClient = ReagentDBClient(SERVER);
	int statusCode = rdbClient.DeletePA(cat);

	EXPECT_EQ(statusCode, 204);

	// try looking for it
	njson retData = rdbClient.GetPAList(cat);
	EXPECT_TRUE(rdbClient.keyExists(retData, "_error"));
	EXPECT_EQ(retData["_error"], "404");
}