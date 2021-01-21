#include "gtest/gtest.h"
#include "ReagentDBClient.h"
#include <Windows.h>
#include <iostream>

std::string SERVER = "http://localhost:8000/";
ReagentDBClient rdb = ReagentDBClient(SERVER);
std::string path_to_fixtures;

/**
 Note: In order to run these tests on the server, it is best to run
 the server in as a test server, where it will create it's own mini
 pre-populated database. Useage:
		 python manage.py testserver path/to/fixture/folder
 Then it is safe to run the tests created here
 */
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

	rdb = ReagentDBClient(SERVER);
	json::value credentials;
	credentials[U("username")] = json::value::string(U("admin"));
	credentials[U("password")] = json::value::string(U("admin"));
	std::vector<std::string> paths;
	paths.push_back("reagents");
	paths.push_back("api-token-auth");
	json::value token = rdb.PostGeneric(paths, credentials);
	if (token.has_field(U("_error"))) {
		return 0;
	}

	rdb.SetAuthorizationToken(utility::conversions::to_utf8string(token[U("token")].as_string()));

	return RUN_ALL_TESTS();
}

TEST(PATests, ReturnsListOfPAs) {
	// read from file the fixture
	njson fixture_data;
	std::ifstream fixture(path_to_fixtures + "\\test_pa.json");
	fixture >> fixture_data;
	fixture.close();

	json::value data = rdb.GetRequest("PA", "");

	ASSERT_EQ(data.size(), fixture_data.size());

	for (int i = 0; i < data.size(); i++) {
		int ncnt = 0;
		for (auto d : data.as_array()) {
			if (utility::conversions::to_utf8string(d[U("catalog")].as_string()).compare(fixture_data[i]["fields"]["catalog"]) == 0) {
				EXPECT_EQ(fixture_data[i]["fields"]["fullname"].get<std::string>(), utility::conversions::to_utf8string(d[U("fullname")].as_string()));
				EXPECT_EQ(fixture_data[i]["fields"]["source"].get<std::string>(), utility::conversions::to_utf8string(d[U("source")].as_string()));
				EXPECT_EQ(fixture_data[i]["fields"]["catalog"].get<std::string>(), utility::conversions::to_utf8string(d[U("catalog")].as_string()));
				EXPECT_EQ(fixture_data[i]["fields"]["volume"].get<int>(), d[U("volume")].as_integer());
				EXPECT_EQ(fixture_data[i]["fields"]["incub"].get<int>(), d[U("incub")].as_integer());
				EXPECT_EQ(fixture_data[i]["fields"]["ar"].get<std::string>(), utility::conversions::to_utf8string(d[U("ar")].as_string()));
				EXPECT_EQ(fixture_data[i]["fields"]["description"].get<std::string>(), utility::conversions::to_utf8string(d[U("description")].as_string()));
				EXPECT_EQ(fixture_data[i]["fields"]["is_factory"].get<bool>(), d[U("is_factory")].as_bool());
				data.erase(ncnt);
				break;
			}
			ncnt++;
		}
	}
}

TEST(PATests, ReturnSinglePA) {
	njson fixture_data;
	std::ifstream fixture(path_to_fixtures + "\\test_pa.json");
	fixture >> fixture_data;
	fixture.close();

	std::string catalog = fixture_data[0]["fields"]["catalog"];
	json::value data = rdb.GetRequest("PA", "");

	// expect single item and full name to correspond to thing
	EXPECT_EQ(utility::conversions::to_utf8string(data[U("fullname")].as_string()), fixture_data[0]["fields"]["fullname"].get<std::string>());
	EXPECT_EQ(utility::conversions::to_utf8string(data[U("source")].as_string()), fixture_data[0]["fields"]["source"].get<std::string>());
	EXPECT_EQ(utility::conversions::to_utf8string(data[U("catalog")].as_string()), fixture_data[0]["fields"]["catalog"].get<std::string>());
	EXPECT_EQ(utility::conversions::to_utf8string(data[U("volume")].as_string()), fixture_data[0]["fields"]["volume"].get<std::string>());
	EXPECT_EQ(data[U("incub")].as_integer(), fixture_data[0]["fields"]["incub"].get<int>());
	EXPECT_EQ(utility::conversions::to_utf8string(data[U("ar")].as_string()), fixture_data[0]["fields"]["ar"].get<std::string>());
	EXPECT_EQ(utility::conversions::to_utf8string(data[U("description")].as_string()), fixture_data[0]["fields"]["description"].get<std::string>());
	EXPECT_EQ(data[U("is_factory")].as_bool(), fixture_data[0]["fields"]["is_factory"].get<bool>());
}

