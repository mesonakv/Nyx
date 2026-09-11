#include "JsonValue.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <charconv>

// ============ 构造 ============

JsonValue::JsonValue() : type_(Type::Null) {}
JsonValue::JsonValue(std::nullptr_t) : type_(Type::Null) {}
JsonValue::JsonValue(bool v) : type_(Type::Bool), bool_(v) {}
JsonValue::JsonValue(int v) : type_(Type::Int), int_((int64_t)v) {}
JsonValue::JsonValue(int64_t v) : type_(Type::Int), int_(v) {}
JsonValue::JsonValue(float v) : type_(Type::Float), float_((double)v) {}
JsonValue::JsonValue(double v) : type_(Type::Float), float_(v) {}
JsonValue::JsonValue(const char* s) : type_(Type::String), string_(s ? s : "") {}
JsonValue::JsonValue(const std::string& s) : type_(Type::String), string_(s) {}

// ============ 读取 ============

bool JsonValue::AsBool(bool def) const {
    if (type_ == Type::Bool) return bool_;
    return def;
}

int64_t JsonValue::AsInt64(int64_t def) const {
    if (type_ == Type::Int) return int_;
    if (type_ == Type::Float) return (int64_t)float_;
    if (type_ == Type::Bool) return bool_ ? 1 : 0;
    return def;
}

int JsonValue::AsInt(int def) const {
    return (int)AsInt64((int64_t)def);
}

double JsonValue::AsDouble(double def) const {
    if (type_ == Type::Int) return (double)int_;
    if (type_ == Type::Float) return float_;
    if (type_ == Type::Bool) return bool_ ? 1.0 : 0.0;
    return def;
}

float JsonValue::AsFloat(float def) const {
    return (float)AsDouble((double)def);
}

std::string JsonValue::AsString(const std::string& def) const {
    if (type_ == Type::String) return string_;
    return def;
}

// ============ 数组 ============

size_t JsonValue::Size() const {
    if (type_ == Type::Array) return array_.size();
    if (type_ == Type::Object) return object_.size();
    return 0;
}

void JsonValue::Push(JsonValue v) {
    if (type_ != Type::Array) ClearAsArray();
    array_.push_back(std::move(v));
}

JsonValue& JsonValue::operator[](size_t index) {
    if (type_ != Type::Array) ClearAsArray();
    if (index >= array_.size()) {
        array_.resize(index + 1);
    }
    return array_[index];
}

const JsonValue& JsonValue::operator[](size_t index) const {
    static const JsonValue kNull;
    if (type_ != Type::Array || index >= array_.size()) return kNull;
    return array_[index];
}

// ============ 对象 ============

bool JsonValue::Has(const std::string& key) const {
    if (type_ != Type::Object) return false;
    for (const auto& p : object_) {
        if (p.first == key) return true;
    }
    return false;
}

void JsonValue::Set(const std::string& key, JsonValue v) {
    if (type_ != Type::Object) ClearAsObject();
    for (auto& p : object_) {
        if (p.first == key) {
            p.second = std::move(v);
            return;
        }
    }
    object_.emplace_back(key, std::move(v));
}

bool JsonValue::Remove(const std::string& key) {
    if (type_ != Type::Object) return false;
    for (auto it = object_.begin(); it != object_.end(); ++it) {
        if (it->first == key) {
            object_.erase(it);
            return true;
        }
    }
    return false;
}

JsonValue& JsonValue::operator[](const std::string& key) {
    if (type_ != Type::Object) ClearAsObject();
    for (auto& p : object_) {
        if (p.first == key) return p.second;
    }
    object_.emplace_back(key, JsonValue());
    return object_.back().second;
}

const JsonValue& JsonValue::operator[](const std::string& key) const {
    static const JsonValue kNull;
    if (type_ != Type::Object) return kNull;
    for (const auto& p : object_) {
        if (p.first == key) return p.second;
    }
    return kNull;
}

