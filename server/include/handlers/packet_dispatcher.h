#pragma once
#include <memory>
#include <string>
#include <functional>
#include <unordered_map>
#include <nlohmann/json.hpp>

class Player;
class MatchLobbyService;

class PacketDispatcher {
public:
    using HandlerFunc = std::function<void(std::shared_ptr<Player>, MatchLobbyService&, const nlohmann::json&)>;

    static PacketDispatcher& instance() {
        static PacketDispatcher dispatcher;
        return dispatcher;
    }

    void register_handler(const std::string& type, HandlerFunc func);
    void dispatch(std::shared_ptr<Player> player, MatchLobbyService& lobby, const std::string& text);

private:
    PacketDispatcher();
    std::unordered_map<std::string, HandlerFunc> handlers_;
};