TEST(PATests, AddSinglePA) {
	std::string alias = "CYN";
	std::string cat = "CYN-1014";
	int incub = 60;
	std::string ar = "Low PH";
	int factory = 1;

	json::value data;
	data[U("alias")] = json::value::string(utility::conversions::to_utf16string(alias));
	data[U("catalog")] = json::value::string(utility::conversions::to_utf16string(cat));
	data[U("incub")] = incub;
	data[U("ar")] = json::value::string(utility::conversions::to_utf16string(ar));
	data[U("factory")] = factory;

	json::value retData = rdb.CUDRequest("PA", methods::POST, "", data);

	// make sure _error key does not exists
	ASSERT_FALSE(retData.has_field(U("_error")));

	// after data is added check to see if it matches data returned
	EXPECT_EQ(utility::conversions::to_utf8string(retData[U("alias")].as_string()), alias);
	EXPECT_EQ(utility::conversions::to_utf8string(retData[U("catalog")].as_string()), cat);
	EXPECT_EQ(retData[U("incub")].as_integer() , incub);
	EXPECT_EQ(utility::conversions::to_utf8string(retData[U("ar")].as_string()), ar);

	// delete what was added
	rdb.CUDRequest("PA", methods::DEL, cat, NULL);
}

TEST(PATests, UpdateSinglePA) {
	std::string alias = "CYN2";
	std::string cat = "PA003";
	int incub = 20;
	std::string ar = "High PH";
	std::string description = "PA from test_ReagentDBClient";

	json::value data;
	data[U("alias")] = json::value::string(utility::conversions::to_utf16string(alias));
	data[U("catalog")] = json::value::string(utility::conversions::to_utf16string(cat));
	data[U("incub")] = incub;
	data[U("ar")] = json::value::string(utility::conversions::to_utf16string(ar));
	data[U("description")] = json::value::string(utility::conversions::to_utf16string(description));;

	rdb.CUDRequest("PA", methods::PUT, cat, data);
	json::value retData = rdb.GetRequest("PA", cat);

	// make sure _error key does not exists
	ASSERT_FALSE(retData.has_array_field(U("_error")));

	// after data is added check to see if it matches data returned
	EXPECT_EQ(retData[U("alias")].as_string(), utility::conversions::to_utf16string(alias));
	EXPECT_EQ(retData[U("catalog")].as_string(), utility::conversions::to_utf16string(cat));
	EXPECT_EQ(retData[U("incub")].as_integer(), incub);
	EXPECT_EQ(retData[U("ar")].as_string(), utility::conversions::to_utf16string(ar));
	EXPECT_EQ(retData[U("description")].as_string(), utility::conversions::to_utf16string(description));

	data[U("alias")] = json::value::string(U("PA3"));
	data[U("catalog")] = json::value::string(utility::conversions::to_utf16string(cat));
	data[U("incub")] = 30;
	data[U("ar")] = json::value::string(U("High PH"));
	data[U("description")] = json::value::string(U(""));;

	// change it back so test is independent
	rdb.CUDRequest("PA", methods::PUT, cat, data);
}

TEST(PATests, DeleteSinglePA) {
	// add the PA then delete it
	std::string alias = "CYN";
	std::string cat = "CYN-1014";
	int incub = 60;
	std::string ar = "Low PH";
	int factory = 1;

	json::value data;
	data[U("alias")] = json::value::string(utility::conversions::to_utf16string(alias));
	data[U("catalog")] = json::value::string(utility::conversions::to_utf16string(cat));
	data[U("incub")] = incub;
	data[U("ar")] = json::value::string(utility::conversions::to_utf16string(ar));
	data[U("factory")] = factory;

	rdb.CUDRequest("PA", methods::PATCH, "", data);

	json::value ret = rdb.CUDRequest("PA", methods::DEL, cat, NULL);
	EXPECT_EQ(ret[U("status_code")], 204);
	ret = rdb.GetRequest("PA", cat);
	EXPECT_TRUE(ret.has_field(U("_error")));
	EXPECT_EQ(ret[U("_error")].as_string(), U("404"));
}

TEST(SyncTest, CheckForPAUpdates) {
	// check if there are any updates for the client
	// settings.ini records latest timestamp sync
	json::value client_data;
	client_data[U("autostainer_sn")] = json::value::string(U("autostainer2"));
	client_data[U("last_sync")] = json::value::string(U("2020-12-02T11:53:04-0700"));
	std::vector<std::string> paths;
	paths.push_back("reagents");
	paths.push_back("api");
	paths.push_back("pa");
	paths.push_back("database_to_client_sync");

	json::value updates = rdb.PostGeneric(paths, client_data);
	// so at least 1 delta will be missing as it is too old
	// check them, they should be missing one entry dated "2020-10-28T16:24:00-08:00",
	bool missing_entry = TRUE;
	for (auto d : updates.as_array()) {
		if (d[U("date")].as_string().compare(U("2020-10-28T16:24:00-08:00")) == 0) {
			missing_entry = FALSE;
		}
	}
	ASSERT_TRUE(missing_entry);
}

