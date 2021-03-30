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

void ReagentDBClient::SetAuthorizationToken(std::string token) {
	authorization_token = "Token " + token;
	_token = token;
}

utf16string ReagentDBClient::GetAuthorizationToken() {
	return utility::conversions::to_utf16string(_token);
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

void ReagentDBClient::GenerateServerCompatiableTimestamp(char *buf, int sizeBuf) {
	struct tm *now;
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
	if (ret.length() < 2) {
		// 7 = 
		ret = "00000" + ret;
	}
	else if (ret.length() < 3) {
		// 49
		ret = "0000" + ret;
	}
	else if (ret.length() < 4) {
		// 750
		ret = "0" + ret + "00";
	}
	else if (ret.length() < 5) {
		// 1250
		ret = ret + "00";
	}
	else if (ret.length() < 6) {
		ret = "0" + ret;
	}

	if (ret.length() == 6) {
		ret.insert(2, ":");
		ret.insert(5, ":");
	}
	else {
		// reach some kind of invalid timestamp...
		ret = "00:00:00";
	}
	return ret;
}

uri_builder ReagentDBClient::build_uri_from_vector(std::vector<std::string> paths) {
	uri_builder uri_b = uri_builder();
	for (auto i : paths) {
		uri_b.append_path(web::uri::encode_data_string(conversions::to_utf16string(i)));
	}
	return uri_b;
}

json::value ReagentDBClient::GetPAByAlias(std::wstring alias) {
	json::value ret;
	uri_builder uriPath = paPath;
	uriPath.append_path(U("alias/"));
	uriPath.append_query(U("alias"), alias);

	http_request request;
	request.set_method(methods::GET);
	request.headers().add(U("Authorization"), utility::conversions::utf8_to_utf16(authorization_token));
	request.set_request_uri(uriPath.to_string());
		
	auto getJson = pplx::create_task([&]() {
		return http_client(SERVER).request(request);
	})
	.then([](http_response response) {
		if (response.status_code() != 200) {
			throw std::runtime_error(std::to_string(response.status_code()));
		}
		return response.extract_json();
	})
	.then([&](json::value jsonObject) {
		ret = jsonObject;
		return;
	});

	// Wait for the concurrent tasks to finish.
	try {
		getJson.wait();
		return ret;
	}
	catch (const std::exception &e) {
		json::value err;
		err[U("_error")] = json::value::string(conversions::to_utf16string(e.what()));
		return err;
	}
}

json::value ReagentDBClient::GetRequest(std::string endpoint, std::string PID) {
	json::value ret;
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
		ret[U("_error")] = json::value::string(U("Invalid endpoint!"));
		return ret;
	}
	uriPath.append_path(conversions::to_utf16string(PID));

	http_request request;
	request.set_method(methods::GET);
	request.headers().add(U("Authorization"), utility::conversions::utf8_to_utf16(authorization_token));
	request.set_request_uri(uriPath.to_string() + U("/"));

	auto requestJson = http_client(SERVER)
		.request(request).then([](http_response response) {
		if (response.status_code() != 200) {
			throw std::runtime_error(std::to_string(response.status_code()));
		}
		return response.extract_json();
	})
	.then([&](json::value jsonObject) {
		ret = jsonObject;
		return;
	});
	try {
		requestJson.wait();
		return ret;
	}
	catch (const std::exception &e) {
		json::value err;
		err[U("_error")] = json::value::string(conversions::to_utf16string(e.what()));
		return err;
	}

	return -1;
}

json::value ReagentDBClient::CUDRequest(std::string endpoint, method mtd, 
	std::string PID, json::value data) {
	// Read the endpoint: REAGENT OR PA
	json::value ret;
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
		ret[U("_error")] = json::value::string(U("Invalid endpoint!"));
		return ret;
	}

	http_request request;
	request.set_method(mtd);
	request.headers().add(U("Authorization"), utility::conversions::utf8_to_utf16(authorization_token));

	// get or delete, we will need a PID
	auto req = pplx::create_task([&]() {
		if (!PID.empty())
			uriPath.append_path(conversions::to_utf16string(PID));

		if (mtd == methods::PUT || mtd == methods::POST) {
			request.set_request_uri(uriPath.to_string() + U("/"));
			request.set_body(data.serialize(),
				U("application/json"));
		}
		else if (mtd == methods::GET || mtd == methods::DEL) {
			request.set_request_uri(uriPath.to_string() + U("/"));
		}

		return http_client(SERVER).request(request);
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
		ret = jsonObject;
		return;
	});


	try {
		req.wait();
		return ret;
	}
	catch (const std::exception &e) {
		json::value err;
		err[U("_error")] = json::value::string(conversions::to_utf16string(e.what()));
		return err;
	}
}

json::value ReagentDBClient::AddReagent(json::value jsonObj) {
	json::value ret;

	http_request request;
	request.set_method(methods::POST);
	request.headers().add(U("Authorization"), utility::conversions::utf8_to_utf16(authorization_token));
	request.set_request_uri(reagentPath.to_string() + U("/"));
	request.set_body(jsonObj.serialize(), U("application/json"));

	auto postJson = pplx::create_task([&]() {
		return http_client(SERVER)
			.request(request);
	})
	.then([](http_response response) {
		// Check the status code.
		if (response.status_code() != 201)
			throw std::runtime_error(std::to_string(response.status_code()));
		return response.extract_json();
	})
	.then([&](json::value jsonObject) {
		ret = jsonObject;
		return;
	});

	try {
		postJson.wait();
		return ret;
	}
	catch (const std::exception &e) {
		json::value err;
		err[U("_error")] = json::value::string(conversions::to_utf16string(e.what()));
		return err;
	}
}

