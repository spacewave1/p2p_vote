//
// Created by wnabo on 31.10.2021.
//

#include <iostream>
#include <zmq.hpp>
#include <regex>
#include <nlohmann/json.hpp>
#include "peer.h"
#include <fstream>


void peer::printConnections() {
    // Iterate through "receiver" nodes
    size_t index = 0;
    std::cout << "Host Ip Addresses" << std::endl;
    std::for_each(known_peer_addresses.begin(), known_peer_addresses.end(), [&index](const std::string ip_address) {
        std::cout << "[" << index << "] " << ip_address << std::endl;
        index++;
    });

    // Iterate through connections
    index = 0;
    std::cout << "Connections" << std::endl;
    std::for_each(connection_table.begin(), connection_table.end(),
                  [&index](const std::pair<std::string, std::string> connection) {
                      std::cout << "[" << index << "] " << connection.first << "->" << connection.second << std::endl;
                      index++;
                  });

    nlohmann::json send_json = nlohmann::json();
    send_json["nodes"] = nlohmann::ordered_json(known_peer_addresses);
    send_json["connections"] = nlohmann::ordered_json(connection_table);
    std::cout << "As Json:" << std::endl << nlohmann::to_string(send_json) << std::endl;
}

void printMetaData(zmq::message_t &msg) {
    std::cout << std::endl;
    std::cout << "Socket-Type: ";
    try {
        std::cout << msg.gets("Socket-Type") << std::endl;
    } catch (zmq::error_t er) {
        std::cout << "[]" << std::endl;
    }

    std::cout << "Peer-address: ";
    try {
        std::cout << msg.gets("Peer-Address") << std::endl;
    } catch (zmq::error_t er) {
        std::cout << "[]" << std::endl;
    }

    std::cout << "User-Id: ";
    try {
        std::cout << msg.gets("User-Id") << std::endl;
    } catch (zmq::error_t er) {
        std::cout << "[]" << std::endl;
    }

    std::cout << "Routing-Id: ";
    try {
        std::cout << msg.gets("Routing-Id") << std::endl;
    } catch (zmq::error_t er) {
        std::cout << "[]" << std::endl;
    }
    std::cout << std::endl;
}

void peer::connect(std::string& input, void *abstractContext) {
    std::cout << "is connecting to " << input << std::endl;
    const std::string delimiter = " ";
    size_t position_of_whitespace = input.find(delimiter);
    auto address = input.substr(position_of_whitespace + delimiter.size(), input.size() - position_of_whitespace);

    zmq::context_t *zmq_context = (zmq::context_t *) abstractContext;
    zmq::socket_t sock(*zmq_context, zmq::socket_type::req);

    sock.connect("tcp://" + std::string(address) + ":5555");

    sock.send(zmq::message_t(address), zmq::send_flags::none);

    zmq::message_t reply;
    sock.recv(reply, zmq::recv_flags::none);
    if (!reply.empty()) {
        std::cout << reply.to_string() << std::endl;
        if (std::regex_match(reply.to_string(),
                             std::regex("[0-9]{1,3}[.][0-9]{1,3}[.][0-9]{1,3}[.][0-9]{1,3}"))) {
            printMetaData(reply);
            std::cout << "added ip" << std::endl;

            peer_address = std::string(address);
            known_peer_addresses.insert(std::string(address));
            connection_table[reply.to_string()] = std::string(address);
        }
    }
}