std::vector<std::string> JsonValue::GetKeys() const {
    std::vector<std::string> keys;
    if (type_ != Type::Object) return keys;
    keys.reserve(object_.size());
    for (const auto& p : object_) keys.push_back(p.first);
    return keys;
}

// ============ 类型强制转换 ============

void JsonValue::ClearAsArray() {
    type_ = Type::Array;
    valid_ = true;
    bool_ = false;
    int_ = 0;
    float_ = 0.0;
    string_.clear();
    array_.clear();
    object_.clear();
}

void JsonValue::ClearAsObject() {
    type_ = Type::Object;
    valid_ = true;
    bool_ = false;
    int_ = 0;
    float_ = 0.0;
    string_.clear();
    array_.clear();
    object_.clear();
}

// ============ 序列化 ============

void JsonValue::EscapeString(std::string& out, const std::string& s) {
    out += '"';
    for (char c : s) {
        switch (c) {
        case '"':  out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n";  break;
        case '\r': out += "\\r";  break;
        case '\t': out += "\\t";  break;
        case '\b': out += "\\b";  break;
        case '\f': out += "\\f";  break;
        default:
            if ((unsigned char)c < 0x20) {
                char buf[8];
                snprintf(buf, sizeof(buf), "\\u%04x", (unsigned char)c);
                out += buf;
            } else {
                out += c;
            }
            break;
        }
    }
    out += '"';
}

static void FormatDoubleTo(std::string& out, double v) {
    // NaN / Inf 不支持，输出 null 保证 JSON 合法
    if (std::isnan(v) || std::isinf(v)) {
        out += "null";
        return;
    }

    char buf[32];
    auto result = std::to_chars(buf, buf + sizeof(buf), v);
    if (result.ec == std::errc()) {
        out.append(buf, result.ptr);
        return;
    }

    snprintf(buf, sizeof(buf), "%.17g", v);
    out += buf;
}

void JsonValue::Serialize(std::string& out, bool pretty, int indent) const {
    auto newlineIndent = [&](int level) {
        if (!pretty) return;
        out += '\n';
        for (int i = 0; i < level * 2; i++) out += ' ';
    };

    switch (type_) {
    case Type::Null:
        out += "null";
        break;

    case Type::Bool:
        out += bool_ ? "true" : "false";
        break;

    case Type::Int:
        out += std::to_string(int_);
        break;

    case Type::Float:
        FormatDoubleTo(out, float_);
        break;

    case Type::String:
        EscapeString(out, string_);
        break;

    case Type::Array:
        if (array_.empty()) {
            out += "[]";
        } else {
            out += '[';
            for (size_t i = 0; i < array_.size(); i++) {
                if (i > 0) out += ',';
                newlineIndent(indent + 1);
                array_[i].Serialize(out, pretty, indent + 1);
            }
            newlineIndent(indent);
            out += ']';
        }
        break;

    case Type::Object:
        if (object_.empty()) {
            out += "{}";
        } else {
            out += '{';
            for (size_t i = 0; i < object_.size(); i++) {
                if (i > 0) out += ',';
                newlineIndent(indent + 1);
                EscapeString(out, object_[i].first);
                out += ':';
                if (pretty) out += ' ';
                object_[i].second.Serialize(out, pretty, indent + 1);
            }
            newlineIndent(indent);
            out += '}';
        }
        break;
    }
}

std::string JsonValue::ToString(bool pretty) const {
    std::string out;
    out.reserve(256);
    Serialize(out, pretty, 0);
    return out;
}

// ============ 解析 ============

namespace {

class Parser {
public:
    explicit Parser(const std::string& text) : text_(text), pos_(0), depth_(0) {}

    JsonValue Parse(std::string* outError) {
        SkipWhitespace();
        JsonValue result = ParseValue();
        if (!error_.empty()) {
            if (outError) *outError = error_;
            return JsonValue();
        }
        SkipWhitespace();
        if (pos_ < text_.size()) {
            SetError("unexpected characters after top-level value");
            if (outError) *outError = error_;
            return JsonValue();
        }
        return result;
    }

private:
    static constexpr int kMaxDepth = 64;