json::value ReagentDBClient::DecreaseReagentVolume(std::string reagentSN, std::string serialNo, int decVol) {
	uri_builder newReagentPath = reagentPath;
	newReagentPath.append_path(conversions::to_utf16string(reagentSN));
	newReagentPath.append_path(conversions::to_utf16string("decrease-volume"));
	json::value ret;
	json::value req;
	req[U("dec_vol")] = decVol;
	req[U("autostainer_sn")] = json::value::string(conversions::to_utf16string(serialNo));

	http_request request;
	request.set_method(methods::PUT);
	request.headers().add(U("Authorization"), utility::conversions::utf8_to_utf16(authorization_token));
	request.set_request_uri(newReagentPath.to_string() + U("/"));
	request.set_body(req.serialize(), U("application/json"));

	auto requestJson = http_client(SERVER).request(request).then([](http_response response) {
		if (response.status_code() != 201) {
			throw std::runtime_error(std::to_string(response.status_code()));
		}
		return response.extract_json();
	})
		.then([&](json::value jsonObject) {
		ret = jsonObject;
		return;
	});

	// Wait for the concurrent tasks to finish.
	try {
		requestJson.wait();
		return ret;
	}
	catch (const std::exception &e) {
		json::value err;
		err[U("_error")] = json::value::string(conversions::to_utf16string(e.what()));
		return err;
	}
}

json::value ReagentDBClient::PostGeneric(std::vector<std::string> paths, json::value jsonObj) {
	uri_builder uri_build = build_uri_from_vector(paths);
	json::value ret;

	http_request request;
	request.set_method(methods::POST);
	request.headers().add(U("Authorization"), utility::conversions::utf8_to_utf16(authorization_token));
	request.set_request_uri(uri_build.to_string() + U("/"));
	request.set_body(jsonObj.serialize(), U("application/json"));

	// use & to use the above variables, or pass them in one at a time
	auto postJson = pplx::create_task([&]() {
		return http_client(SERVER).request(request);
	})
		.then([](http_response response) {
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
	})		// Get the data field and serialize, then use nlohmann/json to parse
		.then([&](json::value jsonObject) {
		ret = jsonObject;
		return;
	});

	// Wait for the concurrent tasks to finish.
	try {
		postJson.wait();
	}
	catch (const std::exception &e) {
		json::value err;
		err[U("_error")] = json::value::string(conversions::to_utf16string(e.what()));
		return err;
	}
	return ret;
}

// GET generic
json::value ReagentDBClient::GetGeneric(std::vector<std::string> paths, const std::map<std::string, std::string>& urlParams) {
	uri_builder uri_build = build_uri_from_vector(paths);
	json::value ret;

	for (auto const& x : urlParams) {
		uri_build.append_query(conversions::to_utf16string(x.first),
			web::uri::encode_data_string(conversions::to_utf16string(x.second)));
	}

	http_request request(methods::GET);
	if (!authorization_token.empty()) {
		request.headers().add(U("Authorization"), utility::conversions::utf8_to_utf16(authorization_token));
	}

	request.set_request_uri(urlParams.empty() ?
		uri_build.to_string() + U("/") : uri_build.to_string());

	auto requestJson = http_client(SERVER)
		.request(request)
		.then([](http_response response) {
		if (response.status_code() != 200) {
			throw std::runtime_error(std::to_string(response.status_code()));
		}
		return response.extract_json();
	})
		.then([&](json::value jsonObject) {
		ret = jsonObject;
		return;
	});

	try {
		requestJson.wait();
		return ret;
	}
	catch (const std::exception &e) {
		json::value err;
		err[U("_error")] = json::value::string(conversions::to_utf16string(e.what()));
		return err;
	}
}

json::value ReagentDBClient::DeleteMultiplePA(json::value data) {
	uri_builder newPaListPath = paPath;
	newPaListPath.append_path(conversions::to_utf16string("delete"));
	int status_code = -1;

	http_request request;
	request.set_method(methods::DEL);
	request.headers().add(U("Authorization"), utility::conversions::utf8_to_utf16(authorization_token));
	request.set_request_uri(newPaListPath.to_string() + U("/"));
	request.set_body(data.serialize(), U("application/json"));

	auto requestJson = http_client(SERVER).request(request)
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
		json::value ret;
		ret[U("status_code")] = status_code;
		return ret;
	}
	catch (const std::exception &e) {
		json::value err;
		err[U("_error")] = json::value::string(conversions::to_utf16string(e.what()));
		return err;
	}
}

void ReagentDBClient::TestClient() {
	websocket_callback_client call_client;
}

json::value ReagentDBClient::ClientToDatabaseSync(json::value data, std::string endpoint) {
	json::value ret;
	uri_builder newPaListPath;

	if (endpoint.compare("PA") == 0) {
		newPaListPath = paPath;
	}
	else if (endpoint.compare("REAGENT") == 0) {
		newPaListPath = reagentPath;
	}

	newPaListPath.append_path(conversions::to_utf16string("client_to_database_sync"));
	int status_code = -1;

	http_request request;
	request.set_method(methods::POST);
	request.headers().add(U("Authorization"), utility::conversions::utf8_to_utf16(authorization_token));
	request.set_request_uri(newPaListPath.to_string() + U("/"));
	request.set_body(data.serialize(), U("application/json"));

	auto postJson = pplx::create_task([&]() {
		return http_client(SERVER).request(request);
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
		ret = jsonObject;
		ret[U("status_code")] = status_code;
		return;
	});

	// Wait for the concurrent tasks to finish.
	try {
		postJson.wait();
		return ret;
	}
	catch (const std::exception &e) {
		json::value err;
		err[U("_error")] = json::value::string(conversions::to_utf16string(e.what()));
		return err;
	}
}
