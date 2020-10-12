// ReagentDBClient.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ReagentDBClient.h"

ReagentDBClient::ReagentDBClient(std::string server) {
	SERVER = conversions::to_utf16string(server);
	// setup URI's here
	autostainerListPath = uri_builder(U("reagents")).append_path(U("api")).append(U("autostainer"));
	paPath = uri_builder(U("reagents")).append_path(U("api")).append(U("pa"));
	reagentPath = uri_builder(U("reagents")).append_path(U("api")).append(U("reagent"));
}

ReagentDBClient::~ReagentDBClient() {

}

std::string ReagentDBClient::ConvertClientDateToServerDateField(int date) {
	// date format from client is YYMMDD
	std::string ret;
	if (date >= 991231) {
		// maximum possible date
		return "2099-12-31";
	}
	else if (date <= 99999) {
		// anything less than 2010 we will not accept...
		return "2010-12-31";
	}
	// YEAR
	ret = "20" + std::to_string(int(date / 10000));
	date %= 10000;
	// MONTH
	ret = ret + "-" + std::to_string(int(date / 100));
	date %= 100;
	// DAY
	ret = ret + "-" + std::to_string(int(date));

	return ret;
}

int ReagentDBClient::ConvertServerDateFieldToClientDate(std::string date) {
	// YYYY-MM-DD
	int ret;
	if (date.length() > 10) {
		// literally still using this in 2099 :)
		return 991231;
	}
	// YEAR
	ret = std::stoi(date.substr(2,2));
	ret *= 10000;
	// MONTH
	ret += (std::stoi(date.substr(5,2)) * 100);
	// DAY
	ret += std::stoi(date.substr(8, 2));

	return ret;
}

void ReagentDBClient::ConvertServerDateTimeFieldToClientDateTime(int &date, int &time, std::string dateTime) {
	// "2020-10-08T09:28:00-07:00"
	date = std::stoi(dateTime.substr(2, 2));
	date *= 100;
	
	date += std::stoi(dateTime.substr(5, 2));
	date *= 100;

	date += std::stoi(dateTime.substr(8, 2));

	time = std::stoi(dateTime.substr(11, 2));
	time *= 100;

	time = std::stoi(dateTime.substr(14, 2));
	time *= 100;

	time = std::stoi(dateTime.substr(17, 2));

	return;
}

void ReagentDBClient::GenerateServerCompatiableTimestamp(struct tm *now, char *buf, int sizeBuf) {
	time_t t;
	time(&t);
	now = localtime(&t);
	strftime(buf, sizeBuf, "%Y-%m-%dT%H:%M:%S%z", now);
}

std::string ReagentDBClient::ConvertClientTimeToServerTimeField(int time) {
	// there was no time field
	if (time == 0) {
		return "00:00:00";
	}
	std::string ret = std::to_string(time);

	// check length
	if (ret.length() < 4) {
		ret = "0" + ret + "00";
	}
	else if (ret.length() < 5) {
		ret = ret + "00";
	}

	ret.insert(2, ":");
	ret.insert(5, ":");

	return ret;
}

bool ReagentDBClient::keyExists(const njson& j, const std::string& key) {
	return j.find(key) != j.end();
}

uri_builder ReagentDBClient::build_uri_from_vector(std::vector<std::string> paths) {
	uri_builder uri_b = uri_builder();
	for (auto i : paths) {
		uri_b.append_path(conversions::to_utf16string(i));
	}
	return uri_b;
}

njson ReagentDBClient::CUDRequest(std::string endpoint, method mtd, std::string PID, njson data) {
	// Read the endpoint: REAGENT OR PA
	njson ret;
	uri_builder uriPath;
	if (endpoint.compare("REAGENT") == 0) {
		// set the path for reagents
		uriPath = reagentPath;
	}
	else if (endpoint.compare("PA") == 0) {
		// set paths for PA
		uriPath = paPath;
	}
	else {
		ret["_error"] = "Invalid endpoint!";
		return ret;
	}

	// get or delete, we will need a PID
	auto req = pplx::create_task([&]() {
		if (!PID.empty())
			uriPath.append_path(conversions::to_utf16string(PID));

		if (mtd == methods::PUT || mtd == methods::POST) {
			return http_client(SERVER)
				.request(mtd, uriPath.to_string() + U("/"),
					conversions::to_utf16string(data.dump()), U("application/json"));
		}
		else if (mtd == methods::GET || mtd == methods::DEL) {
				return http_client(SERVER)
					.request(mtd, uriPath.to_string() + U("/"));
		}
	}).then([](http_response response) {
		// Check the status code.
		if (response.status_code() == 201 || response.status_code() == 200) {
			return response.extract_json();
		}
		else if (response.status_code() == 204) {
			// no content, in order for consistant parsing we'll just add some content
			// before passing it on to our program
			json::value output;
			output[L"content"] = json::value::string(utility::conversions::to_utf16string("No Content"));
			response.set_body(output);
			return response.extract_json();
		}
		else {
			throw std::runtime_error(std::to_string(response.status_code()));
		}
		// Convert the response body to JSON object.
	}).then([&](json::value jsonObject) {
		ret = njson::parse(jsonObject.serialize());
		return;
	});


	try {
		req.wait();
		return ret;
	}
	catch (const std::exception &e) {
		njson err = {
			{"_error", e.what()}
		};
		return err;
	}
	return ret;
}

