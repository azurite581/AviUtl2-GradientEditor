#ifndef GRD_PARSER_H
#define GRD_PARSER_H

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <iostream>
#include <fstream>
#include <format>
#include <filesystem>
#include <vector>
#include <string_view>
#include <string>
#include <expected>
#include <bit>
#include <concepts>
#include <variant>
#include <array>
#include <cstddef>

struct Class {
	std::string name;
	std::string id;
};

struct Object {
	Class class_;
	uint32_t key_item_num;
};

struct Enumerated {
	std::string type_id;
	std::string value_id;
};

struct BookColor {
	std::string book_name{};
	std::string color_name{};
	int32_t book_id{};
	std::vector<std::byte> book_key{};
};

struct HSBC {
	double hue;
	double saturate;
	double brightness;
};

struct RGBC {
	double red;
	double green;
	double blue;
};

struct CMYC {
	double cyan;
	double magenta;
	double yellow;
	double black;
};

struct Grsc {
	double gray;
};

struct LbCl {
	double luminance;
	double a;
	double b;
};

using ColorObjectVariant = std::variant<BookColor, CMYC, Grsc, HSBC, LbCl, RGBC>;

struct PrintColorObject {
	void operator()(const BookColor& color) const
	{
		std::cout << std::format("book_name: {}\n", color.book_name);
		std::cout << std::format("color_name: {}\n", color.color_name);
		std::cout << std::format("book_id: {}\n", color.book_id);
		std::string book_key_str(reinterpret_cast<const char*>(color.book_key.data()), color.book_key.size());
		std::cout << std::format("book_key: {}\n", book_key_str);
	}
	void operator()(const CMYC& color) const
	{
		std::cout << std::format("cyan: {}\n", color.cyan);
		std::cout << std::format("magenta: {}\n", color.magenta);
		std::cout << std::format("yellow: {}\n", color.yellow);
		std::cout << std::format("black: {}\n", color.black);
	}
	void operator()(const Grsc& color) const { std::cout << std::format("gray: {}\n", color.gray); }
	void operator()(const HSBC& color) const
	{
		std::cout << std::format("hue: {}\n", color.hue);
		std::cout << std::format("saturate: {}\n", color.saturate);
		std::cout << std::format("brightness: {}\n", color.brightness);
	}
	void operator()(const LbCl& color) const
	{
		std::cout << std::format("luminance: {}\n", color.luminance);
		std::cout << std::format("a: {}\n", color.a);
		std::cout << std::format("b: {}\n", color.b);
	}
	void operator()(const RGBC& color) const
	{
		std::cout << std::format("red: {}\n", color.red);
		std::cout << std::format("green: {}\n", color.green);
		std::cout << std::format("blue: {}\n", color.blue);
	}
};

struct ColorObject {
	std::string color_type;
	ColorObjectVariant color_object_variant;

	void print() const
	{
		std::cout << std::format("color_type: {}\n", color_type);
		std::visit(PrintColorObject{}, color_object_variant);
	}
};

struct ColorStopObject {
	double location;
	double midpoint;
	std::string type;
	ColorObject color_object;

	void print() const
	{
		std::cout << std::format("location: {}\n", location);
		std::cout << std::format("midpoint: {}\n", midpoint);
		std::cout << std::format("type: {}\n", type);
		color_object.print();
	}
};

struct TransparencyStopObject {
	int32_t location{};
	int32_t midpoint{};
	double opacity{};

	void print() const
	{
		std::cout << std::format("location: {}\n", location);
		std::cout << std::format("midpoint: {}\n", midpoint);
		std::cout << std::format("opacity: {}\n", opacity);
	}
};

struct ColorStops {
	int32_t item_num{};
	std::vector<ColorStopObject> color_stop_objects{};

	void print() const
	{
		std::cout << std::format("item_num: {}\n", item_num);
		for (const auto& e : color_stop_objects) e.print();
	}
};

struct TransparencyStops {
	std::vector<TransparencyStopObject> transparency_stop_objects;

	void print() const
	{
		for (const auto& e : transparency_stop_objects) e.print();
	}
};

struct CustomStopsGradientObject {
	double interpolation{};
	ColorStops color_stops{};
	TransparencyStops transparency_stops{};

	void print() const
	{
		std::cout << std::format("interpolation: {}\n", interpolation);
		color_stops.print();
		transparency_stops.print();
	}
};

struct ColorNoiseGradientObject {
	bool show_transparency{};
	bool vector_color{};
	std::string color_space;
	int32_t random_seed;
	int32_t smoothness;
	std::array<double, 4> minimum_values{};
	std::array<double, 4> maximum_values{};

	void print() const
	{
		std::cout << std::format("show_transparency: {}\n", show_transparency);
		std::cout << std::format("vector_color: {}\n", vector_color);
		std::cout << std::format("color_space: {}\n", color_space);
		std::cout << std::format("random_seed: {}\n", random_seed);
		std::cout << std::format("smoothness: {}\n", smoothness);
		std::cout << std::format("minimum_values = [{}, {}, {}, {}]\n",
			minimum_values[0], minimum_values[1], minimum_values[2], minimum_values[3]);
		std::cout << std::format("maximum_values = [{}, {}, {}, {}]\n",
			maximum_values[0], maximum_values[1], maximum_values[2], maximum_values[3]);
	}
};

using GradientObject = std::variant<CustomStopsGradientObject, ColorNoiseGradientObject>;

struct PrintGradientObject {
	void operator()(const CustomStopsGradientObject& g) const { g.print(); }
	void operator()(const ColorNoiseGradientObject& g) const { g.print(); }
};

struct Gradient {
	std::string gradient_name;
	std::string gradient_form;
	GradientObject gradient_object;

	void print() const
	{
		std::cout << std::format("gradient_name: {}\n", gradient_name);
		std::cout << std::format("gradient_form: {}\n", gradient_form);
		std::visit(PrintGradientObject{}, gradient_object);
	}
};

