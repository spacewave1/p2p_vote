//
// Created by wnabo on 09.11.2021.
//

#ifndef VOTE_P2P_STRAIGHTLINESYNCTHREAD_H
#define VOTE_P2P_STRAIGHTLINESYNCTHREAD_H

#include "abstractThread.h"
#include "logger.h"
#include <set>
#include <map>
#include <string>

class straightLineSyncThread : public abstractThread {
private:
    logger _logger = _logger.Instance();
    void InternalThreadEntry() override;
    void initSyncProcedure();
    void syncForwardProcedure();
    void* arg;
    std::map<std::string, std::string> connection_table;
    std::set<std::string> peers;
    std::string initial_receiver_address;
public:
    const std::map <std::string, std::string> &getConnectionTable() const;
    const std::set <std::string> &getPeers() const;
    void setParams(void* arg, std::map<std::string, std::string> connection_table, std::set<std::string> peers, std::string initial_receiver_address = "");
};


#endif //VOTE_P2P_STRAIGHTLINESYNCTHREAD_H
