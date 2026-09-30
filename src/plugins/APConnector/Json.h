#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>
namespace Plugins::APC {
struct JVal {
    enum class Kind { Null, Bool, Num, Str, Arr, Obj };
    Kind kind = Kind::Null;
    bool b = false;
    double num = 0;
    std::string str;
    std::vector<JVal> arr;
    std::map<std::string, JVal> obj;
    static JVal nul();
    static JVal boolean(bool v);
    static JVal number(double v);
    static JVal string(std::string v);
    static JVal array();
    static JVal object();
    JVal& operator[](const std::string& key);
    const JVal* get(const std::string& key) const;
};
class JParse {
public:
    explicit JParse(const std::string& text) : src(text) {}
    bool parse(JVal& out, std::string& err);
private:
    const std::string& src;
    size_t pos = 0;
    void ws();
    bool eat(char c);
    bool val(JVal& out, std::string& err);
    bool lit(const char* text, JVal out, JVal& dst, std::string& err);
    bool strVal(std::string& out, std::string& err);
    bool arrVal(JVal& out, std::string& err);
    bool objVal(JVal& out, std::string& err);
    bool numVal(JVal& out, std::string& err);
};
std::string dump(const JVal& val);
bool jInt(const JVal* val, uint32_t& out);
bool jBool(const JVal* val, bool& out);
bool jStr(const JVal* val, std::string& out);
JVal reply(const char* type);
JVal fail(const std::string& err);
std::string b64enc(const std::vector<uint8_t>& data);
bool b64dec(const std::string& text, std::vector<uint8_t>& out);
}
