#include "JsonUtility.h"
#include "Logger.h"
#include "StringUtility.h"
#include <cmath>
#include <fstream>
#include <limits>
#include <system_error>
#include <utility>

namespace rd::JsonUtility
{

	using json = nlohmann::json;

	namespace
	{
		// nameの値を返す(なければnullptr)
		const json* Find(const json& object, const char* name)
		{
			// オブジェクト以外に対するfindはend()を返す
			const auto it = object.find(name);
			return it != object.end() ? &*it : nullptr;
		}

		template<typename T>
		bool GetInteger(const json& object, const char* name, T& out)
		{
			const json* value = Find(object, name);
			if (!value || !value->is_number_integer())
			{
				return false;
			}
			// get<T>は範囲外の値を黙って丸めるので、広い型で受けてから確認する
			if (value->is_number_unsigned())
			{
				const uint64_t number = value->get<uint64_t>();
				if (!std::in_range<T>(number))
				{
					return false;
				}
				out = static_cast<T>(number);
			}
			else
			{
				const int64_t number = value->get<int64_t>();
				if (!std::in_range<T>(number))
				{
					return false;
				}
				out = static_cast<T>(number);
			}
			return true;
		}
	}

	std::optional<json> LoadFile(const std::filesystem::path& path)
	{
		std::ifstream file(path);
		if (!file)
		{
			return std::nullopt;
		}

		try
		{
			return json::parse(file);
		}
		catch (const json::exception& e)
		{
			Logger::Error("Failed to parse JSON: {} ({})", StringUtility::ToUtf8(path.native()), e.what());
			return std::nullopt;
		}
	}

	bool SaveFile(const std::filesystem::path& path, const json& root)
	{
		std::error_code error;
		if (path.has_parent_path())
		{
			std::filesystem::create_directories(path.parent_path(), error);
		}

		std::filesystem::path tempPath = path;
		tempPath += L".tmp";
		{
			std::ofstream file(tempPath);
			// 不正なUTF-8が混ざっていても例外を投げないようにする
			file << root.dump(1, '\t', false, json::error_handler_t::replace) << '\n';
			file.close();
			if (!file)
			{
				Logger::Error("Failed to write JSON: {}", StringUtility::ToUtf8(tempPath.native()));
				std::filesystem::remove(tempPath, error);
				return false;
			}
		}

		std::filesystem::rename(tempPath, path, error);
		if (error)
		{
			// error.message()はUTF-8とは限らないのでエラーコードを出力する
			Logger::Error("Failed to replace JSON: {} (error {})", StringUtility::ToUtf8(path.native()), error.value());
			std::filesystem::remove(tempPath, error);
			return false;
		}
		return true;
	}

	const json* FindObject(const json& object, const char* name)
	{
		const json* value = Find(object, name);
		return value && value->is_object() ? value : nullptr;
	}

	bool Get(const json& object, const char* name, bool& out)
	{
		const json* value = Find(object, name);
		if (!value || !value->is_boolean())
		{
			return false;
		}
		out = value->get<bool>();
		return true;
	}

	bool Get(const json& object, const char* name, int32_t& out)
	{
		return GetInteger(object, name, out);
	}

	bool Get(const json& object, const char* name, uint32_t& out)
	{
		return GetInteger(object, name, out);
	}

	bool Get(const json& object, const char* name, float& out)
	{
		const json* value = Find(object, name);
		if (!value || !value->is_number())
		{
			return false;
		}
		const double number = value->get<double>();
		if (std::abs(number) > (std::numeric_limits<float>::max)())
		{
			return false;
		}
		out = static_cast<float>(number);
		return true;
	}

	bool Get(const json& object, const char* name, std::string& out)
	{
		const json* value = Find(object, name);
		if (!value || !value->is_string())
		{
			return false;
		}
		out = value->get<std::string>();
		return true;
	}

} // namespace rd::JsonUtility
