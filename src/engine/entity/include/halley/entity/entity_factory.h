#pragma once
#include <functional>

#include "create_functions.h"
#include "prefab.h"
#include "halley/file_formats/config_file.h"
#include "halley/data_structures/maybe.h"
#include "halley/entity/entity.h"

namespace Halley {
	class World;
	class Resources;
	class EntityScene;
	class EntityData;
	
	class EntityFactory {
	public:
		enum class UpdateMode {
			//TransformOnly,
			UpdateAll,
			UpdateAllDeleteOld
		};

		struct SerializationOptions {
			EntitySerialization::Type type = EntitySerialization::Type::Undefined;
			std::function<bool(EntityRef)> serializeAsStub;

			SerializationOptions() = default;
			explicit SerializationOptions(EntitySerialization::Type type, std::function<bool(EntityRef)> serializeAsStub = {})
				: type(type)
				, serializeAsStub(std::move(serializeAsStub))
			{}
		};

		explicit EntityFactory(World& world, Resources& resources);
		virtual ~EntityFactory();

		EntityRef createEntity(const String& prefabName);
		EntityRef createEntity(const EntityData& data, EntityRef parent = EntityRef());
		EntityScene createScene(const std::shared_ptr<const Prefab>& scene);

		void updateEntity(EntityRef& entity, const EntityData& data);
		void updateScene(std::vector<EntityRef>& entities, const std::shared_ptr<const Prefab>& scene);

		EntityData serializeEntity(EntityRef entity, const SerializationOptions& options, bool canStoreParent = true);

	private:
		World& world;
		Resources& resources;

		EntityRef updateEntityNode(const EntityData& data, std::optional<EntityRef> parent, const std::shared_ptr<EntityFactoryContext>& context);
		void updateEntityComponents(EntityRef entity, const EntityData& data, const EntityFactoryContext& context);
		void updateEntityChildren(EntityRef entity, const EntityData& data, const std::shared_ptr<EntityFactoryContext>& context);

		std::shared_ptr<EntityFactoryContext> makeContext(const EntityData& data, std::optional<EntityRef> existing);
		EntityRef instantiateEntity(const EntityData& data, EntityFactoryContext& context, bool allowWorldLookup);
		EntityRef getEntity(const EntityData& data, EntityFactoryContext& context, bool allowWorldLookup);
		void preInstantiateEntities(const EntityData& data, EntityFactoryContext& context, int depth);
		void collectExistingEntities(EntityRef entity, EntityFactoryContext& context);

		[[nodiscard]] std::shared_ptr<const Prefab> getPrefab(const String& id) const;
		[[nodiscard]] std::shared_ptr<const EntityFactoryContext> makeContext(EntitySerialization::Type type, std::shared_ptr<const Prefab> prefab) const;
	};

	class EntityFactoryContext {
	public:
		EntityFactoryContext(World& world, Resources& resources, int entitySerializationMask, std::shared_ptr<const Prefab> prefab = {}, const EntityData* origEntityData = nullptr);
		
		template <typename T>
		CreateComponentFunctionResult createComponent(EntityRef& e, const ConfigNode& componentData) const
		{
			CreateComponentFunctionResult result;
			result.componentId = T::componentIndex;
			
			auto comp = e.tryGetComponent<T>();
			if (comp) {
				comp->deserialize(configNodeContext, componentData);
			} else {
				T component;
				component.deserialize(configNodeContext, componentData);
				e.addComponent<T>(std::move(component));
				result.created = true;
			}

			return result;
		}

		const std::shared_ptr<const Prefab>& getPrefab() const { return prefab; }
		const ConfigNodeSerializationContext& getConfigNodeContext() const { return configNodeContext; }
		World& getWorld() const { return *world; }
		EntityId getEntityIdFromUUID(const UUID& uuid) const;

		void addEntity(EntityRef entity);
		EntityRef getEntity(const UUID& uuid, bool allowPrefabUUID) const;

		bool needsNewContextFor(const EntityData& value) const;

		const EntityData& getRootEntityData() const;

	private:
		ConfigNodeSerializationContext configNodeContext;
		std::shared_ptr<const Prefab> prefab;
		World* world;
		std::vector<EntityRef> entities;

		const EntityData* entityData;
		EntityData instancedEntityData;
	};
}