TEST(SyncTest, CheckForReagentsUpdates) {
	json::value client_data;
	client_data[U("autostainer_sn")] = json::value::string(U("autostainer2"));
	client_data[U("last_sync_reagent")] = json::value::string(U("2020-12-02T11:53:04-0700"));
	std::vector<std::string> paths;
	paths.push_back("reagents");
	paths.push_back("api");
	paths.push_back("reagent");
	paths.push_back("database_to_client_sync");

	json::value updates = rdb.PostGeneric(paths, client_data);

	bool missing_entry = TRUE;
	for (auto d : updates.as_array()) {
		if (d[U("date")].as_string().compare(U("2020-10-28T16:24:00-08:00")) ==0 ) {
			missing_entry = FALSE;
		}
	}
	ASSERT_TRUE(missing_entry);
}

TEST(SyncTest, SendChangeLogPA) {
	// some data to send
	json::value data;
	// create

	data[U("alias")] = json::value::string(U("pa change add"));
	data[U("autostainer_sn")] = json::value::string(U("autostainer2"));
	data[U("fullname")] = json::value::string(U("test PA create"));
	data[U("catalog")] = json::value::string(U("PA_DELTA01"));
	data[U("incub")] = 15;
	data[U("volume")] = 3000;
	data[U("ar")] = json::value::string(U("NO"));
	data[U("is_factory")] = true;
	data[U("date")] = json::value::string(U("2020-12-02T21:36:58-08:00"));
	data[U("operation")] = json::value::string(U("CREATE"));

	rdb.ClientToDatabaseSync(data, "PA");
	// edit
	data[U("alias")] = json::value::string(U("PA 2"));
	data[U("autostainer_sn")] = json::value::string(U("autostainer2"));
	data[U("fullname")] = json::value::string(U("test PA 2 updated"));
	data[U("catalog")] = json::value::string(U("PA002"));
	data[U("incub")] = 45;
	data[U("volume")] = 3000;
	data[U("ar")] = json::value::string(U("High PH"));
	data[U("source")] = json::value::string(U("admin"));
	data[U("description")] = json::value::string(U(""));
	data[U("is_factory")] = true;
	data[U("date")] = json::value::string(U("2020-12-02T21:36:58-08:00"));
	data[U("operation")] = json::value::string(U("UPDATE"));
	rdb.ClientToDatabaseSync(data, "PA");

	json::value ret = rdb.GetRequest("PA", "");
	bool PA002 = FALSE;
	bool PA_DELTA01 = FALSE;
	bool DELETED_PA = TRUE;
	for (auto d : ret.as_array()) {
		if (d[U("catalog")].as_string().compare(U("PA002")) == 0) {
			EXPECT_EQ(d[U("fullname")].as_string(), U("test PA 2 updated"));
			EXPECT_EQ(d[U("ar")].as_string(), U("High PH"));
			EXPECT_EQ(d[U("alias")].as_string(), U("PA 2"));
			PA002 = TRUE;
		}
		else if (d[U("catalog")].as_string().compare(U("PA_DELTA01")) == 0) {
			PA_DELTA01 = TRUE;
			EXPECT_EQ(d[U("volume")].as_integer(), 3000);
		}
	}

	// delete
	data[U("date")] = json::value::string(U("2020-12-31T21:36:58-08:00"));
	data[U("autostainer_sn")] = json::value::string(U("autostainer2"));
	data[U("catalog")] = json::value::string(U("PA_DELTA01"));
	data[U("operation")] = json::value::string(U("DELETE"));

	rdb.ClientToDatabaseSync(data, "PA");
	ret = rdb.GetRequest("PA", "");
	for (auto d : ret.as_array()) {
		if (d[U("catalog")].as_string().compare(U("PA_DELTA01")) == 0) {
			DELETED_PA = FALSE;
		}
	}

	EXPECT_TRUE(PA002);
	EXPECT_TRUE(PA_DELTA01);
	EXPECT_TRUE(DELETED_PA);

	data[U("alias")] = json::value::string(U("pa change create"));
	data[U("autostainer_sn")] = json::value::string(U("autostainer2"));
	data[U("fullname")] = json::value::string(U("test PA create newer, failed"));
	data[U("catalog")] = json::value::string(U("PA001"));
	data[U("incub")] = 15;
	data[U("volume")] = 2000;
	data[U("ar")] = json::value::string(U("High PH"));
	data[U("is_factory")] = true;
	data[U("date")] = json::value::string(U("2020-12-29T21:36:58-08:00"));
	data[U("operation")] = json::value::string(U("CREATE"));

	rdb.ClientToDatabaseSync(data, "PA");
	// edit
	data[U("catalog")] = json::value::string(U("PA002"));
	data[U("fullname")] = json::value::string(U("test PA 2 updated failed"));
	data[U("autostainer_sn")] = json::value::string(U("autostainer2"));
	data[U("alias")] = json::value::string(U("PA 2"));
	data[U("source")] = json::value::string(U("failed update"));
	data[U("incub")] = 45;
	data[U("volume")] = 5000;
	data[U("ar")] = json::value::string(U("Low PH"));
	data[U("description")] = json::value::string(U(""));
	data[U("is_factory")] = true;
	data[U("date")] = json::value::string(U("2020-12-00T21:36:58-08:00"));
	data[U("operation")] = json::value::string(U("UPDATE"));
	rdb.ClientToDatabaseSync(data, "PA");

	EXPECT_EQ(ret.size(), 4);
	bool CREATE_PA = FALSE;
	PA002 = FALSE;
	for (auto d : ret.as_array()) {
		if (d[U("catalog")].as_string().compare(U("PA001")) == 0) {
			EXPECT_NE(d[U("fullname")].as_string(), U("test PA create newer, failed"));
			EXPECT_NE(d[U("ar")].as_string(), U("High PH"));
			EXPECT_NE(d[U("alias")].as_string(), U("pa change create"));
			CREATE_PA = TRUE;
		}
		else if (d[U("catalog")].as_string().compare(U("PA002")) == 0) {
			EXPECT_EQ(d[U("volume")].as_integer(), 3000);
			EXPECT_NE(d[U("fullname")].as_string(), U("test PA 2 updated failed"));
			EXPECT_NE(d[U("ar")].as_string(), U("Low PH"));
			EXPECT_EQ(d[U("source")].as_string(), U("admin"));
			PA002 = TRUE;
		}
	}
	EXPECT_TRUE(CREATE_PA);
	EXPECT_TRUE(PA002);

	// change PA002 back
	data[U("catalog")] = json::value::string(U("PA002"));
	data[U("fullname")] = json::value::string(U("test PA 2"));
	data[U("autostainer_sn")] = json::value::string(U("autostainer2"));
	data[U("alias")] = json::value::string(U("PA 2"));
	data[U("source")] = json::value::string(U("admin"));
	data[U("incub")] = 45;
	data[U("volume")] = 3000;
	data[U("ar")] = json::value::string(U("Low PH"));
	data[U("description")] = json::value::string(U(""));
	data[U("is_factory")] = true;
	data[U("date")] = json::value::string(U("2020-12-29T21:36:58-08:00"));
	data[U("operation")] = json::value::string(U("UPDATE"));
	rdb.CUDRequest("PA", methods::PUT, "PA002", data);
}