struct GradientList {
	std::vector<Gradient> gradient_list;

	void print() const { for (const auto& e : gradient_list) e.print(); }
};

struct Header {
	std::string signature{};
	uint16_t version{};
	uint32_t descriptor_version{};
};

struct DescripterObject {
	int32_t key_item_num;
};

struct GRD {
	Header header;
	DescripterObject descripter_object;
	GradientList gradient_list;
};

// ============================================================
// BinaryReader — ストリームからのバイト列読み取り
// ============================================================

class BinaryReader {
public:
	explicit BinaryReader(std::ifstream& file) : file_(file) {}

	template <std::integral T>
	std::expected<T, std::string> readIntBE()
	{
		T n = 0;
		if (!file_.read(reinterpret_cast<char*>(&n), sizeof(n))) {
			return std::unexpected{ std::format("Failed to read {}-byte integer from stream", sizeof(T)) };
		}
		return std::byteswap(n);
	}

	std::expected<double, std::string> readDoubleBE()
	{
		auto value = readIntBE<int64_t>();
		if (!value) return std::unexpected{ value.error() };
		return std::bit_cast<double>(value.value());
	}

	std::expected<bool, std::string> readBoolean()
	{
		uint8_t b;
		if (!file_.read(reinterpret_cast<char*>(&b), 1)) {
			return std::unexpected{ "Failed to read 1-byte boolean from stream" };
		}
		return b != 0;
	}

	std::expected<std::string, std::string> readItemType()
	{
		char buf[5] = { 0 };
		if (!file_.read(buf, 4)) {
			return std::unexpected{ "Failed to read 4-byte type tag from stream" };
		}
		return buf;
	}

	std::expected<std::string, std::string> readID()
	{
		auto len = readIntBE<uint32_t>();
		if (!len) return std::unexpected{ len.error() };

		uint32_t actual_len = (len.value() == 0) ? 4 : len.value();
		std::string id;
		id.resize(actual_len);
		if (!file_.read(id.data(), actual_len)) {
			return std::unexpected{ std::format("Failed to read {}-byte ID from stream", actual_len) };
		}
		return id;
	}

	std::expected<std::string, std::string> readUTF16BE(uint32_t str_len)
	{
		if (str_len == 0) {
			return std::unexpected{ "文字数の長さが不正です" };
		}

		uint32_t byte_len = str_len * 2;
		std::vector<uint8_t> buffer(byte_len);
		if (!file_.read(reinterpret_cast<char*>(buffer.data()), byte_len)) {
			return std::unexpected{ "ファイルからの読み込みに失敗しました" };
		}

		std::u16string u16str;
		u16str.reserve(str_len);
		for (size_t i = 0; i < byte_len - 1; i += 2) {
			char16_t c = (static_cast<char16_t>(buffer[i]) << 8) | static_cast<char16_t>(buffer[i + 1]);
			if (c == 0) break;
			u16str.push_back(c);
		}

		int32_t size = ::WideCharToMultiByte(CP_UTF8, 0,
			reinterpret_cast<LPCWCH>(u16str.data()), static_cast<int>(u16str.size()),
			nullptr, 0, nullptr, nullptr);
		std::string u8str(size, 0);
		::WideCharToMultiByte(CP_UTF8, 0,
			reinterpret_cast<LPCWCH>(u16str.data()), static_cast<int>(u16str.size()),
			u8str.data(), size, nullptr, nullptr);

		return u8str;
	}

	std::expected<std::vector<std::byte>, std::string> readRawBytes(uint32_t len)
	{
		std::vector<std::byte> data(len);
		if (!file_.read(reinterpret_cast<char*>(data.data()), len)) {
			return std::unexpected{ std::format("Failed to read {}-byte data from stream", len) };
		}
		return data;
	}

	void skip(std::streamsize n) { file_.ignore(n); }
	bool good() const { return file_.good(); }

private:
	std::ifstream& file_;
};

// ============================================================
// BinaryWriter — ストリームへのバイト列書き込み
// ============================================================

class BinaryWriter {
public:
	explicit BinaryWriter(std::ofstream& file) : file_(file) {}

	template <std::integral T>
	std::expected<void, std::string> writeIntBE(T value)
	{
		T swapped = std::byteswap(value);
		if (!file_.write(reinterpret_cast<const char*>(&swapped), sizeof(T))) {
			return std::unexpected{ std::format("Failed to write {}-byte integer to stream", sizeof(T)) };
		}
		return {};
	}

	std::expected<void, std::string> writeDoubleBE(double value)
	{
		return writeIntBE(std::bit_cast<int64_t>(value));
	}

	std::expected<void, std::string> writeBoolean(bool value)
	{
		uint8_t b = value ? 1 : 0;
		if (!file_.write(reinterpret_cast<const char*>(&b), 1)) {
			return std::unexpected{ "Failed to write 1-byte boolean to stream" };
		}
		return {};
	}

	// 固定長 4 バイトのタグを書き込む
	std::expected<void, std::string> writeItemType(std::string_view tag)
	{
		if (tag.size() != 4) {
			return std::unexpected{ std::format("Item type must be 4 bytes, got {}", tag.size()) };
		}
		if (!file_.write(tag.data(), 4)) {
			return std::unexpected{ "Failed to write 4-byte type tag to stream" };
		}
		return {};
	}

	// ID を書き込む。長さが 4 のときは長さフィールドを 0 にする（仕様上の省略表現）
	std::expected<void, std::string> writeID(std::string_view id)
	{
		uint32_t len = static_cast<uint32_t>(id.size());
		uint32_t written_len = (len == 4) ? 0 : len;
		if (auto err = writeIntBE(written_len); !err) return err;
		if (!file_.write(id.data(), len)) {
			return std::unexpected{ std::format("Failed to write {}-byte ID to stream", len) };
		}
		return {};
	}

