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
#include "cpprest/cpprest/http_client.h"
#include "cpprest/http_listener.h"
#include "cpprest/cpprest/filestream.h"
#include "cpprest/cpprest/uri.h"
#include "cpprest/cpprest/json.h"
#include <string>
#include <atlstr.h>
#include "nlohmann/json.hpp"

using namespace utility;
using namespace web;
using namespace web::http;
using namespace web::http::client;
using njson = nlohmann::json;

class REAGENTDBCLIENT_API ReagentDBClient
{
public:
	ReagentDBClient(std::string server);
	virtual ~ReagentDBClient();

	// general helper functions
	bool keyExists(const njson& j, const std::string& key);

	// Generic GET/POST functions
	njson GetGeneric(std::vector<std::string> paths);
	njson PostGeneric(std::vector<std::string> paths, njson data);
	njson CUDRequest(std::string endpoint, method mtd, std::string PID, njson data);

	// PA api functions
	njson GetPAList();
	njson GetPAList(std::string catalog);
	njson AddPA(njson jsonObj);
	njson PutPA(njson jsonObj, std::string catalog);
	njson DeletePA(std::string);
	njson DeleteMultiplePA(njson jsonObj);
	njson ClientToDatabaseSync(njson jsonObj, std::string endpoint);


	// TODO: reagents
	njson AddReagent(njson reagent);
	njson DecreaseReagentVolume(std::string reagentSN, std::string serialNo, int decVol);
	njson DeleteReagent(CString reagentSN);

	// helper functions
	void GenerateServerCompatiableTimestamp(char *buf, int sizeBuf);
	std::string ConvertClientDateToServerDateField(int date);
	std::string ConvertClientTimeToServerTimeField(int time);
	int ConvertServerDateFieldToClientDate(std::string date);
	void ConvertServerDateTimeFieldToClientDateTime(int &date, int &time, std::string dateTime);


private:
	utf16string SERVER;

	uri_builder autostainerListPath;
	uri_builder paPath;
	uri_builder reagentPath;

	uri_builder build_uri_from_vector(std::vector<std::string> paths);
};