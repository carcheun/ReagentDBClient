// ReagentDBClient.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ReagentDBClient.h"

ReagentDBClient::ReagentDBClient(std::string server, int port) {
	SERVER = conversions::to_utf16string(server);
	if (port > -1) {
		PORT = port;
	}

	// setup URI's here
	autostainerListPath = uri_builder(U("reagents")).append_path(U("api")).append(U("autostainer")).set_port(PORT);
	paListPath = uri_builder(U("reagents")).append_path(U("api")).append(U("pa")).set_port(PORT);
	reagentListPath = uri_builder(U("reagents")).append_path(U("api")).append(U("reagent")).set_port(PORT);
}

ReagentDBClient::~ReagentDBClient() {

}

bool ReagentDBClient::keyExists(const njson& j, const std::string& key) {
	return j.find(key) != j.end();
}

uri_builder ReagentDBClient::build_uri_from_vector(std::vector<std::string> paths) {
	uri_builder uri_b = uri_builder();
	uri_b.set_port(PORT);
	for (auto i : paths) {
		uri_b.append_path(conversions::to_utf16string(i));
	}
	return uri_b;
}

njson ReagentDBClient::GetPAList() {
	njson ret;
	int resp = -1;

	auto requestJson = http_client(SERVER)
		.request(methods::GET, paListPath.to_string() + U("/"))

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

	uri_builder newPaListPath = paListPath;
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
				paListPath.to_string() + U("/"),
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
	uri_builder newPaListPath = paListPath;
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

int ReagentDBClient::DeletePA(std::string catalog) {
	uri_builder newPaListPath = paListPath;
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
		return status_code;
	}
	catch (const std::exception &e) {
		return std::stoi(e.what());
	}
}

