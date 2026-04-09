#pragma once
#include <string>

namespace utils {
namespace jwt {

std::string generate_token(const std::string& username);
std::string decode_and_verify(const std::string& token);

}
}
