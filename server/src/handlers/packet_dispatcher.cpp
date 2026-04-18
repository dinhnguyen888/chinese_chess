#include "handlers/packet_dispatcher.h"
#include "handlers/lobby_handler.h"
#include "handlers/game_handler.h"
#include "type/player.h"
#include "service/match_lobby_service.h"

using json = nlohmann::json;

PacketDispatcher::PacketDispatcher() {
    handlers_["find_match"] = [](auto player, auto& lobby, const auto& msg) { LobbyHandler::handle(player, lobby, msg, "find_match"); };
    handlers_["create_room"]= [](auto player, auto& lobby, const auto& msg) { LobbyHandler::handle(player, lobby, msg, "create_room"); };
    handlers_["join_room"]  = [](auto player, auto& lobby, const auto& msg) { LobbyHandler::handle(player, lobby, msg, "join_room"); };
    handlers_["start_game"] = [](auto player, auto& lobby, const auto& msg) { LobbyHandler::handle(player, lobby, msg, "start_game"); };
    handlers_["list_rooms"] = [](auto player, auto& lobby, const auto& msg) { LobbyHandler::handle(player, lobby, msg, "list_rooms"); };
    handlers_["search_room"]= [](auto player, auto& lobby, const auto& msg) { LobbyHandler::handle(player, lobby, msg, "search_room"); };
    handlers_["chat"]       = [](auto player, auto& lobby, const auto& msg) { LobbyHandler::handle(player, lobby, msg, "chat"); };

    handlers_["move"]       = [](auto player, auto& lobby, const auto& msg) { GameHandler::handle(player, msg, "move"); };
    handlers_["get_history"]= [](auto player, auto& lobby, const auto& msg) { GameHandler::handle(player, msg, "get_history"); };
    handlers_["get_replay"] = [](auto player, auto& lobby, const auto& msg) { GameHandler::handle(player, msg, "get_replay"); };
    handlers_["game_result"]= [](auto player, auto& lobby, const auto& msg) { GameHandler::handle(player, msg, "game_result"); };

    handlers_["ping"]       = [](auto player, auto& lobby, const auto& msg) { player->send_json(json{{"type", "pong"}}); };
}

void PacketDispatcher::register_handler(const std::string& type, HandlerFunc func) {
    handlers_[type] = std::move(func);
}

void PacketDispatcher::dispatch(std::shared_ptr<Player> player, MatchLobbyService& lobby, const std::string& text) {
    json msg;
    try {
        msg = json::parse(text);
    } catch (const json::parse_error&) {
        player->send_json(json{{"type", "error"}, {"message", "invalid_json"}});
        return;
    }

    const std::string type = msg.value("type", "");

    // Enforcement
    if (type == "chat" && !player->can_chat) {
        player->send_json(json{{"type", "error"}, {"message", "Bạn đã bị cấm chat"}});
        return;
    }
    if ((type == "create_room" || type == "find_match") && !player->can_create_room) {
        player->send_json(json{{"type", "error"}, {"message", "Bạn đã bị cấm tạo phòng hoặc tìm trận"}});
        return;
    }

    if (msg.contains("name")) {
        player->name = msg.value("name", player->name.empty() ? "Player" : player->name);
    }

    auto it = handlers_.find(type);
    if (it != handlers_.end()) {
        it->second(player, lobby, msg);
    } else {
        player->send_json(json{{"type", "error"}, {"message", "unknown_type"}});
    }
}