void peer::receive(void *abstractContext) {
    std::cout << "Wait for connection" << std::endl;
    zmq::context_t *zmq_context = (zmq::context_t *) abstractContext;
    zmq::socket_t sock(*zmq_context, zmq::socket_type::rep);

    zmq::version();
    sock.bind("tcp://*:5555");
    zmq::message_t request;

    sock.recv(request, zmq::recv_flags::none);

    if (!request.empty()) {
        if (!known_peer_addresses.contains(request.to_string())) {
            if (std::regex_match(request.to_string(),
                                 std::regex("[0-9]{1,3}[.][0-9]{1,3}[.][0-9]{1,3}[.][0-9]{1,3}"))) {

                // Add to connection to topology
                known_peer_addresses.insert(request.to_string());
                connection_table.insert(
                        std::make_pair(std::string(request.gets("Peer-Address")), std::string(request.to_string())));

                printMetaData(request);
                std::cout << "added ip " << request.to_string() << " to list of network" << std::endl;
            } else {
                std::cout << "not an ip4" << std::endl;
            }
            sock.send(zmq::message_t(std::string(request.gets("Peer-Address"))), zmq::send_flags::none);
        } else {
            std::cout << "already contains ip" << std::endl;
        }
    }
}

peer::~peer() {
    connection_table.clear();
    known_peer_addresses.clear();
    peer_address = nullptr;
}

peer::peer() {
    connection_table = std::map<std::string, std::string>();
    known_peer_addresses = std::set<std::string>();
    peer_address = "";
}

void peer::initSyncThread(void* context, straightLineSyncThread& thread, std::string initial_receiver_address){
    thread.setParams(context, connection_table, known_peer_addresses, initial_receiver_address);
    thread.StartInternalThread();
}

const std::set <std::string> &peer::getKnownPeerAddresses() const {
    return known_peer_addresses;
}

void peer::setKnownPeerAddresses(const std::set <std::string> &known_peer_addresses) {
    peer::known_peer_addresses = known_peer_addresses;
}

const std::map <std::string, std::string> &peer::getConnectionTable() const {
    return connection_table;
}

void peer::setConnectionTable(const std::map <std::string, std::string> &connection_table) {
    peer::connection_table = connection_table;
}

void peer::exportPeerConnections(std::string exportPath) {
    std::ofstream exportStream;
    exportStream.open (exportPath + "connections.json");
    nlohmann::json connectionsJson = nlohmann::json();
    connectionsJson["connections"] = nlohmann::ordered_json(connection_table);
    exportStream << connectionsJson.dump() << "\n";
    exportStream.close();
}

void peer::exportPeersList(std::string exportPath) {
    std::ofstream exportStream;
    exportStream.open (exportPath + "peers.json");
    nlohmann::json peersJson = nlohmann::json();
    peersJson["peers"] = nlohmann::ordered_json(known_peer_addresses);
    exportStream << peersJson.dump() << "\n";
    exportStream.close();
}

void peer::importPeerConnections(std::string importPath) {
    std::ifstream importStream;
    std::string line;

    std::cout << "is importing connections file from " << importPath + "connections.json" << std::endl;

    // File Open in the Read Mode
    importStream.open(importPath + "connections.json");

    if(importStream.is_open())
    {
        if(getline(importStream, line)) {
            std::cout << line << std::endl;

            nlohmann::json connectionsJson = nlohmann::json::parse(line);
            std::cout << "File contents: " << std::endl;
            std::cout << connectionsJson.dump() << std::endl;
            this->connection_table = connectionsJson["connections"].get<std::map<std::string, std::string>>();

        };
        // File Close
        importStream.close();
        std::cout << "Successfully imported connections" << std::endl;
    }
    else
    {
        std::cout << "Unable to open the file!" << std::endl;
    }
}

void peer::importPeersList(std::string importPath) {
    std::ifstream importStream;
    std::string line;

    std::cout << "is importing peers file from " << importPath + "peers.json" << std::endl;

    // File Open in the Read Mode
    importStream.open(importPath + "peers.json");

    if(importStream.is_open())
    {
        if(getline(importStream, line)) {
            std::cout << line << std::endl;

            nlohmann::json peersJson = nlohmann::json::parse(line);
            std::cout << "File contents: " << std::endl;
            std::cout << peersJson.dump() << std::endl;
            this->known_peer_addresses = peersJson["peers"].get<std::set<std::string>>();

        };
        // File Close
        importStream.close();
        std::cout << "Successfully imported peers" << std::endl;
    }
    else
    {
        std::cout << "Unable to open the file!" << std::endl;
    }
}