TEST(SyncTest, SendChangeLogReagent) {
	json::value data;
	data[U("autostainer_sn")] = json::value::string(U("autostainer2"));
	data[U("reagent_sn")] = json::value::string(U("send_changelog_reagent002"));
	data[U("reag_name")] = json::value::string(U("test_reagentDBClient Reagent 1"));
	data[U("catalog")] = json::value::string(U("PA002"));
	data[U("size")] = json::value::string(U("S"));
	data[U("log")] = json::value::string(U("1234"));
	data[U("vol")] = 3000;
	data[U("vol_cur")] = 1500;
	data[U("sequence")] = 0;
	data[U("mfg_date")] = json::value::string(U("2020-03-17"));
	data[U("exp_date")] = json::value::string(U("2022-03-19"));
	data[U("date")] = json::value::string(U("2020-12-31T13:49:05-08:00"));
	data[U("factory")] = true;
	data[U("r_type")] = json::value::string(U("AR"));
	data[U("operation")] = json::value::string(U("CREATE"));
	data[U("executor")] = NULL;

	rdb.ClientToDatabaseSync(data, "REAGENT");

	data[U("autostainer_sn")] = json::value::string(U("autostainer2"));
	data[U("reagent_sn")] = json::value::string(U("REAG003"));
	data[U("reag_name")] = json::value::string(U("changelog updated"));
	data[U("catalog")] = json::value::string(U("PA002"));
	data[U("size")] = json::value::string(U("S"));
	data[U("log")] = json::value::string(U("1234"));
	data[U("vol")] = 6000;
	data[U("vol_cur")] = 2000;
	data[U("sequence")] = 0;
	data[U("mfg_date")] = json::value::string(U("2020-03-17"));
	data[U("exp_date")] = json::value::string(U("2022-03-19"));
	data[U("date")] = json::value::string(U("2020-12-31T13:49:05-08:00"));
	data[U("factory")] = true;
	data[U("r_type")] = json::value::string(U("AR"));
	data[U("operation")] = json::value::string(U("UPDATE"));
	data[U("executor")] = NULL;
	rdb.ClientToDatabaseSync(data, "REAGENT");
	json::value ret = rdb.GetRequest("REAGENT", "");

	// expect the two new updates
	bool reagent_created = FALSE;
	bool reagent_updated = FALSE;
	for (auto d : ret.as_array()) {
		if (d[U("reagent_sn")].as_string().compare(U("send_changelog_reagent002")) == 0) {
			EXPECT_EQ(d[U("reag_name")].as_string(), U("send_changelog_reagent002"));
			EXPECT_EQ(d[U("catalog")].as_string(), U("PA002"));
			EXPECT_EQ(d[U("mfg_date")].as_string(), U("2020-03-17"));
			EXPECT_EQ(d[U("exp_date")].as_string(), U("2022-03-19"));
			EXPECT_EQ(d[U("vol")].as_integer(), 3000);
			EXPECT_EQ(d[U("vol_cur")].as_integer(), 1500);
			reagent_created = TRUE;
		}
		else if (d[U("reagent_sn")].as_string().compare(U("REAG003")) == 0) {
			EXPECT_EQ(d[U("reag_name")].as_string(), U("changelog updated"));
			EXPECT_EQ(d[U("catalog")].as_string(), U("PA002"));
			EXPECT_EQ(d[U("mfg_date")].as_string(), U("2020-03-17"));
			EXPECT_EQ(d[U("exp_date")].as_string(), U("2022-03-19"));
			EXPECT_EQ(d[U("vol")].as_integer(), 6000);
			EXPECT_EQ(d[U("vol_cur")].as_integer(), 2000);
			reagent_updated = TRUE;
		}
	}

	// delete 
	data[U("autostainer_sn")] = json::value::string(U("autostainer2"));
	data[U("reagent_sn")] = json::value::string(U("send_changelog_reagent002"));
	data[U("reag_name")] = json::value::string(U("changelog updated"));
	data[U("date")] = json::value::string(U("2020-12-31T13:49:05-08:00"));
	data[U("operation")] = json::value::string(U("DELETE"));
	data[U("executor")] = NULL;
	rdb.ClientToDatabaseSync(data, "REAGENT");
	ret = rdb.GetRequest("REAGENT", utility::conversions::to_utf8string(data[U("reagent_sn")].as_string()));
	EXPECT_EQ(ret[U("_error")].as_string(), U("404"));

	// create and edit that are too old to work
	data[U("autostainer_sn")] = json::value::string(U("autostainer2"));
	data[U("reagent_sn")] = json::value::string(U("REAG004"));
	data[U("reag_name")] = json::value::string(U("test_reagentDBClient Reagent 1"));
	data[U("catalog")] = json::value::string(U("PA002"));
	data[U("size")] = json::value::string(U("S"));
	data[U("log")] = json::value::string(U("1234"));
	data[U("vol")] = 4000;
	data[U("vol_cur")] = 1500;
	data[U("sequence")] = 0;
	data[U("mfg_date")] = json::value::string(U("2020-03-17"));
	data[U("exp_date")] = json::value::string(U("2022-03-19"));
	data[U("date")] = json::value::string(U("2015-12-31T13:49:05-08:00"));
	data[U("factory")] = true;
	data[U("r_type")] = json::value::string(U("AR"));
	data[U("operation")] = json::value::string(U("CREATE"));
	data[U("executor")] = NULL;
	rdb.ClientToDatabaseSync(data, "REAGENT");
	ret = rdb.GetRequest("REAGENT", utility::conversions::to_utf8string(data[U("reagent_sn")].as_string()));
	EXPECT_EQ(ret[U("vol_cur")].as_integer(), 1000);
	EXPECT_EQ(ret[U("vol")].as_integer(), 3000);
	EXPECT_EQ(ret[U("log")].as_string(), U("test"));
	EXPECT_EQ(ret[U("mfg_date")].as_string(), U("2020-03-17"));
	EXPECT_EQ(ret[U("exp_date")].as_string(), U("2021-12-31"));

	data[U("autostainer_sn")] = json::value::string(U("autostainer2"));
	data[U("reagent_sn")] = json::value::string(U("REAG004"));
	data[U("reag_name")] = json::value::string(U("test_reagentDBClient Reagent 1"));
	data[U("catalog")] = json::value::string(U("PA002"));
	data[U("size")] = json::value::string(U("S"));
	data[U("log")] = json::value::string(U("1234"));
	data[U("vol")] = 4000;
	data[U("vol_cur")] = 1500;
	data[U("sequence")] = 0;
	data[U("mfg_date")] = json::value::string(U("2020-03-17"));
	data[U("exp_date")] = json::value::string(U("2022-03-19"));
	data[U("date")] = json::value::string(U("2015-12-31T13:49:05-08:00"));
	data[U("factory")] = true;
	data[U("r_type")] = json::value::string(U("AR"));
	data[U("operation")] = json::value::string(U("UPDATE"));
	data[U("executor")] = NULL;
	rdb.ClientToDatabaseSync(data, "REAGENT");
	ret = rdb.GetRequest("REAGENT", utility::conversions::to_utf8string(data[U("reagent_sn")].as_string()));
	EXPECT_EQ(ret[U("vol_cur")].as_integer(), 1000);
	EXPECT_EQ(ret[U("vol")].as_integer(), 3000);
	EXPECT_EQ(ret[U("log")].as_string(), U("test"));
	EXPECT_EQ(ret[U("mfg_date")].as_string(), U("2020-03-17"));
	EXPECT_EQ(ret[U("exp_date")].as_string(), U("2021-12-31"));

	// update changed reagent and delet new reagent
	data[U("autostainer_sn")] = json::value::string(U("autostainer2"));
	data[U("reagent_sn")] = json::value::string(U("REAG003"));
	data[U("reag_name")] = json::value::string(U("Test Reagent 3"));
	data[U("catalog")] = json::value::string(U("PA003"));
	data[U("size")] = json::value::string(U("L"));
	data[U("log")] = json::value::string(U("log"));
	data[U("vol")] = 6000;
	data[U("vol_cur")] = 6000;
	data[U("sequence")] = 123;
	data[U("mfg_date")] = json::value::string(U("2015-01-01"));
	data[U("exp_date")] = json::value::string(U("2020-12-28"));
	data[U("factory")] = false;
	data[U("r_type")] = json::value::string(U("NO"));
	data[U("executor")] = NULL;
	rdb.CUDRequest("REAGENT", methods::PUT, utility::conversions::to_utf8string(data[U("reagent_sn")].as_string()), data);
	rdb.CUDRequest("REAGENT", methods::DEL, "send_changelog_reagent002", NULL);
}

