#include "handlers/lobby_handler.h"
#include "service/match_lobby_service.h"

using json = nlohmann::json;

void LobbyHandler::handle(std::shared_ptr<Player> player, MatchLobbyService& lobby, const json& msg, const std::string& type) {
    if (type == "find_match") {
        lobby.try_pair(player);
    } else if (type == "create_room") {
        bool auto_start = msg.value("autoStart", false);
        lobby.create_room(player, msg.value("roomName", "My Room"), auto_start);
    } else if (type == "join_room") {
        lobby.join_room(player, msg.value("roomId", ""));
    } else if (type == "start_game") {
        lobby.start_game(player, msg.value("roomId", ""));
    } else if (type == "list_rooms") {
        lobby.get_room_list(player);
    } else if (type == "search_room") {
        lobby.search_room(player, msg.value("query", ""));
    } else if (type == "chat") {
        lobby.chat_in_room(player, msg.value("message", ""));
    }
}
