/**
 CPPRestSDK is statically linked by following instructions here:
 https://stackoverflow.com/questions/44905708/statically-linking-casablanca-cpprest-sdk
 https://stackoverflow.com/questions/56097412/how-to-statically-link-cpprest-without-dll-files-in-vs-project/57177759
 
 Useage for new projects:
 Copy 
 */

#pragma once

#ifdef REAGENTDBCLIENT_EXPORTS
#define REAGENTDBCLIENT_API __declspec(dllexport)
#else
#define REAGENTDBCLIENT_API __declspec(dllimport)
#endif

#include <iostream>
#include <cpprest/http_client.h>
#include <cpprest/http_listener.h>
#include <cpprest/filestream.h>
#include <cpprest/uri.h>
#include <cpprest/json.h>
#include <cpprest/ws_client.h>
#include <string>

using namespace utility;
using namespace web;
using namespace web::http;
using namespace web::http::client;
using namespace web::websockets::client;

class REAGENTDBCLIENT_API ReagentDBClient
{
public:
	ReagentDBClient(std::string server);
	virtual ~ReagentDBClient();

	// set authorization token in request header
	void SetAuthorizationToken(std::string token);
	utf16string GetAuthorizationToken();

	// Generic GET/POST functions
	json::value GetRequest(std::string endpoint, std::string PID);
	json::value PostGeneric(std::vector<std::string> paths, json::value jsonObj);
	json::value GetGeneric(std::vector<std::string> paths,
		const std::map<std::string, std::string>& urlParams = map_type());

	json::value GetPAByAlias(std::wstring alias);
	json::value DeleteMultiplePA(json::value data);
	json::value AddReagent(json::value jsonObj);
	json::value CUDRequest(std::string endpoint, method mtd, std::string PID, json::value data);
	json::value ClientToDatabaseSync(json::value data, std::string endpoint);
	json::value DecreaseReagentVolume(std::string reagentSN, std::string serialNo, int decVol);

	// helper functions
	void GenerateServerCompatiableTimestamp(char *buf, int sizeBuf);
	std::string ConvertClientDateToServerDateField(int date);
	std::string ConvertClientTimeToServerTimeField(int time);
	int ConvertServerDateFieldToClientDate(std::string date);
	void ConvertServerDateTimeFieldToClientDateTime(int &date, int &time, std::string dateTime);

	void TestClient();

private:
	typedef std::map<std::string, std::string> map_type;
	utf16string SERVER;
	std::string authorization_token;
	std::string _token;
	uri_builder autostainerListPath;
	uri_builder paPath;
	uri_builder reagentPath;
	uri_builder build_uri_from_vector(std::vector<std::string> paths);
};