TEST(SyncTest, SendInitialSyncPA) {
//	njson data_arr = njson::array();
	json::value data_arr = json::value::array();

	for (int i = 0; i < 5; i++) {
		json::value data;
		data[U("fullname")] = json::value::string(U("initial_sync_PA_") + std::to_wstring(i));
		data[U("source")] = json::value::string(U("test_reagentDBClient"));
		data[U("catalog")] = json::value::string(U("TEST-000") + std::to_wstring(i));
		data[U("alias")] = json::value::string(U("alias"));
		data[U("volume")] = 1000 + i;
		data[U("incub")] = 20;
		data[U("ar")] = json::value::string(U("NO"));
		data[U("description")] = json::value::string(U(""));
		data[U("factory")] = 0;
		data[U("autostainer_sn")] = json::value::string(U("autostainer1"));
		data[U("date")] = json::value::string(U("2020-12-29T21:36:58"));
		data_arr[i] = data;
	}

	std::vector<std::string> paths;
	paths.push_back("reagents");
	paths.push_back("api");
	paths.push_back("pa");
	paths.push_back("initial_sync");

	json::value all_pa = rdb.GetRequest("PA", "");
	json::value missing_data = rdb.PostGeneric(paths, data_arr);
	// expect that there are 5 more PA in the database
	EXPECT_EQ(missing_data.size() - all_pa.size(), 5);

	// now clean up and delete the the PA's
	for (int i = 0; i < 5; i++) {
		rdb.CUDRequest("PA", methods::DEL, "TEST-000" + std::to_string(i), NULL);
	}
}

