#pragma once

#include "error.hpp"
#include "types.hpp"
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>

namespace xiaopeng {
namespace loader {

enum class UrlScheme {
  Http,
  Https,
  Ftp,
  Ws,
  Wss,
  Data,
  Blob,
  File,
  About,
  Unknown
};

class Url {
public:
  Url() = default;

  explicit Url(const std::string &url) { parseInternal(url); }

  static Result<Url> parse(const std::string &url) {
    Url result;
    if (result.parseInternal(url)) {
      return result;
    }
    return Result<Url>(ErrorCode::UrlParseError, "Failed to parse URL: " + url);
  }

  static Url fromParts(UrlScheme scheme, const std::string &host,
                       std::uint16_t port, const std::string &path = "/",
                       const std::string &query = "",
                       const std::string &fragment = "",
                       const std::string &userinfo = "") {
    Url url;
    url.scheme_ = scheme;
    url.host_ = host;
    url.port_ = port;
    url.path_ = path.empty() ? "/" : path;
    url.query_ = query;
    url.fragment_ = fragment;
    url.userinfo_ = userinfo;
    url.valid_ = true;
    return url;
  }

  bool isValid() const { return valid_; }

  UrlScheme scheme() const { return scheme_; }
  const std::string &schemeString() const { return schemeStr_; }
  const std::string &host() const { return host_; }
  std::uint16_t port() const { return port_; }
  std::uint16_t defaultPort() const {
    switch (scheme_) {
    case UrlScheme::Http:
      return 80;
    case UrlScheme::Https:
      return 443;
    case UrlScheme::Ftp:
      return 21;
    case UrlScheme::Ws:
      return 80;
    case UrlScheme::Wss:
      return 443;
    default:
      return 0;
    }
  }
  const std::string &path() const { return path_; }
  const std::string &query() const { return query_; }
  const std::string &fragment() const { return fragment_; }
  const std::string &userinfo() const { return userinfo_; }

  std::string authority() const {
    std::string result;
    if (!userinfo_.empty()) {
      result += userinfo_ + "@";
    }
    result += host_;
    if (port_ != defaultPort() && port_ != 0) {
      result += ":" + std::to_string(port_);
    }
    return result;
  }

  std::string pathWithQuery() const {
    std::string result = path_;
    if (!query_.empty()) {
      result += "?" + query_;
    }
    return result;
  }

  std::string toString() const {
    if (!valid_)
      return "";

    std::string result = schemeStr_ + "://";
    result += authority();
    result += path_;

    if (!query_.empty()) {
      result += "?" + query_;
    }
    if (!fragment_.empty()) {
      result += "#" + fragment_;
    }

    return result;
  }

  std::string origin() const {
    if (!valid_)
      return "";

    // Origin only includes scheme, host, and port (if not default)
    std::string result = schemeStr_ + "://" + host_;
    if (port_ != defaultPort() && port_ != 0) {
      result += ":" + std::to_string(port_);
    }
    return result;
  }

  bool isSecure() const {
    return scheme_ == UrlScheme::Https || scheme_ == UrlScheme::Wss;
  }

  bool sameOrigin(const Url &other) const {
    if (!valid_ || !other.valid_)
      return false;
    return scheme_ == other.scheme_ && host_ == other.host_ &&
           port_ == other.port_;
  }

  Url resolve(const std::string &relative) const {
    if (!valid_)
      return Url();

    if (relative.empty())
      return *this;

    auto parsed = Url::parse(relative);
    if (parsed.isOk()) {
      return parsed.unwrap();
    }

    if (relative[0] == '/') {
      Url result = *this;
      if (relative.size() > 1 && relative[1] == '/') {
        // Network-path reference: //host/path
        // Re-parse with same scheme
        Url temp = Url(schemeStr_ + ":" + relative);
        if (temp.isValid()) {
          return temp;
        }
        // Fallback if parse failed (unlikely)
        result.host_ = relative.substr(2);
        result.path_ = "/";
      } else {
        result.path_ = relative;
      }
      return result;
    }

    if (relative[0] == '?') {
      Url result = *this;
      result.query_ = relative.substr(1);
      result.fragment_.clear();
      return result;
    }

    if (relative[0] == '#') {
      Url result = *this;
      result.fragment_ = relative.substr(1);
      return result;
    }

    Url result = *this;
    size_t lastSlash = result.path_.rfind('/');
    if (lastSlash != std::string::npos) {
      result.path_ = result.path_.substr(0, lastSlash + 1) + relative;
    } else {
      result.path_ = "/" + relative;
    }
    result.query_.clear();
    result.fragment_.clear();

    return normalizePath(result);
  }

  static std::string encode(const std::string &input) {
    std::ostringstream result;
    result.fill('0');
    result << std::hex;

    for (char c : input) {
      if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' ||
          c == '.' || c == '~') {
        result << c;
      } else {
        result << std::uppercase;
        result << '%' << std::setw(2)
               << static_cast<int>(static_cast<unsigned char>(c));
        result << std::nouppercase;
      }
    }

    return result.str();
  }