	// UTF-8 文字列を UTF-16BE に変換して書き込む（長さフィールド付き）
	// パース側の readUTF16BE と対称: 長さフィールドは NULL 終端を含む文字数
	std::expected<void, std::string> writeUTF16BE(std::string_view utf8str)
	{
		int wlen = ::MultiByteToWideChar(CP_UTF8, 0,
			utf8str.data(), static_cast<int>(utf8str.size()), nullptr, 0);
		std::wstring wstr(wlen, 0);
		::MultiByteToWideChar(CP_UTF8, 0,
			utf8str.data(), static_cast<int>(utf8str.size()), wstr.data(), wlen);

		// NULL 終端を含めた文字数を長さフィールドに書く
		uint32_t char_count = static_cast<uint32_t>(wstr.size()) + 1;
		if (auto err = writeIntBE(char_count); !err) return err;

		for (wchar_t wc : wstr) {
			uint16_t be = std::byteswap(static_cast<uint16_t>(wc));
			if (!file_.write(reinterpret_cast<const char*>(&be), 2)) {
				return std::unexpected{ "Failed to write UTF-16BE character to stream" };
			}
		}
		// NULL 終端
		uint16_t null_char = 0;
		if (!file_.write(reinterpret_cast<const char*>(&null_char), 2)) {
			return std::unexpected{ "Failed to write UTF-16BE null terminator to stream" };
		}
		return {};
	}

	std::expected<void, std::string> writeRawBytes(const std::vector<std::byte>& data)
	{
		if (!file_.write(reinterpret_cast<const char*>(data.data()), data.size())) {
			return std::unexpected{ std::format("Failed to write {}-byte data to stream", data.size()) };
		}
		return {};
	}

	bool good() const { return file_.good(); }

private:
	std::ofstream& file_;
};

// ============================================================
// GrdParser — .GRD ドメイン固有のパース処理
// ============================================================

class GrdParser {
public:
	explicit GrdParser(std::ifstream& file) : reader_(file) {}

	std::expected<void, std::string> expectItemType(std::string_view expected)
	{
		auto tag = reader_.readItemType();
		if (!tag) return std::unexpected{ tag.error() };
		if (tag.value() != expected) {
			return std::unexpected{ std::format("Expected type tag \"{}\", got \"{}\"", expected, tag.value()) };
		}
		return {};
	}

	std::expected<void, std::string> expectID(std::string_view expected)
	{
		auto id = reader_.readID();
		if (!id) return std::unexpected{ id.error() };
		if (id.value() != expected) {
			return std::unexpected{ std::format("Expected ID \"{}\", got \"{}\"", expected, id.value()) };
		}
		return {};
	}

	std::expected<int32_t, std::string> parseInt32()
	{
		if (auto err = expectItemType("long"); !err) return std::unexpected{ err.error() };
		return reader_.readIntBE<int32_t>();
	}

	std::expected<int32_t, std::string> parseInt32WithID(std::string_view expected_id)
	{
		if (auto err = expectID(expected_id); !err) return std::unexpected{ err.error() };
		return parseInt32();
	}

	std::expected<bool, std::string> parseBool()
	{
		if (auto err = expectItemType("bool"); !err) return std::unexpected{ err.error() };
		return reader_.readBoolean();
	}

	std::expected<double, std::string> parseDouble()
	{
		if (auto err = expectItemType("doub"); !err) return std::unexpected{ err.error() };
		return reader_.readDoubleBE();
	}

	std::expected<double, std::string> parseDoubleWithID(std::string_view expected_id)
	{
		if (auto err = expectID(expected_id); !err) return std::unexpected{ err.error() };
		return parseDouble();
	}

	std::expected<double, std::string> parseUnitDouble(std::string_view expected_unit_id)
	{
		if (auto err = expectItemType("UntF"); !err) return std::unexpected{ err.error() };
		if (auto err = expectItemType(expected_unit_id); !err) return std::unexpected{ err.error() };
		return reader_.readDoubleBE();
	}

	std::expected<double, std::string> parseUnitDoubleWithID(std::string_view expected_unit_id, std::string_view expected_id)
	{
		if (auto err = expectID(expected_id); !err) return std::unexpected{ err.error() };
		return parseUnitDouble(expected_unit_id);
	}

	std::expected<std::vector<std::byte>, std::string> parseRawData()
	{
		if (auto err = expectItemType("tdta"); !err) return std::unexpected{ err.error() };
		auto len = reader_.readIntBE<uint32_t>();
		if (!len) return std::unexpected{ len.error() };
		return reader_.readRawBytes(len.value());
	}

	std::expected<std::string, std::string> parseUnicodeString()
	{
		if (auto err = expectItemType("TEXT"); !err) return std::unexpected{ err.error() };
		auto text_len = reader_.readIntBE<uint32_t>();
		if (!text_len) return std::unexpected{ text_len.error() };
		return reader_.readUTF16BE(text_len.value());
	}

	std::expected<int32_t, std::string> parseListItemNum()
	{
		if (auto err = expectItemType("VlLs"); !err) return std::unexpected{ err.error() };
		return reader_.readIntBE<int32_t>();
	}

	std::expected<Enumerated, std::string> parseEnumerated()
	{
		if (auto err = expectItemType("enum"); !err) return std::unexpected{ err.error() };
		auto type_id = reader_.readID();
		if (!type_id) return std::unexpected{ type_id.error() };
		auto value_id = reader_.readID();
		if (!value_id) return std::unexpected{ value_id.error() };
		return Enumerated{ type_id.value(), value_id.value() };
	}

