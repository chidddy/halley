#include "halley/file_formats/config_file.h"
#include "halley/core/game/game.h"
#include "world.h"
#include "entity_stage.h"
using namespace Halley;

std::unique_ptr<World> EntityStage::createWorld(const String& configName)
{
	return World::make(getAPI(), getResources(), configName, getGame().isDevMode());
}
