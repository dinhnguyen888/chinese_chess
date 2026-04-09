#include "utils/jwt_utils.h"
#include <jwt-cpp/jwt.h>
#include <jwt-cpp/traits/nlohmann-json/defaults.h>
#include <iostream>

namespace utils {
namespace jwt {

const std::string JWT_SECRET = "super_secret_key_123";

std::string generate_token(const std::string& username) {
    auto token = ::jwt::create()
        .set_issuer("chinese_chess")
        .set_type("JWS")
        .set_payload_claim("username", ::jwt::claim(username))
        .set_expires_at(std::chrono::system_clock::now() + std::chrono::hours(24 * 7))
        .sign(::jwt::algorithm::hs256{JWT_SECRET});
    return token;
}

std::string decode_and_verify(const std::string& token) {
    try {
        auto decoded = ::jwt::decode(token);
        auto verifier = ::jwt::verify()
            .allow_algorithm(::jwt::algorithm::hs256{JWT_SECRET})
            .with_issuer("chinese_chess");
        verifier.verify(decoded);
        return decoded.get_payload_claim("username").as_string();
    } catch (const std::exception& e) {
        std::cerr << "JWT Verification Failed: " << e.what() << "\n";
        return "";
    }
}

}
}