// add a reagent
njson ReagentDBClient::AddReagent(njson reagent) {
	njson ret;
	auto postJson = pplx::create_task([&]() {
		return http_client(SERVER)
			.request(methods::POST,
				reagentPath.to_string() + U("/"),
				conversions::to_utf16string(reagent.dump()), U("application/json"));
	})
	.then([](http_response response) {
		if (response.status_code() != 201) {
			throw std::runtime_error(std::to_string(response.status_code()));
		}
		return response.extract_json();
	})
	.then([&](json::value jsonObject) {
		ret = njson::parse(jsonObject.serialize());
		return;
	});

	// Wait for the concurrent tasks to finish.
	try {
		postJson.wait();
		return ret;
	}
	catch (const std::exception &e) {
		njson err = {
			{"_error", e.what()}
		};
		return err;
	}
	return 1;
}

// decrease volume
njson ReagentDBClient::DecreaseReagentVolume(std::string reagentSN, std::string serialNo, int decVol) {
	uri_builder newReagentPath = reagentPath;
	newReagentPath.append_path(conversions::to_utf16string(reagentSN));
	newReagentPath.append_path(conversions::to_utf16string("decrease-volume"));
	njson ret;
	njson req;
	req["dec_vol"] = decVol;
	req["autostainer_sn"] = serialNo;

	auto requestJson = http_client(SERVER)
		.request(methods::PUT, newReagentPath.to_string() + U("/"), conversions::to_utf16string(req.dump()), U("application/json"))
		.then([](http_response response) {
		if (response.status_code() != 201) {
			throw std::runtime_error(std::to_string(response.status_code()));
		}
		return response.extract_json();
	})
		// Get the data field and serialize, then use nlohmann/json to parse
		.then([&](json::value jsonObject) {
		ret = njson::parse(jsonObject.serialize());
		return;
	});

	// Wait for the concurrent tasks to finish.
	try {
		requestJson.wait();
		return ret;
	}
	catch (const std::exception &e) {
		njson err = {
			{"_error", e.what()},
		};
		return err;
	}
}

// delete
njson ReagentDBClient::DeleteReagent(CString reagentSN) {
	uri_builder newReagentPath = reagentPath;
	newReagentPath.append_path((LPCTSTR)reagentSN);
	int status_code = -1;

	auto requestJson = http_client(SERVER)
		.request(methods::DEL, newReagentPath.to_string() + U("/"))

		// Get the response.
		.then([&](http_response response) {
		// Check the status code.
		if (response.status_code() != 204) {
			throw std::runtime_error(std::to_string(response.status_code()));
		}

		// Convert the response body to JSON object.
		status_code = response.status_code();
		return;
	});

	// Wait for the concurrent tasks to finish.
	try {
		requestJson.wait();
		njson ret;
		ret["status_code"] = status_code;
		return ret;
	}
	catch (const std::exception &e) {
		njson err = {
			{"_error", e.what()},
		};
		return err;
	}
}


njson ReagentDBClient::GetPAList() {
	njson ret;
	int resp = -1;

	auto requestJson = http_client(SERVER)
		.request(methods::GET, paPath.to_string() + U("/"))

		// Get the response.
		.then([](http_response response) {
		// Check the status code.
		if (response.status_code() != 200) {
			throw std::runtime_error(std::to_string(response.status_code()));
		}

		// Convert the response body to JSON object.
		return response.extract_json();
	})
		// Get the data field and serialize, then use nlohmann/json to parse
		.then([&](json::value jsonObject) {
		ret = njson::parse(jsonObject.serialize());
		return;
	});

	// Wait for the concurrent tasks to finish.
	try {
		requestJson.wait();
		return ret;
	}
	catch (const std::exception &e) {
		njson err = {
			{"_error", e.what()},
		};
		return err;
	}

	return -1;
}