	std::expected<std::string, std::string> parseEnumeratedWithTypeID(std::string_view expected_type_id)
	{
		if (auto err = expectItemType("enum"); !err) return std::unexpected{ err.error() };
		auto type_id = reader_.readID();
		if (!type_id) return std::unexpected{ type_id.error() };
		if (type_id.value() != expected_type_id) {
			return std::unexpected{ std::format("Expected enum type \"{}\", got \"{}\"", expected_type_id, type_id.value()) };
		}
		auto value_id = reader_.readID();
		if (!value_id) return std::unexpected{ value_id.error() };
		return value_id.value();
	}

	std::expected<Object, std::string> parseObject()
	{
		if (auto err = expectItemType("Objc"); !err) return std::unexpected{ err.error() };
		auto class_name_len = reader_.readIntBE<uint32_t>();
		if (!class_name_len) return std::unexpected{ class_name_len.error() };
		auto class_name = reader_.readUTF16BE(class_name_len.value());
		if (!class_name) return std::unexpected{ class_name.error() };
		auto class_id = reader_.readID();
		if (!class_id) return std::unexpected{ class_id.error() };
		auto key_item_num = reader_.readIntBE<uint32_t>();
		if (!key_item_num) return std::unexpected{ key_item_num.error() };
		return Object{ Class{ class_name.value(), class_id.value() }, key_item_num.value() };
	}

	std::expected<std::string, std::string> parseName()
	{
		if (auto err = expectID("Nm  "); !err) return std::unexpected{ err.error() };
		return parseUnicodeString();
	}

	std::expected<Enumerated, std::string> parseGradientForm()
	{
		if (auto err = expectID("GrdF"); !err) return std::unexpected{ err.error() };
		return parseEnumerated();
	}

	std::expected<double, std::string> parseInterpolation()
	{
		if (auto err = expectID("Intr"); !err) return std::unexpected{ err.error() };
		return parseDouble();
	}

	std::expected<Header, std::string> parseHeader()
	{
		auto tag = reader_.readItemType();
		if (!tag) return std::unexpected{ tag.error() };
		auto version = reader_.readIntBE<uint16_t>();
		if (!version) return std::unexpected{ version.error() };
		auto descriptor_version = reader_.readIntBE<uint32_t>();
		if (!descriptor_version) return std::unexpected{ descriptor_version.error() };
		return Header{ tag.value(), version.value(), descriptor_version.value() };
	}

	std::expected<DescripterObject, std::string> parseDescripterObject()
	{
		auto class_name_len = reader_.readIntBE<uint32_t>();
		if (!class_name_len) return std::unexpected{ class_name_len.error() };
		auto class_name = reader_.readUTF16BE(class_name_len.value());
		if (!class_name) return std::unexpected{ class_name.error() };
		if (auto err = expectID("null"); !err) return std::unexpected{ err.error() };
		auto key_item_num = reader_.readIntBE<int32_t>();
		if (!key_item_num) return std::unexpected{ key_item_num.error() };
		return DescripterObject{ key_item_num.value() };
	}

	std::expected<RGBC, std::string> parseRGBC()
	{
		auto red = parseDoubleWithID("Rd  ");  if (!red)   return std::unexpected{ red.error() };
		auto green = parseDoubleWithID("Grn ");  if (!green) return std::unexpected{ green.error() };
		auto blue = parseDoubleWithID("Bl  ");  if (!blue)  return std::unexpected{ blue.error() };
		return RGBC{ red.value(), green.value(), blue.value() };
	}

	std::expected<CMYC, std::string> parseCMYC()
	{
		auto cyan = parseDoubleWithID("Cyn ");  if (!cyan)    return std::unexpected{ cyan.error() };
		auto magenta = parseDoubleWithID("Mgnt");  if (!magenta) return std::unexpected{ magenta.error() };
		auto yellow = parseDoubleWithID("Ylw ");  if (!yellow)  return std::unexpected{ yellow.error() };
		auto black = parseDoubleWithID("Blck");  if (!black)   return std::unexpected{ black.error() };
		return CMYC{ cyan.value(), magenta.value(), yellow.value(), black.value() };
	}

	std::expected<Grsc, std::string> parseGrsc()
	{
		auto gray = parseDoubleWithID("Gry ");  if (!gray) return std::unexpected{ gray.error() };
		return Grsc{ gray.value() };
	}

	std::expected<HSBC, std::string> parseHSBC()
	{
		auto hue = parseUnitDoubleWithID("#Ang", "H   ");  if (!hue)        return std::unexpected{ hue.error() };
		auto saturate = parseDoubleWithID("Strt");               if (!saturate)   return std::unexpected{ saturate.error() };
		auto brightness = parseDoubleWithID("Brgh");               if (!brightness) return std::unexpected{ brightness.error() };
		return HSBC{ hue.value(), saturate.value(), brightness.value() };
	}

	std::expected<LbCl, std::string> parseLbCl()
	{
		auto luminance = parseDoubleWithID("Lmnc");  if (!luminance) return std::unexpected{ luminance.error() };
		auto a = parseDoubleWithID("A   ");  if (!a)         return std::unexpected{ a.error() };
		auto b = parseDoubleWithID("B   ");  if (!b)         return std::unexpected{ b.error() };
		return LbCl{ luminance.value(), a.value(), b.value() };
	}

	std::expected<BookColor, std::string> parseBkCl()
	{
		if (auto err = expectID("Bk  "); !err) return std::unexpected{ err.error() };
		auto book_name = parseUnicodeString();  if (!book_name)  return std::unexpected{ book_name.error() };
		auto color_name = parseName();           if (!color_name) return std::unexpected{ color_name.error() };
		if (auto err = expectID("bookID"); !err) return std::unexpected{ err.error() };
		auto book_id = parseInt32();          if (!book_id)    return std::unexpected{ book_id.error() };
		if (auto err = expectID("bookKey"); !err) return std::unexpected{ err.error() };
		auto book_key = parseRawData();        if (!book_key)   return std::unexpected{ book_key.error() };
		return BookColor{ book_name.value(), color_name.value(), book_id.value(), book_key.value() };
	}

