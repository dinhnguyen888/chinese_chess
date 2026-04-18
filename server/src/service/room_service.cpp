#include "service/room_service.h"
#include "type/player.h"

using json = nlohmann::json;

void RoomService::broadcast(const RoomState& room, const json& msg) {
    if (room.host) room.host->send_json(msg);
    if (room.guest) room.guest->send_json(msg);
}

void RoomService::start_game(RoomState& room) {
    room.is_playing = true;
    if (!is_full(room)) return;

    pair_players(room.host, room.guest, room.id);
}

void RoomService::pair_players(std::shared_ptr<Player> a, std::shared_ptr<Player> b, const std::string& room_id) {
    if (!a || !b) return;

    a->attach_opponent(b);
    b->attach_opponent(a);

    constexpr int kRedFirst = 1;
    json msg_a = {{"type", "matched"}, {"color", "r"}, {"orderSide", kRedFirst}, {"opponentName", b->name}};
    json msg_b = {{"type", "matched"}, {"color", "b"}, {"orderSide", kRedFirst}, {"opponentName", a->name}};
    
    if (!room_id.empty()) {
        msg_a["roomId"] = room_id;
        msg_b["roomId"] = room_id;
    }

    a->send_json(msg_a);
    b->send_json(msg_b);
}

bool RoomService::is_full(const RoomState& room) {
    return room.host != nullptr && room.guest != nullptr;
}

json RoomService::to_json(const RoomState& room) {
    return {
        {"id", room.id},
        {"name", room.name},
        {"host", room.host ? room.host->name : "Unknown"},
        {"guest", room.guest ? room.guest->name : ""},
        {"isPlaying", room.is_playing},
        {"autoStart", room.auto_start}
    };
}
