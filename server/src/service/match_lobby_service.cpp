#include "service/match_lobby_service.h"
#include "type/player.h"
#include "service/room_service.h"

using json = nlohmann::json;

std::shared_ptr<MatchLobbyService> g_lobby = nullptr;

void MatchLobbyService::register_player(const std::string& name, std::shared_ptr<Player> player) {
    std::lock_guard<std::mutex> lock(mutex_);
    online_players_[name] = player;
}

void MatchLobbyService::unregister_player(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    online_players_.erase(name);
}

void MatchLobbyService::notify_punishment(const std::string& target, const std::string& reason, const std::string& reporter, int ban_days, bool can_chat, bool can_create_room) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = online_players_.find(target);
    if (it != online_players_.end()) {
        auto player = it->second;
        // Cập nhật nóng quyền hạn vào đối tượng player đang online
        player->can_chat = can_chat;
        player->can_create_room = can_create_room;
        
        player->send_json(json{
            {"type", "punishment_notify"},
            {"reason", reason},
            {"reporter", reporter},
            {"ban_days", ban_days},
            {"can_chat", can_chat},
            {"can_create_room", can_create_room},
            {"message", "Bạn đã bị xử lý vi phạm. Vui lòng kiểm tra chi tiết để biết các hạn chế."}
        });
    }
}

void MatchLobbyService::try_pair(std::shared_ptr<Player> player) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!state_.waiting) {
        state_.waiting = std::move(player);
        state_.waiting->send_json(json{{"type", "queue"}, {"message", "waiting_for_opponent"}});
        return;
    }
    auto a = state_.waiting;
    state_.waiting.reset();
    auto b = std::move(player);
    RoomService::pair_players(a, b);
}

void MatchLobbyService::cancel_waiting(const std::shared_ptr<Player>& player) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_.waiting == player) state_.waiting.reset();
    leave_room_internal(player);
}

void MatchLobbyService::create_room(std::shared_ptr<Player> player, const std::string& room_name, bool auto_start) {
    std::lock_guard<std::mutex> lock(mutex_);
    leave_room_internal(player);

    std::string room_id = std::to_string(state_.next_room_id++);
    RoomState room;
    room.id = room_id;
    room.name = room_name;
    room.host = player;
    room.is_playing = false;
    room.auto_start = auto_start;

    state_.rooms[room_id] = room;
    player->current_room_id = room_id;

    player->send_json(json{{"type", "room_created"}, {"roomId", room_id}, {"roomName", room_name}, {"autoStart", auto_start}});
}

void MatchLobbyService::join_room(std::shared_ptr<Player> player, const std::string& room_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = state_.rooms.find(room_id);
    if (it == state_.rooms.end()) {
        player->send_json(json{{"type", "error"}, {"message", "Phòng không tồn tại"}});
        return;
    }

    RoomState& room = it->second;

    if (room.host == player) {
        player->send_json(json{{"type", "error"}, {"message", "Bạn là chủ phòng này"}});
        return;
    }

    if (RoomService::is_full(room) || room.is_playing) {
        player->send_json(json{{"type", "error"}, {"message", "Phòng đã đầy"}});
        return;
    }

    room.guest = player;
    player->current_room_id = room_id;

    player->send_json(json{{"type", "room_joined"}, {"roomId", room_id}, {"hostName", room.host ? room.host->name : "Unknown"}});

    if (room.host) {
        room.host->send_json(json{{"type", "guest_joined"}, {"guestName", player->name}, {"roomId", room_id}});
    }

    if (room.auto_start) {
        RoomService::start_game(room);
    }
}

void MatchLobbyService::start_game(std::shared_ptr<Player> player, const std::string& room_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = state_.rooms.find(room_id);
    if (it == state_.rooms.end()) {
        player->send_json(json{{"type", "error"}, {"message", "Phòng không tồn tại"}});
        return;
    }

    RoomState& room = it->second;

    if (room.host != player) {
        player->send_json(json{{"type", "error"}, {"message", "Chỉ chủ phòng mới có thể bắt đầu"}});
        return;
    }

    if (!RoomService::is_full(room)) {
        player->send_json(json{{"type", "error"}, {"message", "Chưa đủ người chơi để bắt đầu"}});
        return;
    }

    RoomService::start_game(room);
}

void MatchLobbyService::get_room_list(std::shared_ptr<Player> player) {
    search_room(player, "");
}

void MatchLobbyService::search_room(std::shared_ptr<Player> player, const std::string& query) {
    std::lock_guard<std::mutex> lock(mutex_);
    json room_list = json::array();
    for (const auto& pair : state_.rooms) {
        const RoomState& r = pair.second;
        if (query.empty() || r.name.find(query) != std::string::npos || r.id == query) {
            room_list.push_back(RoomService::to_json(r));
        }
    }
    player->send_json(json{{"type", "room_list"}, {"rooms", room_list}});
}

void MatchLobbyService::chat_in_room(std::shared_ptr<Player> sender, const std::string& msg_text) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (sender->current_room_id.empty()) {
        sender->send_json(json{{"type", "error"}, {"message", "Bạn chưa vào phòng"}});
        return;
    }
    auto it = state_.rooms.find(sender->current_room_id);
    if (it != state_.rooms.end()) {
        RoomState& room = it->second;
        json chat_msg = {{"type", "chat"}, {"sender", sender->name}, {"message", msg_text}};
        RoomService::broadcast(room, chat_msg);
    }
}

void MatchLobbyService::leave_room_internal(const std::shared_ptr<Player>& player) {
    if (player->current_room_id.empty()) return;
    auto it = state_.rooms.find(player->current_room_id);
    if (it != state_.rooms.end()) {
        RoomState& room = it->second;
        if (room.host == player) {
            if (room.guest) {
                room.guest->current_room_id = "";
                room.guest->send_json(json{{"type", "opponent_left"}});
            }
            state_.rooms.erase(it);
        } else if (room.guest == player) {
            room.guest = nullptr;
            room.is_playing = false;
            if (room.host) {
                room.host->send_json(json{{"type", "guest_left"}, {"roomId", room.id}});
            }
        }
    }
    player->current_room_id = "";
}