	std::expected<ColorObjectVariant, std::string> parseColorObjectVariant(std::string_view color_type)
	{
		if (color_type == "BkCl") return parseBkCl();
		if (color_type == "CMYC") return parseCMYC();
		if (color_type == "Grsc") return parseGrsc();
		if (color_type == "HSBC") return parseHSBC();
		if (color_type == "LbCl") return parseLbCl();
		if (color_type == "RGBC") return parseRGBC();
		return std::unexpected{ std::format("Unknown color type \"{}\"", color_type) };
	}

	std::expected<ColorObject, std::string> parseColorObject()
	{
		auto objc = parseObject();
		if (!objc) return std::unexpected{ objc.error() };
		const std::string& color_type = objc.value().class_.id;
		auto variant = parseColorObjectVariant(color_type);
		if (!variant) return std::unexpected{ variant.error() };
		return ColorObject{ color_type, variant.value() };
	}

	std::expected<ColorStopObject, std::string> parseColorStopObject()
	{
		auto objc = parseObject();
		if (!objc) return std::unexpected{ objc.error() };
		if (objc.value().class_.id != "Clrt") return std::unexpected{ "Failed to parse \"Clrt\"" };
		if (auto err = expectID("Clr "); !err) return std::unexpected{ err.error() };
		auto color_object = parseColorObject();
		if (!color_object) return std::unexpected{ color_object.error() };
		if (auto err = expectID("Type"); !err) return std::unexpected{ err.error() };
		auto color_stop_type = parseEnumerated();
		if (!color_stop_type) return std::unexpected{ color_stop_type.error() };
		if (color_stop_type.value().type_id != "Clry") return std::unexpected{ "Failed to parse \"Clry\"" };
		auto location = parseInt32WithID("Lctn");
		if (!location) return std::unexpected{ "Failed to parse \"Lctn\"" };
		auto midpoint = parseInt32WithID("Mdpn");
		if (!midpoint) return std::unexpected{ "Failed to parse \"Mdpn\"" };
		return ColorStopObject{
			static_cast<double>(location.value()),
			static_cast<double>(midpoint.value()),
			color_stop_type.value().value_id,
			color_object.value()
		};
	}

	std::expected<ColorStops, std::string> parseColorStops()
	{
		if (auto err = expectID("Clrs"); !err) return std::unexpected{ err.error() };
		auto list_item_num = parseListItemNum();
		if (!list_item_num) return std::unexpected{ list_item_num.error() };
		ColorStops color_stops;
		color_stops.item_num = list_item_num.value();
		color_stops.color_stop_objects.resize(color_stops.item_num);
		for (auto& object : color_stops.color_stop_objects) {
			auto stop = parseColorStopObject();
			if (!stop) return std::unexpected{ stop.error() };
			object = std::move(stop.value());
		}
		return color_stops;
	}

	std::expected<TransparencyStopObject, std::string> parseTransparencyStopObject()
	{
		auto objc = parseObject();
		if (!objc) return std::unexpected{ objc.error() };
		if (objc.value().class_.id != "TrnS") return std::unexpected{ "Failed to parse \"TrnS\"" };
		auto opacity = parseUnitDoubleWithID("#Prc", "Opct");  if (!opacity)  return std::unexpected{ opacity.error() };
		auto location = parseInt32WithID("Lctn");                if (!location) return std::unexpected{ location.error() };
		auto midpoint = parseInt32WithID("Mdpn");                if (!midpoint) return std::unexpected{ midpoint.error() };
		return TransparencyStopObject{ location.value(), midpoint.value(), opacity.value() };
	}

	std::expected<TransparencyStops, std::string> parseTransparencyStops()
	{
		if (auto err = expectID("Trns"); !err) return std::unexpected{ err.error() };
		auto list_item_num = parseListItemNum();
		if (!list_item_num) return std::unexpected{ list_item_num.error() };
		TransparencyStops transparency_stops;
		transparency_stops.transparency_stop_objects.resize(list_item_num.value());
		for (auto& e : transparency_stops.transparency_stop_objects) {
			auto stop = parseTransparencyStopObject();
			if (!stop) return std::unexpected{ stop.error() };
			e = std::move(stop.value());
		}
		return transparency_stops;
	}

	std::expected<CustomStopsGradientObject, std::string> parseCustomStopsGradientObject()
	{
		auto interpolation = parseInterpolation();
		if (!interpolation) return std::unexpected{ interpolation.error() };
		auto color_stops = parseColorStops();
		if (!color_stops) return std::unexpected{ color_stops.error() };
		auto transparency_stops = parseTransparencyStops();
		if (!transparency_stops) return std::unexpected{ transparency_stops.error() };
		return CustomStopsGradientObject{
			interpolation.value(),
			std::move(color_stops.value()),
			std::move(transparency_stops.value())
		};
	}

	std::expected<ColorNoiseGradientObject, std::string> parseColorNoiseGradientObject()
	{
		ColorNoiseGradientObject obj;
		if (auto err = expectID("ShTr"); !err) return std::unexpected{ err.error() };
		auto show_transparency = parseBool();
		if (!show_transparency) return std::unexpected{ show_transparency.error() };
		obj.show_transparency = show_transparency.value();
		if (auto err = expectID("VctC"); !err) return std::unexpected{ err.error() };
		auto vector_color = parseBool();
		if (!vector_color) return std::unexpected{ vector_color.error() };
		obj.vector_color = vector_color.value();
		if (auto err = expectID("ClrS"); !err) return std::unexpected{ err.error() };
		auto color_space = parseEnumeratedWithTypeID("ClrS");
		if (!color_space) return std::unexpected{ color_space.error() };
		obj.color_space = color_space.value();
		auto random_seed = parseInt32WithID("RndS");
		if (!random_seed) return std::unexpected{ random_seed.error() };
		obj.random_seed = random_seed.value();
		auto smoothness = parseInt32WithID("Smth");
		if (!smoothness) return std::unexpected{ smoothness.error() };
		obj.smoothness = smoothness.value();
		if (auto err = expectID("Mnm "); !err) return std::unexpected{ err.error() };
		[[maybe_unused]] auto min_list_num = parseListItemNum();
		for (auto& e : obj.minimum_values) {
			auto value = parseDouble();
			if (!value) return std::unexpected{ value.error() };
			e = value.value();
		}
		if (auto err = expectID("Mxm "); !err) return std::unexpected{ err.error() };
		[[maybe_unused]] auto max_list_num = parseListItemNum();
		for (auto& e : obj.maximum_values) {
			auto value = parseDouble();
			if (!value) return std::unexpected{ value.error() };
			e = value.value();
		}
		return obj;
	}

