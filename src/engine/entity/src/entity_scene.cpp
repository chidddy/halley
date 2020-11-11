#include "entity_scene.h"

#include "entity_factory.h"
#include "world.h"
#include "halley/support/logger.h"
#include "halley/utils/algorithm.h"
using namespace Halley;

EntityScene::EntityScene(bool allowReload, uint8_t worldPartition)
	: allowReload(allowReload)
	, worldPartition(worldPartition)
{
}

std::vector<EntityRef>& EntityScene::getEntities()
{
	return entities;
}

const std::vector<EntityRef>& EntityScene::getEntities() const
{
	return entities;
}

bool EntityScene::needsUpdate() const
{
	for (const auto& entry: sceneObservers) {
		if (entry.needsUpdate()) {
			return true;
		}
	}
	for (const auto& entry: prefabObservers) {
		if (entry.needsUpdate()) {
			return true;
		}
	}
	return false;
}

void EntityScene::update(EntityFactory& factory)
{
	// Collect all prefabs that changed
	for (auto& entry: prefabObservers) {
		if (entry.needsUpdate()) {
			entry.updateEntities(factory, *this);
			entry.markUpdated();		
		}
	}

	// Update scenes
	for (auto& entry: sceneObservers) {
		if (entry.needsUpdate()) {
			entry.updateEntities(factory, *this);
			entry.markUpdated();
		}
	}
}

void EntityScene::updateOnEditor(EntityFactory& factory)
{
	update(factory);
}

void EntityScene::addPrefabReference(const std::shared_ptr<const Prefab>& prefab, const EntityRef& entity)
{
	if (allowReload) {
		getOrMakeObserver(prefab).addEntity(entity);
	}
}

void EntityScene::addRootEntity(EntityRef entity)
{
	entities.emplace_back(entity);
}

uint8_t EntityScene::getWorldPartition() const
{
	return worldPartition;
}

EntityScene::PrefabObserver::PrefabObserver(std::shared_ptr<const Prefab> prefab)
	: prefab(std::move(prefab))
{
	assetVersion = this->prefab->getAssetVersion();
}

bool EntityScene::PrefabObserver::needsUpdate() const
{
	return assetVersion != prefab->getAssetVersion();
}

void EntityScene::PrefabObserver::updateEntities(EntityFactory& factory, EntityScene& scene) const
{
	const auto& modified = prefab->getEntitiesModified();
	const auto& removed = prefab->getEntitiesRemoved();
	const auto& dataMap = prefab->getEntityDataMap();

	if (!prefab->isScene()) {
		assert(modified.size() == 1 && removed.empty());
	}

	// Modified entities
	for (auto& entity: getEntities(factory.getWorld())) {
		const auto& uuid = prefab->isScene() ? entity.getInstanceUUID() : entity.getPrefabUUID();
		
		auto deltaIter = modified.find(uuid);
		if (deltaIter != modified.end()) {
			// A simple delta is available for this entity, apply that
			factory.updateEntity(entity, deltaIter->second);
		} else if (removed.find(uuid) != removed.end()) {
			// Remove
			factory.getWorld().destroyEntity(entity);
		}
	}

	// Added
	for (const auto& uuid: prefab->getEntitiesAdded()) {
		auto dataIter = dataMap.find(uuid);
		if (dataIter != dataMap.end()) {
			// Create
			factory.createEntity(*dataIter->second, {}, &scene);
		} else {
			// Not found
			Logger::logError("PrefabObserver::update error: UUID " + uuid.toString() + " not found in prefab " + prefab->getAssetId());
		}
	}
}

void EntityScene::PrefabObserver::markUpdated()
{
	assetVersion = prefab->getAssetVersion();
}

void EntityScene::PrefabObserver::addEntity(EntityRef entity)
{
	if (!std_ex::contains(entityIds, entity.getEntityId())) {
		entityIds.push_back(entity.getEntityId());
	}
}

const std::shared_ptr<const Prefab>& EntityScene::PrefabObserver::getPrefab() const
{
	return prefab;
}

std::vector<EntityRef> EntityScene::PrefabObserver::getEntities(World& world) const
{
	std::vector<EntityRef> entities;
	for (const auto& id: entityIds) {
		auto* entity = world.tryGetRawEntity(id);
		if (entity) {
			entities.emplace_back(*entity, world);
		}
	}
	return entities;
}

EntityScene::PrefabObserver& EntityScene::getOrMakeObserver(const std::shared_ptr<const Prefab>& prefab)
{
	auto& list = prefab->isScene() ? sceneObservers : prefabObservers;
	
	for (auto& o: list) {
		if (o.getPrefab() == prefab) {
			return o;
		}
	}
	return list.emplace_back(prefab);
}
