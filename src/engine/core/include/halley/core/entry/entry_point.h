#pragma once

#include <memory>
#include <vector>
#include "halley/core/game/main_loop.h"

#ifdef _WIN32
#define HALLEY_STDCALL __stdcall
#else
#define HALLEY_STDCALL
#endif

namespace Halley
{
	class Game;
	class Core;

	constexpr static uint32_t HALLEY_DLL_API_VERSION = 20;
	
	class IHalleyEntryPoint
	{
	public:
		virtual ~IHalleyEntryPoint() = default;
		virtual std::unique_ptr<Core> createCore(const std::vector<std::string>& args) = 0;
		virtual std::unique_ptr<Game> createGame() = 0;
		virtual uint32_t getApiVersion() { return HALLEY_DLL_API_VERSION; }
	};

	template <typename GameType>
	class HalleyEntryPoint final : public IHalleyEntryPoint
	{
	public:
		std::unique_ptr<Game> createGame() override
		{
			return std::make_unique<GameType>();
		}

		std::unique_ptr<Core> createCore(const std::vector<std::string>& args) override
		{
			Expects(args.size() >= 1);
			Expects(args.size() < 1000);
			return std::make_unique<Core>(std::make_unique<GameType>(), args);
		}
	};
}