	std::expected<Gradient, std::string> parseGradient()
	{
		if (auto objc = parseObject(); !objc) return std::unexpected{ objc.error() };
		if (auto err = expectID("Grad"); !err) return std::unexpected{ err.error() };
		if (auto objc = parseObject(); !objc) return std::unexpected{ objc.error() };
		auto gradient_name = parseName();
		if (!gradient_name) return std::unexpected{ gradient_name.error() };
		auto gradient_form = parseGradientForm();
		if (!gradient_form) return std::unexpected{ gradient_form.error() };
		Gradient gradient;
		gradient.gradient_name = gradient_name.value();
		gradient.gradient_form = gradient_form.value().value_id;
		if (gradient_form.value().value_id == "CstS") {
			auto obj = parseCustomStopsGradientObject();
			if (!obj) return std::unexpected{ obj.error() };
			gradient.gradient_object = std::move(obj.value());
		}
		else if (gradient_form.value().value_id == "ClNs") {
			auto obj = parseColorNoiseGradientObject();
			if (!obj) return std::unexpected{ obj.error() };
			gradient.gradient_object = std::move(obj.value());
		}
		else {
			return std::unexpected{ std::format("Unknown gradient form \"{}\"", gradient_form.value().value_id) };
		}
		return gradient;
	}

	std::expected<GradientList, std::string> parseGradientList()
	{
		if (auto err = expectID("GrdL"); !err) return std::unexpected{ err.error() };
		auto list_item_num = parseListItemNum();
		if (!list_item_num) return std::unexpected{ list_item_num.error() };
		GradientList gradient_list;
		gradient_list.gradient_list.resize(list_item_num.value());
		for (auto& e : gradient_list.gradient_list) {
			auto gradient = parseGradient();
			if (!gradient) return std::unexpected{ gradient.error() };
			e = std::move(gradient.value());
		}
		return gradient_list;
	}

	std::expected<GRD, std::string> parseGRD()
	{
		auto header = parseHeader();
		if (!header) return std::unexpected{ header.error() };
		if (header.value().signature != "8BGR") return std::unexpected{ "Unsupported file format" };
		if (header.value().version != 5)        return std::unexpected{ "Unsupported version" };
		auto descripter_object = parseDescripterObject();
		if (!descripter_object) return std::unexpected{ descripter_object.error() };
		auto gradient_list = parseGradientList();
		if (!gradient_list) return std::unexpected{ gradient_list.error() };
		return GRD{ header.value(), descripter_object.value(), std::move(gradient_list.value()) };
	}

private:
	BinaryReader reader_;
};

// ============================================================
// GrdWriter — GRD オブジェクトを .grd ファイルとして書き込む
// ============================================================

class GrdWriter {
public:
	explicit GrdWriter(std::ofstream& file) : writer_(file) {}

	// パース側の if (!x) return ... パターンと対称にするための簡易マクロ
#define TRY(expr) if (auto _err = (expr); !_err) return std::unexpected{ _err.error() }

	// ---- 基本型 ----

	std::expected<void, std::string> writeInt32(int32_t value)
	{
		TRY(writer_.writeItemType("long"));
		TRY(writer_.writeIntBE(value));
		return {};
	}

	std::expected<void, std::string> writeInt32WithID(std::string_view id, int32_t value)
	{
		TRY(writer_.writeID(id));
		TRY(writeInt32(value));
		return {};
	}

	std::expected<void, std::string> writeBool(bool value)
	{
		TRY(writer_.writeItemType("bool"));
		TRY(writer_.writeBoolean(value));
		return {};
	}

	std::expected<void, std::string> writeDouble(double value)
	{
		TRY(writer_.writeItemType("doub"));
		TRY(writer_.writeDoubleBE(value));
		return {};
	}

	std::expected<void, std::string> writeDoubleWithID(std::string_view id, double value)
	{
		TRY(writer_.writeID(id));
		TRY(writeDouble(value));
		return {};
	}

	std::expected<void, std::string> writeUnitDouble(std::string_view unit_id, double value)
	{
		TRY(writer_.writeItemType("UntF"));
		TRY(writer_.writeItemType(unit_id));
		TRY(writer_.writeDoubleBE(value));
		return {};
	}

	std::expected<void, std::string> writeUnitDoubleWithID(std::string_view id, std::string_view unit_id, double value)
	{
		TRY(writer_.writeID(id));
		TRY(writeUnitDouble(unit_id, value));
		return {};
	}

	std::expected<void, std::string> writeRawData(const std::vector<std::byte>& data)
	{
		TRY(writer_.writeItemType("tdta"));
		TRY(writer_.writeIntBE(static_cast<uint32_t>(data.size())));
		TRY(writer_.writeRawBytes(data));
		return {};
	}

	std::expected<void, std::string> writeUnicodeString(std::string_view text)
	{
		TRY(writer_.writeItemType("TEXT"));
		TRY(writer_.writeUTF16BE(text));
		return {};
	}

