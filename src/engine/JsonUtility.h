#pragma once
#include <cstdint>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

namespace rd::JsonUtility
{

	// ファイルがない、または壊れている場合はnulloptを返す(壊れている場合のみ出力ウィンドウへ通知)
	std::optional<nlohmann::json> LoadFile(const std::filesystem::path& path);
	// 親フォルダがなければ作成する
	// 一時ファイルを経由するので、失敗しても既存のファイルは壊れない(失敗は出力ウィンドウへ通知)
	bool SaveFile(const std::filesystem::path& path, const nlohmann::json& root);

	// nameの子オブジェクトを返す(なければnullptr)
	const nlohmann::json* FindObject(const nlohmann::json& object, const char* name);

	// nameの値をoutへ読み込む
	// キーがない、型が違う、または範囲外の場合はfalseを返し、outは変更しない
	bool Get(const nlohmann::json& object, const char* name, bool& out);
	bool Get(const nlohmann::json& object, const char* name, int32_t& out);
	bool Get(const nlohmann::json& object, const char* name, uint32_t& out);
	// 手で編集された場合を考慮して、整数の書式(1など)も受け付ける
	bool Get(const nlohmann::json& object, const char* name, float& out);
	bool Get(const nlohmann::json& object, const char* name, std::string& out);

	// nameへ値を書き込む
	template<typename T>
	void Set(nlohmann::json& object, const char* name, const T& in)
	{
		object[name] = in;
	}

} // namespace rd::JsonUtility