njson ReagentDBClient::GetPAList(std::string catalog) {
	// append ID to new uri
	njson ret;

	uri_builder newPaListPath = paPath;
	newPaListPath.append_path(conversions::to_utf16string(catalog));

	auto requestJson = http_client(SERVER)
		.request(methods::GET, newPaListPath.to_string() + U("/"))

		// Get the response.
		.then([](http_response response) {
		// Check the status code.
		if (response.status_code() != 200) {
			throw std::runtime_error(std::to_string(response.status_code()));
		}

		// Convert the response body to JSON object.
		return response.extract_json();
	})
		// Get the data field and serialize, then use nlohmann/json to parse
		.then([&](json::value jsonObject) {
		ret = njson::parse(jsonObject.serialize());
		return;
	});

	// Wait for the concurrent tasks to finish.
	try {
		requestJson.wait();
		return ret;
	}
	catch (const std::exception &e) {
		//printf("Error exception:%s\n", e.what());
		njson err = {
			{"_error", e.what()},
		};
		return err;
	}

	return -1;
}

njson ReagentDBClient::AddPA(njson jsonObj) {
	njson ret;

	// use & to use the above variables, or pass them in one at a time
	auto postJson = pplx::create_task([&]() {
		return http_client(SERVER)
			.request(methods::POST,
				paPath.to_string() + U("/"),
				conversions::to_utf16string(jsonObj.dump()), U("application/json"));
	})
		// Get the response.
		.then([](http_response response) {
		// Check the status code.
		if (response.status_code() != 201) {
			throw std::runtime_error(std::to_string(response.status_code()));
		}

		// Convert the response body to JSON object.
		return response.extract_json();
	})		// Get the data field and serialize, then use nlohmann/json to parse
		.then([&](json::value jsonObject) {
		ret = njson::parse(jsonObject.serialize());
		return;
	});

	// Wait for the concurrent tasks to finish.
	try {
		postJson.wait();
		return ret;
	}
	catch (const std::exception &e) {
		//printf("Error exception:%s\n", e.what());
		njson err = {
			{"_error", e.what()}
		};
		return err;
	}

	return 1;
}

njson ReagentDBClient::PutPA(njson jsonObj, std::string catalog) {
	// find the catalog value
	uri_builder newPaListPath = paPath;
	njson ret;
	newPaListPath.append_path(conversions::to_utf16string(catalog));

	// use & to use the above variables, or pass them in one at a time
	auto putJson = pplx::create_task([&]() {
		return http_client(SERVER)
			.request(methods::PUT,
				newPaListPath.to_string() + U("/"),
				conversions::to_utf16string(jsonObj.dump()), U("application/json"));
	})
		// Get the response.
		.then([](http_response response) {
		// Check the status code.
		if (response.status_code() != 200) {
			throw std::runtime_error(std::to_string(response.status_code()));
		}

		// Convert the response body to JSON object.
		return response.extract_json();
	})	// Get the data field and serialize, then use nlohmann/json to parse
		.then([&](json::value jsonObject) {
		ret = njson::parse(jsonObject.serialize());
		return;
	});

	try {
		putJson.wait();
		return ret;
	}
	catch (const std::exception &e) {
		njson err = {
			{"_error", e.what()}
		};
		return err;
	}

	return -1;
}

njson ReagentDBClient::PostGeneric(std::vector<std::string> paths, njson data) {
	uri_builder uri_build = build_uri_from_vector(paths);
	njson ret;

	// use & to use the above variables, or pass them in one at a time
	auto postJson = pplx::create_task([&]() {
		return http_client(SERVER)
			.request(methods::POST,
				uri_build.to_string() + U("/"),
				conversions::to_utf16string(data.dump()), U("application/json"));
	})
		// Get the response.
		.then([](http_response response) {
		// Check the status code.
		if (response.status_code() == 201 || response.status_code()== 200) {
			return response.extract_json();
		}
		else if (response.status_code() == 204) {
			// no content, in order for consistant parsing we'll just add some content
			// before passing it on to our program
			json::value output;
			output[L"content"] = json::value::string(utility::conversions::to_utf16string("No Content"));
			response.set_body(output);
			return response.extract_json();
		}
		else {
			throw std::runtime_error(std::to_string(response.status_code()));
		}

		// Convert the response body to JSON object.
	})		// Get the data field and serialize, then use nlohmann/json to parse
		.then([&](json::value jsonObject) {
		ret = njson::parse(jsonObject.serialize());
		return;
	});

	// Wait for the concurrent tasks to finish.
	try {
		postJson.wait();
		return ret;
	}
	catch (const std::exception &e) {
		//printf("Error exception:%s\n", e.what());
		njson err = {
			{"_error", e.what()}
		};
		return err;
	}

	return 1;
}