	// リストヘッダー: ID + "VlLs" タグ + 要素数
	std::expected<void, std::string> writeListHeader(std::string_view id, int32_t item_num)
	{
		TRY(writer_.writeID(id));
		TRY(writer_.writeItemType("VlLs"));
		TRY(writer_.writeIntBE(item_num));
		return {};
	}

	std::expected<void, std::string> writeEnumerated(std::string_view type_id, std::string_view value_id)
	{
		TRY(writer_.writeItemType("enum"));
		TRY(writer_.writeID(type_id));
		TRY(writer_.writeID(value_id));
		return {};
	}

	// Object ヘッダー: "Objc" タグ + クラス名(UTF-16BE) + クラス ID + キー数
	std::expected<void, std::string> writeObjectHeader(std::string_view class_name, std::string_view class_id, uint32_t key_item_num)
	{
		TRY(writer_.writeItemType("Objc"));
		TRY(writer_.writeUTF16BE(class_name));
		TRY(writer_.writeID(class_id));
		TRY(writer_.writeIntBE(key_item_num));
		return {};
	}

	std::expected<void, std::string> writeName(std::string_view name)
	{
		TRY(writer_.writeID("Nm  "));
		TRY(writeUnicodeString(name));
		return {};
	}

	// ---- ヘッダー / DescripterObject ----

	std::expected<void, std::string> writeHeader(const Header& header)
	{
		TRY(writer_.writeItemType(header.signature));
		TRY(writer_.writeIntBE(header.version));
		TRY(writer_.writeIntBE(header.descriptor_version));
		return {};
	}

	std::expected<void, std::string> writeDescripterObject(const DescripterObject& obj)
	{
		// class_name は空文字列（NULL 終端のみ）として書き込む
		TRY(writer_.writeUTF16BE(""));
		TRY(writer_.writeID("null"));
		TRY(writer_.writeIntBE(obj.key_item_num));
		return {};
	}

	// ---- 色空間 ----

	std::expected<void, std::string> writeRGBC(const RGBC& color)
	{
		TRY(writeDoubleWithID("Rd  ", color.red));
		TRY(writeDoubleWithID("Grn ", color.green));
		TRY(writeDoubleWithID("Bl  ", color.blue));
		return {};
	}

	std::expected<void, std::string> writeCMYC(const CMYC& color)
	{
		TRY(writeDoubleWithID("Cyn ", color.cyan));
		TRY(writeDoubleWithID("Mgnt", color.magenta));
		TRY(writeDoubleWithID("Ylw ", color.yellow));
		TRY(writeDoubleWithID("Blck", color.black));
		return {};
	}

	std::expected<void, std::string> writeGrsc(const Grsc& color)
	{
		TRY(writeDoubleWithID("Gry ", color.gray));
		return {};
	}

	std::expected<void, std::string> writeHSBC(const HSBC& color)
	{
		TRY(writeUnitDoubleWithID("H   ", "#Ang", color.hue));
		TRY(writeDoubleWithID("Strt", color.saturate));
		TRY(writeDoubleWithID("Brgh", color.brightness));
		return {};
	}

	std::expected<void, std::string> writeLbCl(const LbCl& color)
	{
		TRY(writeDoubleWithID("Lmnc", color.luminance));
		TRY(writeDoubleWithID("A   ", color.a));
		TRY(writeDoubleWithID("B   ", color.b));
		return {};
	}

	std::expected<void, std::string> writeBkCl(const BookColor& color)
	{
		TRY(writer_.writeID("Bk  "));
		TRY(writeUnicodeString(color.book_name));
		TRY(writeName(color.color_name));
		TRY(writer_.writeID("bookID"));
		TRY(writeInt32(color.book_id));
		TRY(writer_.writeID("bookKey"));
		TRY(writeRawData(color.book_key));
		return {};
	}

	// ---- カラーオブジェクト ----

	// color_type ごとのキー数（色空間フィールド数に対応）
	static uint32_t colorKeyItemNum(std::string_view color_type)
	{
		if (color_type == "BkCl") return 4; // Bk, Nm, bookID, bookKey
		if (color_type == "CMYC") return 4; // Cyn, Mgnt, Ylw, Blck
		if (color_type == "Grsc") return 1; // Gry
		if (color_type == "HSBC") return 3; // H, Strt, Brgh
		if (color_type == "LbCl") return 3; // Lmnc, A, B
		if (color_type == "RGBC") return 3; // Rd, Grn, Bl
		return 0;
	}

	struct WriteColorObjectVariant {
		GrdWriter& w;
		std::expected<void, std::string> operator()(const BookColor& c) { return w.writeBkCl(c); }
		std::expected<void, std::string> operator()(const CMYC& c) { return w.writeCMYC(c); }
		std::expected<void, std::string> operator()(const Grsc& c) { return w.writeGrsc(c); }
		std::expected<void, std::string> operator()(const HSBC& c) { return w.writeHSBC(c); }
		std::expected<void, std::string> operator()(const LbCl& c) { return w.writeLbCl(c); }
		std::expected<void, std::string> operator()(const RGBC& c) { return w.writeRGBC(c); }
	};

	std::expected<void, std::string> writeColorObject(const ColorObject& color_object)
	{
		TRY(writeObjectHeader("", color_object.color_type, colorKeyItemNum(color_object.color_type)));
		TRY(std::visit(WriteColorObjectVariant{ *this }, color_object.color_object_variant));
		return {};
	}

	// ---- カラーストップ ----

	std::expected<void, std::string> writeColorStopObject(const ColorStopObject& stop)
	{
		// Clrt オブジェクト: Clr(1) + Type(1) + Lctn(1) + Mdpn(1) = 4 キー
		TRY(writeObjectHeader("", "Clrt", 4));
		TRY(writer_.writeID("Clr "));
		TRY(writeColorObject(stop.color_object));
		TRY(writer_.writeID("Type"));
		TRY(writeEnumerated("Clry", stop.type));
		TRY(writeInt32WithID("Lctn", static_cast<int32_t>(stop.location)));
		TRY(writeInt32WithID("Mdpn", static_cast<int32_t>(stop.midpoint)));
		return {};
	}

