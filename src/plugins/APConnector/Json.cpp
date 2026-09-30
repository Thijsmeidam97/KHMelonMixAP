#include "Json.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <sstream>
namespace Plugins::APC {
JVal JVal::nul() {
    return {};
}
JVal JVal::boolean(bool value) {
    JVal out;
    out.kind = Kind::Bool;
    out.b = value;
    return out;
}
JVal JVal::number(double value) {
    JVal out;
    out.kind = Kind::Num;
    out.num = value;
    return out;
}
JVal JVal::string(std::string value) {
    JVal out;
    out.kind = Kind::Str;
    out.str = std::move(value);
    return out;
}
JVal JVal::array() {
    JVal out;
    out.kind = Kind::Arr;
    return out;
}
JVal JVal::object() {
    JVal out;
    out.kind = Kind::Obj;
    return out;
}
JVal& JVal::operator[](const std::string& key) {
    kind = Kind::Obj;
    return obj[key];
}
const JVal* JVal::get(const std::string& key) const {
    const JVal* out = nullptr;
    if (kind == Kind::Obj) {
        const auto it = obj.find(key);
        if (it != obj.end()) out = &it->second;
    }
    return out;
}
void JParse::ws() {
    while (pos < src.size() && std::isspace((unsigned char)src[pos])) ++pos;
}
bool JParse::eat(char want) {
    ws();
    const bool ok = pos < src.size() && src[pos] == want;
    if (ok) ++pos;
    return ok;
}
bool JParse::parse(JVal& out, std::string& err) {
    ws();
    bool ok = val(out, err);
    if (ok) {
        ws();
        if (pos != src.size()) {
            err = "trailing JSON data";
            ok = false;
        }
    }
    return ok;
}
bool JParse::lit(const char* text, JVal value, JVal& dst, std::string& err) {
    const size_t size = std::strlen(text);
    const bool ok = !src.compare(pos, size, text);
    if (!ok) {
        err = "invalid JSON literal";
    }
    if (ok) {
        pos += size;
        dst = std::move(value);
    }
    return ok;
}
bool JParse::val(JVal& out, std::string& err) {
    ws();
    bool ok = pos < src.size();
    if (!ok)
        err = "unexpected end of JSON";
    if (ok) {
        switch (src[pos]) {
        case 'n': ok = lit("null", JVal::nul(), out, err); break;
        case 't': ok = lit("true", JVal::boolean(true), out, err); break;
        case 'f': ok = lit("false", JVal::boolean(false), out, err); break;
        case '"': {
            std::string value;
            ok = strVal(value, err);
            if (ok) out = JVal::string(std::move(value));
            break;
        }
        case '[': ok = arrVal(out, err); break;
        case '{': ok = objVal(out, err); break;
        default: ok = numVal(out, err); break;
        }
    }
    return ok;
}
bool JParse::strVal(std::string& out, std::string& err) {
    bool ok = pos < src.size() && src[pos] == '"';
    if (!ok) {
        err = "expected JSON string";
    }
    bool done = false;
    if (ok) {
        ++pos;
        out.clear();
        while (pos < src.size() && !done && ok) {
            const char c = src[pos++];
            if (c == '"') done = true;
            else if ((unsigned char)c < 0x20) {
                err = "control character in JSON string";
                ok = false;
            }
            else {
                if (c != '\\') out.push_back(c);
                else if (pos >= src.size()) {
                    err = "unfinished JSON escape";
                    ok = false;
                }
                else {
                    const char e = src[pos++];
                    const char* esc = "\"\\/bfnrt";
                    const char* val = "\"\\/\b\f\n\r\t";
                    const char* found = std::strchr(esc, e);
                    if (e != 'u') {
                        if (!found) {
                            err = "invalid JSON escape";
                            ok = false;
                        }
                        else out.push_back(val[found - esc]);
                    }
                    else if (pos + 4 > src.size()) {
                        err = "short unicode escape";
                        ok = false;
                    }
                    else {
                        unsigned code = 0;
                        bool hex = true;
                        for (int i = 0; i < 4; ++i) {
                            const char h = src[pos++];
                            code <<= 4;
                            if (h >= '0' && h <= '9') code |= h - '0';
                            else if (h >= 'a' && h <= 'f') code |= h - 'a' + 10;
                            else if (h >= 'A' && h <= 'F') code |= h - 'A' + 10;
                            else hex = false;
                        }
                        if (!hex) {
                            err = "invalid unicode escape";
                            ok = false;
                        }
                        else if (code <= 0x7F) out.push_back((char)code);
                        else if (code <= 0x7FF) {
                            out.push_back((char)(0xC0 | (code >> 6)));
                            out.push_back((char)(0x80 | (code & 63)));
                        }
                        else {
                            out.push_back((char)(0xE0 | (code >> 12)));
                            out.push_back((char)(0x80 | ((code >> 6) & 63)));
                            out.push_back((char)(0x80 | (code & 63)));
                        }
                    }
                }
            }
        }
    }
    if (ok && !done)
        err = "unterminated JSON string";
    return ok && done;
}
bool JParse::arrVal(JVal& out, std::string& err) {
    bool ok = eat('[');
    if (!ok) {
        err = "expected JSON array";
    }
    bool done = false;
    if (ok) {
        ws();
        out = JVal::array();
        if (pos < src.size() && src[pos] == ']') {
            ++pos;
            done = true;
        }
        while (!done && ok) {
            JVal value;
            ok = val(value, err);
            if (ok) out.arr.push_back(std::move(value));
            ws();
            if (ok && pos < src.size() && src[pos] == ']') {
                ++pos;
                done = true;
            }
            else if (ok && !eat(',')) {
                err = "expected comma in JSON array";
                ok = false;
            }
        }
    }
    return ok && done;
}
bool JParse::objVal(JVal& out, std::string& err) {
    bool ok = eat('{');
    if (!ok) {
        err = "expected JSON object";
    }
    bool done = false;
    if (ok) {
        out = JVal::object();
        ws();
        if (pos < src.size() && src[pos] == '}') {
            ++pos;
            done = true;
        }
        while (!done && ok) {
            ws();
            std::string key;
            ok = strVal(key, err) && eat(':');
            if (!ok && err.empty()) err = "expected object key";
            JVal value;
            if (ok) ok = val(value, err);
            if (ok) out.obj[std::move(key)] = std::move(value);
            ws();
            if (ok && pos < src.size() && src[pos] == '}') {
                ++pos;
                done = true;
            }
            else if (ok && !eat(',')) {
                err = "expected comma in JSON object";
                ok = false;
            }
        }
    }
    return ok && done;
}
bool JParse::numVal(JVal& out, std::string& err) {
    const size_t start = pos;
    if (pos < src.size() && (src[pos] == '-' || src[pos] == '+')) ++pos;
    while (pos < src.size() && std::isdigit((unsigned char)src[pos])) ++pos;
    if (pos < src.size() && src[pos] == '.') {
        ++pos;
        while (pos < src.size() && std::isdigit((unsigned char)src[pos])) ++pos;
    }
    if (pos < src.size() && (src[pos] == 'e' || src[pos] == 'E')) {
        ++pos;
        if (pos < src.size() && (src[pos] == '-' || src[pos] == '+')) ++pos;
        while (pos < src.size() && std::isdigit((unsigned char)src[pos])) ++pos;
    }
    bool ok = start != pos;
    if (!ok) {
        err = "invalid JSON value";
    }
    if (ok) {
        try {
            out = JVal::number(std::stod(src.substr(start, pos - start)));
        }
        catch (...) {
            err = "invalid JSON number";
            ok = false;
        }
    }
    return ok;
}
static void put(const JVal& value, std::string& out) {
    switch (value.kind) {
    case JVal::Kind::Null:
        out += "null";
        break;
    case JVal::Kind::Bool:
        out += value.b ? "true" : "false";
        break;
    case JVal::Kind::Num: {
        std::ostringstream text;
        text << std::setprecision(17) << value.num;
        out += text.str();
        break;
    }
    case JVal::Kind::Str:
        out.push_back('"');
        for (unsigned char c : value.str) {
            switch (c) {
            case '"':
                out += "\\\"";
                break;
            case '\\':
                out += "\\\\";
                break;
            case '\b':
                out += "\\b";
                break;
            case '\f':
                out += "\\f";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            default:
                if (c < 0x20) {
                    std::ostringstream text;
                    text << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (unsigned)c;
                    out += text.str();
                }
                else out.push_back((char)c);
                break;
            }
        }
        out.push_back('"');
        break;
    case JVal::Kind::Arr: {
        out.push_back('[');
        size_t i = 0;
        for (; i < value.arr.size(); ++i) {
            if (i) out.push_back(',');
            put(value.arr[i], out);
        }
        out.push_back(']');
        break;
    }
    case JVal::Kind::Obj:
        out.push_back('{'); {
            bool first = true;
            for (const auto& [key, child] : value.obj) {
                if (!first) out.push_back(',');
                first = false;
                put(JVal::string(key), out);
                out.push_back(':');
                put(child, out);
            }
        }
        out.push_back('}');
        break;
    }
}
std::string dump(const JVal& value) {
    std::string out;
    put(value, out);
    return out;
}
bool jInt(const JVal* value, uint32_t& out) {
    const bool ok = value && value->kind == JVal::Kind::Num && std::isfinite(value->num) && value->num >= 0 && value->num <= 4294967295.0 && std::floor(value->num) == value->num;
    if (ok) out = (uint32_t)value->num;
    return ok;
}
bool jBool(const JVal* value, bool& out) {
    const bool ok = value && value->kind == JVal::Kind::Bool;
    if (ok) out = value->b;
    return ok;
}
bool jStr(const JVal* value, std::string& out) {
    const bool ok = value && value->kind == JVal::Kind::Str;
    if (ok) out = value->str;
    return ok;
}
JVal reply(const char* type) {
    JVal out = JVal::object();
    out["type"] = JVal::string(type);
    return out;
}
JVal fail(const std::string& err) {
    JVal out = reply("ERROR");
    out["err"] = JVal::string(err);
    return out;
}
std::string b64enc(const std::vector<uint8_t>& data) {
    static constexpr char tab[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve((data.size() + 2) / 3 * 4);
    size_t i = 0;
    for (; i < data.size(); i += 3) {
        const uint32_t a = data[i];
        const uint32_t b = i + 1 < data.size() ? data[i + 1] : 0;
        const uint32_t c = i + 2 < data.size() ? data[i + 2] : 0;
        const uint32_t bits = (a << 16) | (b << 8) | c;
        out.push_back(tab[(bits >> 18) & 63]);
        out.push_back(tab[(bits >> 12) & 63]);
        out.push_back(i + 1 < data.size() ? tab[(bits >> 6) & 63] : '=');
        out.push_back(i + 2 < data.size() ? tab[bits & 63] : '=');
    }
    return out;
}
bool b64dec(const std::string& text, std::vector<uint8_t>& out) {
    static unsigned char tab[256] = {};
    static bool init = false;
    if (!init) {
        std::fill(std::begin(tab), std::end(tab), (unsigned char)0xFF);
        const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        unsigned i = 0;
        for (; chars[i]; ++i) tab[(unsigned char)chars[i]] = (unsigned char)i;
        init = true;
    }
    out.clear();
    bool ok = text.size() % 4 == 0;
    for (size_t i = 0; i < text.size() && ok; i += 4) {
        const unsigned char a = tab[(unsigned char)text[i]];
        const unsigned char b = tab[(unsigned char)text[i + 1]];
        const char c = text[i + 2];
        const char d = text[i + 3];
        ok = a != 0xFF && b != 0xFF && (c == '=' || tab[(unsigned char)c] != 0xFF) && (d == '=' || tab[(unsigned char)d] != 0xFF);
        if (ok) {
            const uint32_t bits = ((uint32_t)a << 18) | ((uint32_t)b << 12) | (c == '=' ? 0 : (uint32_t)tab[(unsigned char)c] << 6) | (d == '=' ? 0 : tab[(unsigned char)d]);
            out.push_back((uint8_t)(bits >> 16));
            if (c != '=') out.push_back((uint8_t)(bits >> 8));
            if (d != '=') out.push_back((uint8_t)bits);
        }
    }
    return ok;
}
}