TEST(ReagentTest, AddReagent) {
	json::value data;
	data[U("reag_name")] = json::value::string(U("test_reagentDBClient Reagent 1"));
	data[U("reagent_sn")] = json::value::string(U("add_reag001"));
	data[U("size")] = json::value::string(U("S"));
	data[U("log")] = json::value::string(U("1234"));
	data[U("vol_cur")] = 5000;
	data[U("vol")] = 3000;
	data[U("sequence")] = 0;
	data[U("mfg_date")] = json::value::string(U("2020-03-17"));
	data[U("exp_date")] = json::value::string(U("2022-03-19"));
	data[U("date")] = json::value::string(U("2020-12-29T21:49:05Z"));
	data[U("r_type")] = json::value::string(U("AR"));
	data[U("factory")] = true;
	data[U("catalog")] = json::value::string(U("PA002"));
	data[U("autostainer_sn")] = json::value::string(U("autostainer2"));
	data[U("in_use")] = false;

	rdb.CUDRequest("REAGENT", methods::POST, "", data);
	json::value get = rdb.GetRequest("REAGENT", 
		utility::conversions::to_utf8string(data[U("reagent_sn")].as_string()));
	EXPECT_NE(get[U("_error")].as_string(), U("404"));
	EXPECT_EQ(get[U("reag_name")].as_string(), data[U("reag_name")].as_string());
	EXPECT_EQ(get[U("vol_cur")].as_integer(), data[U("vol_cur")].as_integer());

	rdb.CUDRequest("REAGENT", methods::DEL, 
		utility::conversions::to_utf8string(data[U("reagent_sn")].as_string()), NULL);
}

