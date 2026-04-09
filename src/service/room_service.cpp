#include "service/room_service.h"
#include "type/player.h"

using json = nlohmann::json;

void RoomService::broadcast(const RoomState& room, const json& msg) {
    if (room.host) room.host->send_json(msg);
    if (room.guest) room.guest->send_json(msg);
}

void RoomService::start_game(RoomState& room) {
    room.is_playing = true;
    if (!room.host || !room.guest) return;

    room.host->attach_opponent(room.guest);
    room.guest->attach_opponent(room.host);

    constexpr int kRedFirst = 1;
    room.host->send_json(json{{"type", "matched"}, {"color", "r"}, {"orderSide", kRedFirst}, {"opponentName", room.guest->name}, {"roomId", room.id}});
    room.guest->send_json(json{{"type", "matched"}, {"color", "b"}, {"orderSide", kRedFirst}, {"opponentName", room.host->name}, {"roomId", room.id}});
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
