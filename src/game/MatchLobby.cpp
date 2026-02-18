#include "game/MatchLobby.h"
#include "network/WsSession.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// ─── Quick Match (random pair) ────────────────────────────────────────────────
void MatchLobby::try_pair(std::shared_ptr<WsSession> session) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!waiting_) {
        waiting_ = std::move(session);
        waiting_->send_json(json{{"type", "queue"}, {"message", "waiting_for_opponent"}});
        return;
    }
    auto a = waiting_;
    waiting_.reset();
    auto b = std::move(session);
    a->attach_opponent(b);
    b->attach_opponent(a);
    constexpr int kRedFirst = 1;
    a->send_json(json{{"type", "matched"}, {"color", "r"}, {"orderSide", kRedFirst}, {"opponentName", b->name_}});
    b->send_json(json{{"type", "matched"}, {"color", "b"}, {"orderSide", kRedFirst}, {"opponentName", a->name_}});
}

void MatchLobby::cancel_waiting(const std::shared_ptr<WsSession>& session) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (waiting_ == session)
        waiting_.reset();
    leave_room_internal(session);
}

// ─── Room Management ──────────────────────────────────────────────────────────
void MatchLobby::create_room(std::shared_ptr<WsSession> session, const std::string& room_name, bool auto_start) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Leave any existing room first
    leave_room_internal(session);

    std::string room_id = std::to_string(next_room_id_++);
    Room room;
    room.id = room_id;
    room.name = room_name;
    room.host = session;
    room.is_playing = false;
    room.auto_start = auto_start;

    rooms_[room_id] = room;
    session->current_room_id_ = room_id;

    session->send_json(json{
        {"type", "room_created"},
        {"roomId", room_id},
        {"roomName", room_name},
        {"autoStart", auto_start}
    });
}

void MatchLobby::join_room(std::shared_ptr<WsSession> session, const std::string& room_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = rooms_.find(room_id);
    if (it == rooms_.end()) {
        session->send_json(json{{"type", "error"}, {"message", "Phòng không tồn tại"}});
        return;
    }

    Room& room = it->second;

    // Prevent host from joining their own room
    if (room.host == session) {
        session->send_json(json{{"type", "error"}, {"message", "Bạn là chủ phòng này"}});
        return;
    }

    if (room.guest || room.is_playing) {
        session->send_json(json{{"type", "error"}, {"message", "Phòng đã đầy"}});
        return;
    }

    room.guest = session;
    session->current_room_id_ = room_id;

    // Notify guest they joined, waiting for host to start
    session->send_json(json{
        {"type", "room_joined"},
        {"roomId", room_id},
        {"hostName", room.host ? room.host->name_ : "Unknown"}
    });

    // Notify host that a guest has joined
    if (room.host) {
        room.host->send_json(json{
            {"type", "guest_joined"},
            {"guestName", session->name_},
            {"roomId", room_id}
        });
    }

    // Auto-start if the flag is set
    if (room.auto_start) {
        do_start_game(room);
    }
}

void MatchLobby::start_game(std::shared_ptr<WsSession> session, const std::string& room_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = rooms_.find(room_id);
    if (it == rooms_.end()) {
        session->send_json(json{{"type", "error"}, {"message", "Phòng không tồn tại"}});
        return;
    }

    Room& room = it->second;

    // Only the host can start the game
    if (room.host != session) {
        session->send_json(json{{"type", "error"}, {"message", "Chỉ chủ phòng mới có thể bắt đầu"}});
        return;
    }

    if (!room.guest) {
        session->send_json(json{{"type", "error"}, {"message", "Chưa có người chơi vào phòng"}});
        return;
    }

    do_start_game(room);
}

void MatchLobby::do_start_game(Room& room) {
    room.is_playing = true;

    auto a = room.host;
    auto b = room.guest;

    a->attach_opponent(b);
    b->attach_opponent(a);

    constexpr int kRedFirst = 1;
    a->send_json(json{{"type", "matched"}, {"color", "r"}, {"orderSide", kRedFirst}, {"opponentName", b->name_}, {"roomId", room.id}});
    b->send_json(json{{"type", "matched"}, {"color", "b"}, {"orderSide", kRedFirst}, {"opponentName", a->name_}, {"roomId", room.id}});
}

// ─── Room Listing / Search ────────────────────────────────────────────────────
void MatchLobby::get_room_list(std::shared_ptr<WsSession> session) {
    std::lock_guard<std::mutex> lock(mutex_);
    json room_list = json::array();
    for (const auto& pair : rooms_) {
        const Room& r = pair.second;
        room_list.push_back({
            {"id", r.id},
            {"name", r.name},
            {"host", r.host ? r.host->name_ : "Unknown"},
            {"guest", r.guest ? r.guest->name_ : ""},
            {"isPlaying", r.is_playing},
            {"autoStart", r.auto_start}
        });
    }
    session->send_json(json{{"type", "room_list"}, {"rooms", room_list}});
}

void MatchLobby::search_room(std::shared_ptr<WsSession> session, const std::string& query) {
    std::lock_guard<std::mutex> lock(mutex_);
    json room_list = json::array();
    for (const auto& pair : rooms_) {
        const Room& r = pair.second;
        if (r.name.find(query) != std::string::npos || r.id == query) {
            room_list.push_back({
                {"id", r.id},
                {"name", r.name},
                {"host", r.host ? r.host->name_ : "Unknown"},
                {"guest", r.guest ? r.guest->name_ : ""},
                {"isPlaying", r.is_playing},
                {"autoStart", r.auto_start}
            });
        }
    }
    session->send_json(json{{"type", "room_list"}, {"rooms", room_list}});
}

// ─── Chat ─────────────────────────────────────────────────────────────────────
void MatchLobby::chat_in_room(std::shared_ptr<WsSession> sender, const std::string& msg_text) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (sender->current_room_id_.empty()) {
        sender->send_json(json{{"type", "error"}, {"message", "Bạn chưa vào phòng"}});
        return;
    }
    auto it = rooms_.find(sender->current_room_id_);
    if (it != rooms_.end()) {
        Room& room = it->second;
        json chat_msg = {{"type", "chat"}, {"sender", sender->name_}, {"message", msg_text}};
        if (room.host) room.host->send_json(chat_msg);
        if (room.guest) room.guest->send_json(chat_msg);
    }
}

// ─── Internal Cleanup ─────────────────────────────────────────────────────────
void MatchLobby::leave_room_internal(const std::shared_ptr<WsSession>& session) {
    if (session->current_room_id_.empty()) return;
    auto it = rooms_.find(session->current_room_id_);
    if (it != rooms_.end()) {
        Room& room = it->second;
        if (room.host == session) {
            // Host left → notify guest and delete room
            if (room.guest) {
                room.guest->current_room_id_ = "";
                room.guest->send_json(json{{"type", "opponent_left"}});
            }
            rooms_.erase(it);
        } else if (room.guest == session) {
            room.guest = nullptr;
            room.is_playing = false;
            // Notify host that guest left
            if (room.host) {
                room.host->send_json(json{{"type", "guest_left"}, {"roomId", room.id}});
            }
        }
    }
    session->current_room_id_ = "";
}
