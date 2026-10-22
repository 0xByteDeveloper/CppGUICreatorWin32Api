// json.h — minimal JSON ayrıştırıcı/yazıcı (UTF-8), Config_info.json ve proje
// kaydetme/yükleme için yeterli. Bağımlılıksız.
#pragma once
#include <string>
#include <vector>

namespace json {

class Value {
public:
    enum Type { NUL, BOOL, NUM, STR, ARR, OBJ };
    Type t = NUL;
    bool b = false;
    double num = 0;
    std::wstring str;
    std::vector<Value> arr;
    std::vector<std::pair<std::wstring, Value>> obj;

    Value() {}
    static Value MakeStr(const std::wstring& s) { Value v; v.t = STR; v.str = s; return v; }
    static Value MakeNum(double d) { Value v; v.t = NUM; v.num = d; return v; }
    static Value MakeBool(bool b) { Value v; v.t = BOOL; v.b = b; return v; }
    static Value MakeArr() { Value v; v.t = ARR; return v; }
    static Value MakeObj() { Value v; v.t = OBJ; return v; }

    bool IsObj() const { return t == OBJ; }
    bool IsArr() const { return t == ARR; }
    bool IsStr() const { return t == STR; }
    bool IsNum() const { return t == NUM; }

    // Nesne alanı erişimi (yoksa NUL döner)
    const Value& Get(const wchar_t* key) const {
        if (t == OBJ)
            for (auto& kv : obj) if (kv.first == key) return kv.second;
        static Value nul;
        return nul;
    }
    bool Has(const wchar_t* key) const {
        if (t == OBJ) for (auto& kv : obj) if (kv.first == key) return true;
        return false;
    }
    void Set(const std::wstring& key, Value v) {
        if (t != OBJ) { t = OBJ; obj.clear(); }
        for (auto& kv : obj) if (kv.first == key) { kv.second = v; return; }
        obj.push_back({ key, v });
    }
    void Set(const std::wstring& key, const std::wstring& s) { Set(key, MakeStr(s)); }
    void Set(const std::wstring& key, const wchar_t* s) { Set(key, MakeStr(s)); }
    void Set(const std::wstring& key, double d) { Set(key, MakeNum(d)); }
    void Push(Value v) { if (t != ARR) { t = ARR; arr.clear(); } arr.push_back(v); }

    // string/bool/num dönüşümleri
    std::wstring AsStr(const std::wstring& def = L"") const {
        if (t == STR) return str;
        if (t == NUM) { wchar_t buf[64]; swprintf(buf, 64, L"%g", num); return buf; }
        if (t == BOOL) return b ? L"True" : L"False";
        return def;
    }
    double AsNum(double def = 0) const {
        if (t == NUM) return num;
        if (t == STR) { wchar_t* e = nullptr; double d = wcstod(str.c_str(), &e); if (e != str.c_str()) return d; }
        if (t == BOOL) return b ? 1 : 0;
        return def;
    }
    bool AsBool(bool def = false) const {
        if (t == BOOL) return b;
        if (t == STR) { return _wcsicmp(str.c_str(), L"true") == 0 || str == L"1"; }
        if (t == NUM) return num != 0;
        return def;
    }
};

// utf-8 girdi; hata durumunda *ok=false
Value Parse(const std::string& utf8, bool* ok = nullptr);
// pretty=true girintili çıktı
std::string Stringify(const Value& v, bool pretty = true);

} // namespace json
