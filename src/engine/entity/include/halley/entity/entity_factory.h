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

		EntityRef createEntity(const char* prefabName);
		EntityRef createEntity(const String& prefabName);
		EntityRef createEntity(const std::shared_ptr<const Prefab>& prefab);
		EntityRef createEntity(const EntityData& data, EntityRef parent = EntityRef());
		EntityScene createScene(const std::shared_ptr<const Prefab>& scene);

		void updateEntity(EntityRef& entity, const EntityData& data);
		void updateScene(std::vector<EntityRef>& entities, const std::shared_ptr<const Prefab>& scene, EntitySerialization::Type sourceType);

		EntityData serializeEntity(EntityRef entity, const SerializationOptions& options, bool canStoreParent = true);

	private:
		World& world;
		Resources& resources;

		EntityRef createEntityTree(const EntityData& data, EntityRef parent, const std::shared_ptr<const Prefab>& prevPrefab);
		EntityRef createEntityNode(const EntityData& data, EntityRef parent, const std::shared_ptr<const Prefab>& prefab);

		std::shared_ptr<const Prefab> getPrefab(const String& id) const;

		std::shared_ptr<const EntityFactoryContext> makeContext(EntitySerialization::Type type) const;
	};

	class EntityFactoryContext {
	public:
		ConfigNodeSerializationContext configNodeContext;
		
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
	};
}