  static std::string decode(const std::string &input) {
    std::string result;
    result.reserve(input.size());

    for (size_t i = 0; i < input.size(); ++i) {
      if (input[i] == '%' && i + 2 < input.size()) {
        std::string hex = input.substr(i + 1, 2);
        try {
          int value = std::stoi(hex, nullptr, 16);
          result += static_cast<char>(value);
          i += 2;
        } catch (...) {
          result += input[i];
          // Do not increment i by 2 here, so next char is processed normally
        }
      } else if (input[i] == '+') {
        result += ' ';
      } else {
        result += input[i];
      }
    }

    return result;
  }

  bool operator==(const Url &other) const {
    return toString() == other.toString();
  }

  bool operator!=(const Url &other) const { return !(*this == other); }

  bool operator<(const Url &other) const {
    return toString() < other.toString();
  }

private:
  bool parseInternal(const std::string &url) {
    valid_ = false;

    if (url.empty())
      return false;

    size_t schemeEnd = url.find("://");
    if (schemeEnd == std::string::npos) {
      return false;
    }

    schemeStr_ = url.substr(0, schemeEnd);
    std::transform(schemeStr_.begin(), schemeStr_.end(), schemeStr_.begin(),
                   ::tolower);
    scheme_ = parseScheme(schemeStr_);

    size_t hostStart = schemeEnd + 3;
    size_t pathStart = url.find('/', hostStart);
    size_t queryStart = url.find('?', hostStart);
    size_t fragmentStart = url.find('#', hostStart);

    size_t authorityEnd = url.size();
    if (pathStart != std::string::npos)
      authorityEnd = std::min(authorityEnd, pathStart);
    if (queryStart != std::string::npos)
      authorityEnd = std::min(authorityEnd, queryStart);
    if (fragmentStart != std::string::npos)
      authorityEnd = std::min(authorityEnd, fragmentStart);

    std::string authority = url.substr(hostStart, authorityEnd - hostStart);

    size_t atPos = authority.find('@');
    if (atPos != std::string::npos) {
      userinfo_ = authority.substr(0, atPos);
      authority = authority.substr(atPos + 1);
    }

    size_t colonPos = authority.rfind(':');
    size_t bracketPos = authority.rfind(']');

    if (colonPos != std::string::npos &&
        (bracketPos == std::string::npos || colonPos > bracketPos)) {
      host_ = authority.substr(0, colonPos);
      try {
        port_ = static_cast<std::uint16_t>(
            std::stoi(authority.substr(colonPos + 1)));
      } catch (...) {
        return false;
      }
    } else {
      host_ = authority;
      port_ = defaultPort();
    }

    if (!host_.empty() && host_[0] == '[' && host_.back() == ']') {
      host_ = host_.substr(1, host_.size() - 2);
    }

    if (host_.empty() && scheme_ != UrlScheme::File &&
        scheme_ != UrlScheme::About) {
      return false;
    }

    if (pathStart != std::string::npos && pathStart < authorityEnd) {
      pathStart = authorityEnd;
    }

    if (pathStart != std::string::npos && pathStart < url.size()) {
      size_t pathEnd = url.size();
      if (queryStart != std::string::npos)
        pathEnd = std::min(pathEnd, queryStart);
      if (fragmentStart != std::string::npos)
        pathEnd = std::min(pathEnd, fragmentStart);
      path_ = url.substr(pathStart, pathEnd - pathStart);
    } else {
      path_ = "/";
    }

    if (queryStart != std::string::npos && queryStart + 1 < url.size()) {
      size_t queryEnd =
          fragmentStart != std::string::npos ? fragmentStart : url.size();
      query_ = url.substr(queryStart + 1, queryEnd - queryStart - 1);
    }

    if (fragmentStart != std::string::npos && fragmentStart + 1 < url.size()) {
      fragment_ = url.substr(fragmentStart + 1);
    }

    valid_ = true;
    return true;
  }

  static UrlScheme parseScheme(const std::string &scheme) {
    if (scheme == "http")
      return UrlScheme::Http;
    if (scheme == "https")
      return UrlScheme::Https;
    if (scheme == "ftp")
      return UrlScheme::Ftp;
    if (scheme == "ws")
      return UrlScheme::Ws;
    if (scheme == "wss")
      return UrlScheme::Wss;
    if (scheme == "data")
      return UrlScheme::Data;
    if (scheme == "blob")
      return UrlScheme::Blob;
    if (scheme == "file")
      return UrlScheme::File;
    if (scheme == "about")
      return UrlScheme::About;
    return UrlScheme::Unknown;
  }

  static Url normalizePath(Url url) {
    std::vector<std::string> segments;
    std::string path = url.path_;

    if (path.empty() || path[0] != '/') {
      path = "/" + path;
    }

    std::istringstream iss(path);
    std::string segment;

    while (std::getline(iss, segment, '/')) {
      if (segment.empty() || segment == ".") {
        continue;
      } else if (segment == "..") {
        if (!segments.empty()) {
          segments.pop_back();
        }
      } else {
        segments.push_back(segment);
      }
    }

    std::string normalized = "/";
    for (size_t i = 0; i < segments.size(); ++i) {
      if (i > 0)
        normalized += "/";
      normalized += segments[i];
    }

    url.path_ = normalized;
    return url;
  }

  UrlScheme scheme_ = UrlScheme::Unknown;
  std::string schemeStr_;
  std::string host_;
  std::uint16_t port_ = 0;
  std::string path_ = "/";
  std::string query_;
  std::string fragment_;
  std::string userinfo_;
  bool valid_ = false;
};

} // namespace loader
} // namespace xiaopeng
