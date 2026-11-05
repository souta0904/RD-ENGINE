#pragma once
#include <cstdint>
#include <string_view>

namespace rd
{

	// エンジンの名前とバージョン
	namespace EngineInfo
	{
		inline constexpr std::wstring_view kName = L"RD ENGINE";
		inline constexpr uint32_t kVersionMajor = 1;
		inline constexpr uint32_t kVersionMinor = 0;
		inline constexpr uint32_t kVersionPatch = 0;
	}

} // namespace rd
