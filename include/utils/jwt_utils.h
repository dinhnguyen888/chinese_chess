#pragma once
#include <jwt-cpp/jwt.h>
#include <jwt-cpp/traits/nlohmann-json/defaults.h>
#include <iostream>

namespace utils {
namespace jwt {

inline const std::string JWT_SECRET = "super_secret_key_123";

inline std::string generate_token(const std::string& username, const std::string& role = "user") {
    auto token = ::jwt::create()
        .set_issuer("chinese_chess")
        .set_type("JWS")
        .set_payload_claim("username", ::jwt::claim(username))
        .set_payload_claim("role", ::jwt::claim(role))
        .set_expires_at(std::chrono::system_clock::now() + std::chrono::hours(24 * 7))
        .sign(::jwt::algorithm::hs256{JWT_SECRET});
    return token;
}

inline std::string decode_and_verify(const std::string& token) {
    try {
        auto decoded = ::jwt::decode(token);
        auto verifier = ::jwt::verify()
            .allow_algorithm(::jwt::algorithm::hs256{JWT_SECRET})
            .with_issuer("chinese_chess");
        verifier.verify(decoded);
        return decoded.get_payload_claim("username").as_string();
    } catch (...) {
        return "";
    }
}

inline std::string extract_role(const std::string& token) {
    try {
        auto decoded = ::jwt::decode(token);
        return decoded.get_payload_claim("role").as_string();
    } catch (...) {
        return "user";
    }
}

}
}