TEST(ReagentTest, DeleteReagent) {
	json::value data;
	data[U("reag_name")] = json::value::string(U("test_reagentDBClient updated"));
	data[U("reagent_sn")] = json::value::string(U("add_reag001"));
	data[U("size")] = json::value::string(U("S"));
	data[U("log")] = json::value::string(U("12345"));
	data[U("vol_cur")] = 5000;
	data[U("vol")] = 3000;
	data[U("sequence")] = 0;
	data[U("mfg_date")] = json::value::string(U("2020-04-15"));
	data[U("exp_date")] = json::value::string(U("2025-03-08"));
	data[U("date")] = json::value::string(U("2020-12-30T21:49:05Z"));
	data[U("r_type")] = json::value::string(U("AR"));
	data[U("factory")] = true;
	data[U("catalog")] = json::value::string(U("PA002"));
	data[U("autostainer_sn")] = json::value::string(U("autostainer2"));
	data[U("in_use")] = false;

	rdb.CUDRequest("REAGENT", methods::POST, "", data);
	json::value get = rdb.GetRequest("REAGENT", 
		utility::conversions::to_utf8string(data[U("reagent_sn")].as_string()));
	EXPECT_NE(get[U("_error")].as_string(), U("404"));

	rdb.CUDRequest("REAGENT", methods::DEL, 
		utility::conversions::to_utf8string(data[U("reagent_sn")].as_string()), NULL);
	get = rdb.GetRequest("REAGENT", 
		utility::conversions::to_utf8string(data[U("reagent_sn")].as_string()));
	EXPECT_EQ(get[U("_error")].as_string(), U("404"));
}

TEST(ReagentTest, UpdateReagent) {
	json::value data;
	data[U("reag_name")] = json::value::string(U("test_reagentDBClient updated"));
	data[U("reagent_sn")] = json::value::string(U("REAG006"));
	data[U("size")] = json::value::string(U("S"));
	data[U("log")] = json::value::string(U("12345"));
	data[U("vol_cur")] = 5000;
	data[U("vol")] = 3000;
	data[U("sequence")] = 0;
	data[U("mfg_date")] = json::value::string(U("2020-04-15"));
	data[U("exp_date")] = json::value::string(U("2025-03-08"));
	data[U("date")] = json::value::string(U("2020-12-30T21:49:05Z"));
	data[U("r_type")] = json::value::string(U("AR"));
	data[U("factory")] = true;
	data[U("catalog")] = json::value::string(U("PA002"));
	data[U("autostainer_sn")] = json::value::string(U("autostainer2"));
	data[U("in_use")] = false;

	rdb.CUDRequest("REAGENT", methods::PUT, 
		utility::conversions::to_utf8string(data[U("reagent_sn")].as_string()), data);
	json::value get = rdb.GetRequest("REAGENT", 
		utility::conversions::to_utf8string(data[U("reagent_sn")].as_string()));
	EXPECT_EQ(get[U("reag_name")].as_string(), data[U("reag_name")].as_string());
	EXPECT_EQ(get[U("vol_cur")].as_string(), data[U("vol_cur")].as_string());
	EXPECT_EQ(get[U("size")].as_string(), data[U("size")].as_string());
	EXPECT_EQ(get[U("mfg_date")].as_string(), data[U("mfg_date")].as_string());
	EXPECT_EQ(get[U("exp_date")].as_string(), data[U("exp_date")].as_string());
	EXPECT_EQ(get[U("catalog")].as_string(), data[U("catalog")].as_string());

	data[U("reag_name")] = json::value::string(U("Test Reagent 6"));
	data[U("reagent_sn")] = json::value::string(U("REAG006"));
	data[U("size")] = json::value::string(U("M"));
	data[U("log")] = json::value::string(U("1234"));
	data[U("vol_cur")] = 120;
	data[U("vol")] = 3000;
	data[U("sequence")] = 0;
	data[U("mfg_date")] = json::value::string(U("2019-10-02"));
	data[U("exp_date")] = json::value::string(U("2020-12-31"));
	data[U("date")] = json::value::string(U("2020-12-28T22:01:37Z"));
	data[U("r_type")] = json::value::string(U("AR"));
	data[U("factory")] = true;
	data[U("catalog")] = json::value::string(U("PA003"));
	data[U("autostainer_sn")] = json::value::string(U("autostainer2"));
	data[U("in_use")] = false;

	rdb.CUDRequest("REAGENT", methods::PUT, 
		utility::conversions::to_utf8string(data[U("reagent_sn")].as_string()), NULL);
}

