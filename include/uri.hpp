/** Brief: Uniform Resource Identifier (URI)
 *  Copyright (C) 2016  Carlos Guimaraes <cguimaraes@av.it.pt>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef URI__HPP_
#define URI__HPP_

#include "utils.hpp"

#include <regex>

class Uri {
private:
  std::string _schema;
  std::string _authority;
  std::string _path;
  std::string _query;
  std::string _fragment;

  bool _isValid = false;
  bool _isAbsolute = false;
  bool _isRelative = false;

public:
  Uri()
  { }

  Uri(std::string uri)
  {
    fromString(unescapeString(trimString(uri)));
    normalize();
  }

  Uri(const char* uri)
  {
    fromString(unescapeString(trimString(uri)));
    normalize();
  }

  Uri(Uri base, Uri ref)
  {
    if(!base.isValid() || !ref.isValid()) {
      return;
    }

    if(ref.isAbsolute()) {
      _isValid    = true;
      _isAbsolute = true;
      _isRelative = false;

      _schema    = ref.getSchema();
      _authority = ref.getAuthority();
      _path      = ref.getPath();
      _query     = ref.getQuery();
      _fragment  = ref.getFragment();
    }

    if(ref.isRelative()) {
      if(ref.getAuthority().size() != 0) {
        _isValid    = true;
        _isAbsolute = true;
        _isRelative = false;

        _schema    = base.getSchema();
        _authority = ref.getAuthority();
        _path      = ref.getPath();
      } else if(ref.getPath().size() != 0) {
        _isValid    = true;
        _isAbsolute = true;
        _isRelative = false;

        _schema    = base.getSchema();
        _authority = base.getAuthority();
        if(ref.getPath().find('/') == 0) {
          _path = ref.getPath();
        } else {
          size_t pos;
          if((pos = base.getPath().rfind('/')) != std::string::npos) {
            _path = base.getPath().substr(0, pos) + '/' + ref.getPath();
          } else {
            _path = '/' + ref.getPath();
          }
        }
        _query = ref.getQuery();
      } else {
        _isValid    = true;
        _isAbsolute = true;
        _isRelative = false;

        _schema    = base.getSchema();
        _authority = base.getAuthority();
        _path      = base.getPath();
        _query     = base.getQuery();
      }
    }
    _fragment  = ref.getFragment();

    normalize();
  }

  void reset()
  {
    _isValid    = false;
    _isAbsolute = false;
    _isRelative = false;

    _schema    = "";
    _authority = "";
    _path      = "";
    _query     = "";
    _fragment  = "";
  }

  bool isValid() const
  {
    return _isValid;
  }

  bool isAbsolute() const
  {
    return _isAbsolute;
  }

  bool isRelative() const
  {
    return _isRelative;
  }

  std::string getSchema() const
  {
    return _schema;
  }

  std::string getAuthority() const
  {
    return _authority;
  }

  std::string getPath() const
  {
    return _path;
  }

  std::string getQuery() const
  {
    return _query;
  }

  std::string getUriEncodedQuery() const
  {
    return escapeString(_query);
  }

  std::string getFragment() const
  {
    return _fragment;
  }

  std::string getUriEncodedFragment() const
  {
    return escapeString(_fragment);
  }

  void setUri(const Uri uri)
  {
    reset();

    _isValid    = uri.isValid();
    _isAbsolute = uri.isAbsolute();
    _isRelative = uri.isRelative();

    _schema    = uri.getSchema();
    _authority = uri.getAuthority();
    _path      = uri.getPath();
    _query     = uri.getQuery();
    _fragment  = uri.getFragment();
  }

  void setUri(const std::string uri)
  {
    reset();

    fromString(unescapeString(trimString(uri)));
    normalize();
  }

  std::string toString() const
  {
    if(!_isValid) {
      return "";
    }

    std::string ret;

    ret.append(_schema).append(":");
    if(_authority.size() != 0) {
      ret.append("//").append(_authority);
    }

    ret.append(_path);

    if(_query.size() != 0) {
      ret.append("?").append(_query);
    }

    if(_fragment.size() != 0) {
      ret.append("#").append(_fragment);
    }

    return ret;
  }

  std::string toUriEncodedString() const
  {
    if(!_isValid) {
      return "";
    }

    std::string ret;

    ret.append(_schema).append(":");
    if(_authority.size() != 0) {
      ret.append("//").append(_authority);
    }

    ret.append(_path);

    if(_query.size() != 0) {
      ret.append("?").append(escapeString(_query));
    }

    if(_fragment.size() != 0) {
      ret.append("#").append(escapeString(_fragment));
    }

    return ret;
  }

  Uri& operator=(const Uri& other)
  {
    setUri(other);
    return *this;
  }

  bool operator==(const Uri& other) const
  {
    return toString() == other.toString();
  }

  bool operator<(const Uri& other) const
  {
    return toString() < other.toString();
  }

  bool operator>(const Uri& other) const
  {
    return toString() > other.toString();
  }

private:
  void fromString(std::string str)
  {
    if(!regexAbsoluteUri(str)) {
      regexRelativeUri(str);
    }
  }

  bool regexAbsoluteUri(std::string str)
  {
    reset();

    std::smatch match;
    static std::regex const expression("^(([^:/?#]+):)(//([^/?#]*))?([^?#]*)([?]([^#]*))?(#(.*))?",
                                       std::regex_constants::ECMAScript | std::regex_constants::icase);

    if(std::regex_match(str, match, expression) && match.size() == 10) {
      _isValid    = true;
      _isAbsolute = true;
      _isRelative = false;

      _schema    = match[2];
      _authority = match[4];
      _path      = match[5];
      _query     = match[7];
      _fragment  = match[9];
    }

    return _isValid;
  }

  bool regexRelativeUri(std::string str)
  {
    reset();

    std::smatch match;
    static std::regex const expression("^(//([^/?#]*))?([^?#]*)([?]([^#]*))?(#(.*))?",
                                       std::regex_constants::ECMAScript | std::regex_constants::icase);

    if(std::regex_match(str, match, expression) && match.size() == 8) {
      _isValid    = true;
      _isAbsolute = false;
      _isRelative = true;

      _schema    = "";
      _authority = match[2];
      _path      = match[3];
      _query     = match[5];
      _fragment  = match[7];
    }

    return _isValid;
  }

  void removeDoubleSlashes(std::string& str) {
    size_t pos = 0;
    while((pos = str.find("//")) != std::string::npos) {
      str.replace(pos, 2, "/");
    }
  }

  void compressPath(std::string& str) {
    size_t pos = 0;

    while((pos = str.find("/./")) != std::string::npos) {
      str.replace(pos, 3, "/");
    }

    while((pos = str.find("/../")) != std::string::npos) {
      size_t tmp = str.rfind("/", pos - 1);
      str.erase(tmp + 1, pos - tmp + 3);
    }

  }

  void normalize()
  {
    compressPath(_path);

    // Remove double slashes from authority and path only
    removeDoubleSlashes(_authority);
    removeDoubleSlashes(_path);
  }
};

#endif /* URI__HPP_ */