	std::expected<void, std::string> writeColorStops(const ColorStops& color_stops)
	{
		TRY(writeListHeader("Clrs", color_stops.item_num));
		for (const auto& stop : color_stops.color_stop_objects) {
			TRY(writeColorStopObject(stop));
		}
		return {};
	}

	// ---- 透明度ストップ ----

	std::expected<void, std::string> writeTransparencyStopObject(const TransparencyStopObject& stop)
	{
		// TrnS オブジェクト: Opct(1) + Lctn(1) + Mdpn(1) = 3 キー
		TRY(writeObjectHeader("", "TrnS", 3));
		TRY(writeUnitDoubleWithID("Opct", "#Prc", stop.opacity));
		TRY(writeInt32WithID("Lctn", stop.location));
		TRY(writeInt32WithID("Mdpn", stop.midpoint));
		return {};
	}

	std::expected<void, std::string> writeTransparencyStops(const TransparencyStops& transparency_stops)
	{
		const int32_t item_num = static_cast<int32_t>(transparency_stops.transparency_stop_objects.size());
		TRY(writeListHeader("Trns", item_num));
		for (const auto& stop : transparency_stops.transparency_stop_objects) {
			TRY(writeTransparencyStopObject(stop));
		}
		return {};
	}

	// ---- グラデーションオブジェクト ----

	std::expected<void, std::string> writeCustomStopsGradientObject(const CustomStopsGradientObject& obj)
	{
		TRY(writer_.writeID("Intr"));
		TRY(writeDouble(obj.interpolation));
		TRY(writeColorStops(obj.color_stops));
		TRY(writeTransparencyStops(obj.transparency_stops));
		return {};
	}

	std::expected<void, std::string> writeColorNoiseGradientObject(const ColorNoiseGradientObject& obj)
	{
		TRY(writer_.writeID("ShTr"));
		TRY(writeBool(obj.show_transparency));
		TRY(writer_.writeID("VctC"));
		TRY(writeBool(obj.vector_color));
		TRY(writer_.writeID("ClrS"));
		TRY(writeEnumerated("ClrS", obj.color_space));
		TRY(writeInt32WithID("RndS", obj.random_seed));
		TRY(writeInt32WithID("Smth", obj.smoothness));
		TRY(writeListHeader("Mnm ", static_cast<int32_t>(obj.minimum_values.size())));
		for (double v : obj.minimum_values) { TRY(writeDouble(v)); }
		TRY(writeListHeader("Mxm ", static_cast<int32_t>(obj.maximum_values.size())));
		for (double v : obj.maximum_values) { TRY(writeDouble(v)); }
		return {};
	}

	// ---- グラデーション / グラデーションリスト ----

	// グラデーションオブジェクト本体のキー数
	// CstS: Intr(1) + Clrs(1) + Trns(1) = 3
	// ClNs: ShTr(1) + VctC(1) + ClrS(1) + RndS(1) + Smth(1) + Mnm(1) + Mxm(1) = 7
	static uint32_t gradientObjectKeyItemNum(std::string_view gradient_form)
	{
		if (gradient_form == "CstS") return 3;
		if (gradient_form == "ClNs") return 7;
		return 0;
	}

	struct WriteGradientObject {
		GrdWriter& w;
		std::expected<void, std::string> operator()(const CustomStopsGradientObject& g) { return w.writeCustomStopsGradientObject(g); }
		std::expected<void, std::string> operator()(const ColorNoiseGradientObject& g) { return w.writeColorNoiseGradientObject(g); }
	};

	std::expected<void, std::string> writeGradient(const Gradient& gradient)
	{
		const char* class_name = "Gradient";  // 言語設定によって異なる？日本語環境で出力されたプリセットは "グラデーション" になっていた
		TRY(writeObjectHeader(class_name, "Grdn", 1));
		TRY(writer_.writeID("Grad"));
		const uint32_t inner_key_num = 2 + gradientObjectKeyItemNum(gradient.gradient_form);  // 2 == Nm(1) + GrdF(1)
		TRY(writeObjectHeader(class_name, "Grdn", inner_key_num));
		TRY(writeName(gradient.gradient_name));
		TRY(writer_.writeID("GrdF"));
		TRY(writeEnumerated("GrdF", gradient.gradient_form));
		TRY(std::visit(WriteGradientObject{ *this }, gradient.gradient_object));
		return {};
	}

	std::expected<void, std::string> writeGradientList(const GradientList& gradient_list)
	{
		const int32_t item_num = static_cast<int32_t>(gradient_list.gradient_list.size());
		TRY(writeListHeader("GrdL", item_num));
		for (const auto& gradient : gradient_list.gradient_list) {
			TRY(writeGradient(gradient));
		}
		return {};
	}

	std::expected<void, std::string> writeGRD(const GRD& grd)
	{
		TRY(writeHeader(grd.header));
		TRY(writeDescripterObject(grd.descripter_object));
		TRY(writeGradientList(grd.gradient_list));
		return {};
	}

#undef TRY

private:
	BinaryWriter writer_;
};

//
// 公開 API
//

inline std::expected<GRD, std::string> parseGRD(const std::filesystem::path& file_path)
{
	std::ifstream file(file_path, std::ios::binary);
	if (!file) return std::unexpected{ "Failed to load the file" };
	GrdParser parser(file);
	return parser.parseGRD();
}

inline std::expected<void, std::string> writeGRD(const GRD& grd, const std::filesystem::path& file_path)
{
	std::ofstream file(file_path, std::ios::binary);
	if (!file) return std::unexpected{ "Failed to open the file for writing" };
	GrdWriter writer(file);
	return writer.writeGRD(grd);
}

#endif