    const std::string& text_;
    size_t pos_;
    int depth_;
    std::string error_;

    void SetError(const std::string& msg) {
        if (error_.empty()) {
            char buf[256];
            snprintf(buf, sizeof(buf), "%s (at position %zu)", msg.c_str(), pos_);
            error_ = buf;
        }
    }

    char Peek() const { return pos_ < text_.size() ? text_[pos_] : '\0'; }
    char Advance() { return pos_ < text_.size() ? text_[pos_++] : '\0'; }

    void SkipWhitespace() {
        while (pos_ < text_.size()) {
            char c = text_[pos_];
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                pos_++;
            } else {
                break;
            }
        }
    }

    JsonValue ParseValue() {
        if (depth_ >= kMaxDepth) {
            SetError("maximum nesting depth exceeded");
            return JsonValue();
        }

        SkipWhitespace();
        if (pos_ >= text_.size()) {
            SetError("unexpected end of input");
            return JsonValue();
        }

        char c = Peek();
        switch (c) {
        case '{': return ParseObject();
        case '[': return ParseArray();
        case '"': {
            std::string s = ParseStringValue();
            if (!error_.empty()) return JsonValue();
            return JsonValue(s);
        }
        case 't': return ParseTrue();
        case 'f': return ParseFalse();
        case 'n': return ParseNull();
        default:
            if (c == '-' || (c >= '0' && c <= '9')) return ParseNumber();
            SetError("unexpected character");
            return JsonValue();
        }
    }

    JsonValue ParseObject() {
        depth_++;
        Advance();  // {

        JsonValue obj;
        obj.ClearAsObject();

        SkipWhitespace();
        if (Peek() == '}') {
            Advance();
            depth_--;
            return obj;
        }

        while (true) {
            SkipWhitespace();
            if (Peek() != '"') {
                SetError("expected string key in object");
                depth_--;
                return JsonValue();
            }
            std::string key = ParseStringValue();
            if (!error_.empty()) { depth_--; return JsonValue(); }

            SkipWhitespace();
            if (Advance() != ':') {
                SetError("expected ':' after object key");
                depth_--;
                return JsonValue();
            }

            JsonValue value = ParseValue();
            if (!error_.empty()) { depth_--; return JsonValue(); }

            obj.Set(key, std::move(value));

            SkipWhitespace();
            char c = Advance();
            if (c == ',') continue;
            if (c == '}') break;
            SetError("expected ',' or '}' in object");
            depth_--;
            return JsonValue();
        }

        depth_--;
        return obj;
    }

    JsonValue ParseArray() {
        depth_++;
        Advance();  // [

        JsonValue arr;
        arr.ClearAsArray();

        SkipWhitespace();
        if (Peek() == ']') {
            Advance();
            depth_--;
            return arr;
        }

        while (true) {
            JsonValue value = ParseValue();
            if (!error_.empty()) { depth_--; return JsonValue(); }
            arr.Push(std::move(value));

            SkipWhitespace();
            char c = Advance();
            if (c == ',') continue;
            if (c == ']') break;
            SetError("expected ',' or ']' in array");
            depth_--;
            return JsonValue();
        }

        depth_--;
        return arr;
    }

