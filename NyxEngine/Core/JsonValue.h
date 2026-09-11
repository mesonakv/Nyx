#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <utility>

// ============ JsonValue ============
//
// 通用 JSON 容器。
//
// 支持类型：Null, Bool, Int (int64), Float (double), String, Array, Object
//
// 用法：
//   JsonValue root;
//   root["lighting"]["timeOfDay"] = 0.5f;
//   root["lighting"]["enabled"] = true;
//   std::string text = root.ToString(true);   // pretty print
//
//   std::string err;
//   JsonValue parsed = JsonValue::Parse(text, &err);
//   if (!err.empty()) { /* 错误 */ }
//   float t = parsed["lighting"]["timeOfDay"].AsFloat(0.5f);
//
// 设计：
//   - 解析出错时返回无效值（IsValid() == false）
//   - 对象保持键插入顺序
//   - 数组下标越界返回 Null
//   - 对象键不存在返回 Null
//
// 限制：
//   - \uXXXX 只支持 BMP（U+0000 ~ U+FFFF），不支持代理对
//   - 不支持注释
//   - 不支持 NaN / Infinity
//   - 不支持循环引用（不会出现）

class JsonValue {
public:
    enum class Type {
        Null,
        Bool,
        Int,
        Float,
        String,
        Array,
        Object
    };

    // ---------- 构造 ----------
    JsonValue();
    JsonValue(std::nullptr_t);
    JsonValue(bool v);
    JsonValue(int v);
    JsonValue(int64_t v);
    JsonValue(float v);
    JsonValue(double v);
    JsonValue(const char* s);
    JsonValue(const std::string& s);

    JsonValue(const JsonValue&) = default;
    JsonValue(JsonValue&&) = default;
    JsonValue& operator=(const JsonValue&) = default;
    JsonValue& operator=(JsonValue&&) = default;

    // ---------- 类型查询 ----------
    Type GetType() const { return type_; }
    bool IsNull() const   { return type_ == Type::Null; }
    bool IsBool() const   { return type_ == Type::Bool; }
    bool IsInt() const    { return type_ == Type::Int; }
    bool IsFloat() const  { return type_ == Type::Float; }
    bool IsNumber() const { return type_ == Type::Int || type_ == Type::Float; }
    bool IsString() const { return type_ == Type::String; }
    bool IsArray() const  { return type_ == Type::Array; }
    bool IsObject() const { return type_ == Type::Object; }
    bool IsValid() const  { return valid_; }

    // ---------- 读取（带默认值）----------
    bool        AsBool(bool def = false) const;
    int64_t     AsInt64(int64_t def = 0) const;
    int         AsInt(int def = 0) const;
    double      AsDouble(double def = 0.0) const;
    float       AsFloat(float def = 0.0f) const;
    std::string AsString(const std::string& def = "") const;

    // ---------- 数组 ----------
    size_t Size() const;
    void Push(JsonValue v);
    JsonValue& operator[](size_t index);
    const JsonValue& operator[](size_t index) const;

    // ---------- 对象 ----------
    bool Has(const std::string& key) const;
    void Set(const std::string& key, JsonValue v);
    bool Remove(const std::string& key);
    JsonValue& operator[](const std::string& key);
    const JsonValue& operator[](const std::string& key) const;
    std::vector<std::string> GetKeys() const;

    // ---------- 强制转换类型（清空现有内容）----------
    void ClearAsArray();
    void ClearAsObject();

    // ---------- 序列化 ----------
    std::string ToString(bool pretty = false) const;

    // ---------- 解析 ----------
    static JsonValue Parse(const std::string& text, std::string* outError = nullptr);

private:
    Type type_ = Type::Null;
    bool valid_ = true;

    bool bool_ = false;
    int64_t int_ = 0;
    double float_ = 0.0;
    std::string string_;
    std::vector<JsonValue> array_;
    std::vector<std::pair<std::string, JsonValue>> object_;

    void Serialize(std::string& out, bool pretty, int indent) const;
    static void EscapeString(std::string& out, const std::string& s);
};