njson ReagentDBClient::GetGeneric(std::vector<std::string> paths) {
	uri_builder uri_build = build_uri_from_vector(paths);
	njson ret;
	int resp = -1;

	auto requestJson = http_client(SERVER)
		.request(methods::GET, uri_build.to_string() + U("/"))
		.then([](http_response response) {
		if (response.status_code() != 200) {
			throw std::runtime_error(std::to_string(response.status_code()));
		}
		return response.extract_json();
	})
		.then([&](json::value jsonObject) {
		ret = njson::parse(jsonObject.serialize());
		return;
	});
	try {
		requestJson.wait();
		return ret;
	}
	catch (const std::exception &e) {
		njson err = {
			{"_error", e.what()},
		};
		return err;
	}

	return -1;
}

njson ReagentDBClient::DeletePA(std::string catalog) {
	uri_builder newPaListPath = paPath;
	newPaListPath.append_path(conversions::to_utf16string(catalog));
	int status_code = -1;

	auto requestJson = http_client(SERVER)
		.request(methods::DEL, newPaListPath.to_string() + U("/"))

		// Get the response.
		.then([&](http_response response) {
		// Check the status code.
		if (response.status_code() != 204) {
			throw std::runtime_error(std::to_string(response.status_code()));
		}

		// Convert the response body to JSON object.
		status_code = response.status_code();
		return;
	});

	// Wait for the concurrent tasks to finish.
	try {
		requestJson.wait();
		njson ret;
		ret["status_code"] = status_code;
		return ret;
	}
	catch (const std::exception &e) {
		njson err = {
			{"_error", e.what()},
		};
		return err;
	}
}

njson ReagentDBClient::DeleteMultiplePA(njson data) {
	uri_builder newPaListPath = paPath;
	newPaListPath.append_path(conversions::to_utf16string("delete"));
	int status_code = -1;

	auto requestJson = http_client(SERVER)
		.request(methods::DEL, newPaListPath.to_string() + U("/"), 
			conversions::to_utf16string(data.dump()), U("application/json"))

	// Get the response.
	.then([&](http_response response) {
		// Check the status code.
		if (response.status_code() != 204) {
			throw std::runtime_error(std::to_string(response.status_code()));
		}

		// Convert the response body to JSON object.
		status_code = response.status_code();
		return;
	});

	// Wait for the concurrent tasks to finish.
	try {
		requestJson.wait();
		njson ret;
		ret["status_code"] = status_code;
		return ret;
	}
	catch (const std::exception &e) {
		njson err = {
			{"_error", e.what()},
		};
		return err;
	}
}

njson ReagentDBClient::ClientToDatabaseSync(njson data, std::string endpoint) {
	// send my data to the URL
	// await response.
	// parse response
	njson ret;
	uri_builder newPaListPath;
	if (endpoint.compare("PA") == 0) {
		newPaListPath = paPath;
	}
	else if (endpoint.compare("REAGENT") == 0) {
		newPaListPath = reagentPath;
	}

	newPaListPath.append_path(conversions::to_utf16string("client_to_database_sync"));
	int status_code = -1;

	auto postJson = pplx::create_task([&]() {
		return http_client(SERVER)
			.request(methods::POST,
				newPaListPath.to_string() + U("/"),
				conversions::to_utf16string(data.dump()), U("application/json"));
	})
	.then([&](http_response response) {
		status_code = response.status_code();
		if (status_code == 200 || status_code == 201) {
			// 200 & 201 & 204 status codes are always 'good', and client
			// can safely delete their delta log entry and will not require
			// further modifications
			return response.extract_json();
		}
		else if (status_code == 204) {
			json::value output;
			output[L"data"] = json::value::string(utility::conversions::to_utf16string("no content"));
			response.set_body(output);
			return response.extract_json();
		}
		else if (status_code == 400) {
			// bad request, client delta log has some request error
			throw std::runtime_error(std::to_string(response.status_code()));
		}
		else if (status_code == 404) {
			// not found, if the action was to update something, this implies that
			// at some point the server has deleted the item. Client should remove
			// item from local DB
			throw std::runtime_error(std::to_string(response.status_code()));
		}
		else if (status_code == 409) {
			// conflict, generally means that item already existed when creating,
			// or that an update attempt is older than the current time stamp.
			// client will recieve the server copy and update themselves accordingly ?
			return response.extract_json();
		}
		else {
			throw std::runtime_error(std::to_string(response.status_code()));
		}
	})
	.then([&](json::value jsonObject) {
		ret = njson::parse(jsonObject.serialize());
		ret["status_code"] = status_code;
		return;
	});

	// Wait for the concurrent tasks to finish.
	try {
		postJson.wait();
		return ret;
	}
	catch (const std::exception &e) {
		//printf("Error exception:%s\n", e.what());
		njson err = {
			{"_error", e.what()}
		};
		return err;
	}
}

