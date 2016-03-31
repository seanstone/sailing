#include <string>
#include <iostream>
#include <sstream>
#include <thread>
#include <boost/algorithm/string/replace.hpp>
#include <iomanip>

#include <command.hpp>

std::string parseCmd(std::string cmd)
{
	std::stringstream ss(cmd);
	std::string word;
	ss >> word;
	std::stringstream response;

	/** Commands **/

	if 		(word == "echo") {
		std::string msg;
		getline(ss, msg);
		response << msg;
	}
	else if (word == "run") {
		voyage = new Voyage();
		(*voyage)();
		//std::thread voyageThread(std::ref(*voyage));
		//voyageThread.detach();
	}
	else if (word == "send") {
		std::string msg;
		getline(ss, msg);
		wsServer->sendAll(msg);
	}

	/** Variables **/

	else if (word == "orig") {
		response << std::fixed << std::setprecision(6) << voyage->orig;
	}
	else if (word == "dest") {
		response << std::fixed << std::setprecision(6) << voyage->dest;
	}
	else if (word == "timestep") {
		response << voyage->timestep;
	}
	else if (word == "movement_factor") {
		response << voyage->movement_factor;
	}
	else if (word == "alpha") {
		response << std::fixed << std::setprecision(2) << voyage->alpha;
	}
	else if (word == "altitude") {
		response << std::fixed << std::setprecision(2) << voyage->altitude;
	}
	else if (word == "range") {
		response << std::fixed << std::setprecision(2) << voyage->range;
	}
	else if (word == "sail_open") {
		response << voyage->sail_open;
	}
	else if (word == "dir") {
		if (voyage->dir) response << std::fixed << std::setprecision(2) << *voyage->dir;
	}

	/** Assignment **/

	else if (word == "orig=") {
		ss >> voyage->orig;
		response << word << std::fixed << std::setprecision(6) << voyage->orig;
	}
	else if (word == "dest=") {
		ss >> voyage->dest;
		response << word << std::fixed << std::setprecision(6) << voyage->dest;
	}
	else if (word == "timestep=") {
		ss >> voyage->timestep;
		response << word << voyage->timestep;
	}
	else if (word == "movement_factor=") {
		ss >> voyage->movement_factor;
		response << word << voyage->movement_factor;
	}
	else if (word == "alpha=") {
		ss >> voyage->alpha;
		response << word << std::fixed << std::setprecision(2) << voyage->alpha;
	}
	else if (word == "altitude=") {
		ss >> voyage->altitude;
		response << word << std::fixed << std::setprecision(2) << voyage->altitude;
	}
	else if (word == "range=") {
		ss >> voyage->range;
		response << word << std::fixed << std::setprecision(2) << voyage->range;
	}
	else if (word == "sail_open=") {
		ss >> voyage->sail_open;
		response << word << voyage->sail_open;
	}
	else if (word == "dir=") {
		if (voyage->dir) {
			ss >> *voyage->dir;
			response << word << std::fixed << std::setprecision(2) << *voyage->dir;
		}
	}

	/** **/

	else if (word == "help") {
		response <<
		"####################################\n"
		"############ General commands:\n"
		"echo				(message)\n"
		"send				(message)\n"
		"run\n"
		"help\n"
		"############ Variables:\n"
		"orig\n"
		"dest\n"
		"timestep\n"
		"movement_factor\n"
		"alpha\n"
		"altitude\n"
		"range\n"
		"sail_open\n"
		"dir\n"
		"############ Assignment commands:\n"
		"orig=          	(lat) (lon)\n"
		"dest=          	(lat) (lon)\n"
		"timestep=			(size of timestep in seconds)\n"
		"movement_factor=	(movement_factor)\n"
		"alpha=				(alpha)\n"
		"altitude= 			(altitude in m)\n"
		"range= 			(range)\n"
		"sail_open= 		(0/1)\n"
		"dir= 				(u) (v)\n"
		"####################################"
		;
	}

	else {
		response << "Invalid command: \"" << word << "\"";
	}
	return response.str();
}

void recvCmd(connection_hdl hdl, std::string cmd)
{
	std::string response = parseCmd(cmd);
	std::cout << "\n[" << hdl.lock().get() << "] " << response << "\n";
	boost::replace_all(response, "\n", "</br>");
	boost::replace_all(response, "\t", " ");
	wsServer->sendMsg(hdl, response);
}