    std::string ParseStringValue() {
        Advance();  // opening "

        std::string result;
        result.reserve(32);

        while (true) {
            if (pos_ >= text_.size()) {
                SetError("unterminated string");
                return {};
            }

            char c = text_[pos_++];

            if (c == '"') break;

            if (c == '\\') {
                if (pos_ >= text_.size()) {
                    SetError("unterminated escape sequence");
                    return {};
                }
                char esc = text_[pos_++];
                switch (esc) {
                case '"':  result += '"';  break;
                case '\\': result += '\\'; break;
                case '/':  result += '/';  break;
                case 'n':  result += '\n'; break;
                case 'r':  result += '\r'; break;
                case 't':  result += '\t'; break;
                case 'b':  result += '\b'; break;
                case 'f':  result += '\f'; break;
                case 'u': {
                    if (pos_ + 4 > text_.size()) {
                        SetError("incomplete \\u escape");
                        return {};
                    }
                    char hex[5] = { text_[pos_], text_[pos_+1], text_[pos_+2], text_[pos_+3], 0 };
                    pos_ += 4;
                    unsigned int cp = 0;
                    if (sscanf(hex, "%x", &cp) != 1) {
                        SetError("invalid \\u escape");
                        return {};
                    }
                    // 只支持 BMP（不支持代理对）
                    if (cp < 0x80) {
                        result += (char)cp;
                    } else if (cp < 0x800) {
                        result += (char)(0xC0 | (cp >> 6));
                        result += (char)(0x80 | (cp & 0x3F));
                    } else {
                        result += (char)(0xE0 | (cp >> 12));
                        result += (char)(0x80 | ((cp >> 6) & 0x3F));
                        result += (char)(0x80 | (cp & 0x3F));
                    }
                    break;
                }
                default:
                    SetError("invalid escape sequence");
                    return {};
                }
            } else {
                result += c;
            }
        }

        return result;
    }

    JsonValue ParseTrue() {
        if (text_.compare(pos_, 4, "true") == 0) {
            pos_ += 4;
            return JsonValue(true);
        }
        SetError("invalid literal (expected 'true')");
        return JsonValue();
    }

    JsonValue ParseFalse() {
        if (text_.compare(pos_, 5, "false") == 0) {
            pos_ += 5;
            return JsonValue(false);
        }
        SetError("invalid literal (expected 'false')");
        return JsonValue();
    }

    JsonValue ParseNull() {
        if (text_.compare(pos_, 4, "null") == 0) {
            pos_ += 4;
            return JsonValue();
        }
        SetError("invalid literal (expected 'null')");
        return JsonValue();
    }

    JsonValue ParseNumber() {
        size_t start = pos_;
        bool isFloat = false;

        if (Peek() == '-') Advance();

        if (Peek() == '0') {
            Advance();
        } else if (Peek() >= '1' && Peek() <= '9') {
            while (Peek() >= '0' && Peek() <= '9') Advance();
        } else {
            SetError("invalid number");
            return JsonValue();
        }

        if (Peek() == '.') {
            isFloat = true;
            Advance();
            if (!(Peek() >= '0' && Peek() <= '9')) {
                SetError("invalid number (expected digits after '.')");
                return JsonValue();
            }
            while (Peek() >= '0' && Peek() <= '9') Advance();
        }

        if (Peek() == 'e' || Peek() == 'E') {
            isFloat = true;
            Advance();
            if (Peek() == '+' || Peek() == '-') Advance();
            if (!(Peek() >= '0' && Peek() <= '9')) {
                SetError("invalid number (expected digits in exponent)");
                return JsonValue();
            }
            while (Peek() >= '0' && Peek() <= '9') Advance();
        }

        std::string numStr = text_.substr(start, pos_ - start);

        if (isFloat) {
            double v = 0.0;
            auto result = std::from_chars(numStr.data(), numStr.data() + numStr.size(), v);
            if (result.ec != std::errc()) {
                SetError("failed to parse float");
                return JsonValue();
            }
            return JsonValue(v);
        } else {
            int64_t v = 0;
            auto result = std::from_chars(numStr.data(), numStr.data() + numStr.size(), v);
            if (result.ec != std::errc()) {
                // 超出 int64 范围：当作 double
                double dv = 0.0;
                auto r2 = std::from_chars(numStr.data(), numStr.data() + numStr.size(), dv);
                if (r2.ec != std::errc()) {
                    SetError("failed to parse number");
                    return JsonValue();
                }
                return JsonValue(dv);
            }
            return JsonValue(v);
        }
    }
};

} // anonymous namespace

JsonValue JsonValue::Parse(const std::string& text, std::string* outError) {
    Parser parser(text);
    return parser.Parse(outError);
}