TEST(ReagentTest, UpdateReagentNonExistant) {
	json::value data;
	data[U("reag_name")] = json::value::string(U("test_reagentDBClient updated"));
	data[U("reagent_sn")] = json::value::string(U("add_reag001"));
	data[U("size")] = json::value::string(U("S"));
	data[U("log")] = json::value::string(U("12345"));
	data[U("vol_cur")] = 5000;
	data[U("vol")] = 3000;
	data[U("sequence")] = 0;
	data[U("mfg_date")] = json::value::string(U("2020-04-15"));
	data[U("exp_date")] = json::value::string(U("2025-03-08"));
	data[U("date")] = json::value::string(U("2020-12-30T21:49:05Z"));
	data[U("r_type")] = json::value::string(U("AR"));
	data[U("factory")] = true;
	data[U("catalog")] = json::value::string(U("PA002"));
	data[U("autostainer_sn")] = json::value::string(U("autostainer2"));
	data[U("in_use")] = false;

	rdb.CUDRequest("REAGENT", methods::PUT, 
		utility::conversions::to_utf8string(data[U("reagent_sn")].as_string()), data);
	json::value get = rdb.GetRequest("REAGENT", 
		utility::conversions::to_utf8string(data[U("reagent_sn")].as_string()));
	EXPECT_EQ(get[U("reag_name")].as_string(), data[U("reag_name")].as_string());
	EXPECT_EQ(get[U("vol_cur")].as_integer(), data[U("vol_cur")].as_integer());
	EXPECT_EQ(get[U("size")].as_string(), data[U("size")].as_string());
	EXPECT_EQ(get[U("mfg_date")].as_string(), data[U("mfg_date")].as_string());
	EXPECT_EQ(get[U("exp_date")].as_string(), data[U("exp_date")].as_string());
	EXPECT_EQ(get[U("catalog")].as_string(), data[U("catalog")].as_string());
	
	rdb.CUDRequest("REAGENT", methods::DEL, 
		utility::conversions::to_utf8string(data[U("reagent_sn")].as_string()), NULL);
}

TEST(ReagentTest, GetReagent) {
	std::string sn = "REAG005";
	json::value get = rdb.GetRequest("REAGENT", sn);

	EXPECT_EQ(get[U("reag_name")].as_string(), U("Test Reagent 5"));
	EXPECT_EQ(get[U("vol_cur")].as_integer(), 100);
	EXPECT_EQ(get[U("size")].as_string(), U("S"));
	EXPECT_EQ(get[U("mfg_date")].as_string(), U("2020-12-01"));
	EXPECT_EQ(get[U("exp_date")].as_string(), U("2030-12-01"));
	EXPECT_EQ(get[U("catalog")].as_string(), U("PA001"));
	EXPECT_EQ(get[U("r_type")].as_string(), U("AR"));
}

TEST(ReagentTest, GetReagentList) {
	json::value data = rdb.GetRequest("REAGENT", "");

	// expecting all the reagents in fixture to return
	njson fixture_data;
	std::ifstream fixture(path_to_fixtures + "\\test_reagent.json");
	fixture >> fixture_data;
	fixture.close();

	ASSERT_EQ(data.size(), fixture_data.size());

	for (int i = 0; i < data.size(); i++) {
		int ncnt = 0;
		for (auto d : data.as_array()) {
			if (utility::conversions::to_utf8string(d[U("reagent_sn")].as_string()).compare(fixture_data[i]["fields"]["reagent_sn"]) == 0) {
				EXPECT_EQ(fixture_data[i]["fields"]["size"].get<std::string>(), utility::conversions::to_utf8string(d[U("size")].as_string()));
				EXPECT_EQ(fixture_data[i]["fields"]["reag_name"].get<std::string>(), utility::conversions::to_utf8string(d[U("reag_name")].as_string()));
				EXPECT_EQ(fixture_data[i]["fields"]["catalog"].get<std::string>(), utility::conversions::to_utf8string(d[U("catalog")].as_string()));
				EXPECT_EQ(fixture_data[i]["fields"]["log"].get<std::string>(), utility::conversions::to_utf8string(d[U("log")].as_string()));
				EXPECT_EQ(fixture_data[i]["fields"]["vol_cur"].get<std::string>(), utility::conversions::to_utf8string(d[U("vol_cur")].as_string()));
				EXPECT_EQ(fixture_data[i]["fields"]["vol"].get<int>(), d[U("vol")].as_integer());
				EXPECT_EQ(fixture_data[i]["fields"]["sequence"].get<int>(), d[U("sequence")].as_integer());
				EXPECT_EQ(fixture_data[i]["fields"]["exp_date"].get<std::string>(), utility::conversions::to_utf8string(d[U("exp_date")].as_string()));
				EXPECT_EQ(fixture_data[i]["fields"]["mfg_date"].get<std::string>(), utility::conversions::to_utf8string(d[U("mfg_date")].as_string()));
				EXPECT_EQ(fixture_data[i]["fields"]["r_type"].get<std::string>(), utility::conversions::to_utf8string(d[U("r_type")].as_string()));
				EXPECT_EQ(fixture_data[i]["fields"]["factory"].get<std::string>(), utility::conversions::to_utf8string(d[U("factory")].as_string()));
				data.erase(ncnt);
				break;
			}
			ncnt++;
		}
	}
}
