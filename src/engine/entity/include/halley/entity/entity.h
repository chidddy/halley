#pragma once

#include <array>
#include <memory>
#include "component.h"
#include "message.h"
#include "family_mask.h"
#include "entity_id.h"
#include "type_deleter.h"
#include <halley/data_structures/vector.h>
#include "halley/utils/type_traits.h"
#include "halley/maths/uuid.h"

namespace Halley {
	class World;
	class System;
	class EntityRef;
	class Prefab;

	// True if T::onAddedToEntity(EntityRef&) exists
	template <class, class = void_t<>> struct HasOnAddedToEntityMember : std::false_type {};
	template <class T> struct HasOnAddedToEntityMember<T, decltype(std::declval<T&>().onAddedToEntity(std::declval<EntityRef&>()))> : std::true_type { };
	
	class MessageEntry
	{
	public:
		std::unique_ptr<Message> msg;
		int type = -1;
		int age = -1;

		MessageEntry() {}
		MessageEntry(std::unique_ptr<Message> msg, int type, int age) : msg(std::move(msg)), type(type), age(age) {}
	};

	class EntityRef;
	class ConstEntityRef;

	class Entity
	{
		friend class World;
		friend class System;
		friend class EntityRef;
		friend class ConstEntityRef;

	public:
		~Entity();

		template <typename T>
		T* tryGetComponent()
		{
			constexpr int id = FamilyMask::RetrieveComponentIndex<T>::componentIndex;
			for (uint8_t i = 0; i < liveComponents; i++) {
				if (components[i].first == id) {
					return static_cast<T*>(components[i].second);
				}
			}
			return nullptr;
		}

		template <typename T>
		const T* tryGetComponent() const
		{
			constexpr int id = FamilyMask::RetrieveComponentIndex<T>::componentIndex;
			for (uint8_t i = 0; i < liveComponents; i++) {
				if (components[i].first == id) {
					return static_cast<const T*>(components[i].second);
				}
			}
			return nullptr;
		}

		template <typename T>
		T& getComponent()
		{
			auto value = tryGetComponent<T>();
			if (value) {
				return *value;
			} else {
				throw Exception("Component " + String(typeid(T).name()) + " does not exist in entity.", HalleyExceptions::Entity);
			}
		}

		template <typename T>
		const T& getComponent() const
		{
			auto value = tryGetComponent<T>();
			if (value) {
				return *value;
			} else {
				throw Exception("Component " + String(typeid(T).name()) + " does not exist in entity.", HalleyExceptions::Entity);
			}
		}

		template <typename T>
		bool hasComponent(World& world) const
		{
			if (dirty) {
				return tryGetComponent<T>() != nullptr;
			} else {
				return hasBit(world, FamilyMask::RetrieveComponentIndex<T>::componentIndex);
			}
		}

		bool needsRefresh() const
		{
			return dirty;
		}

		bool isAlive() const
		{
			return alive;
		}
		
		const UUID& getPrefabUUID() const
		{
			return prefabUUID;
		}

		const UUID& getInstanceUUID() const
		{
			return instanceUUID;
		}

		FamilyMaskType getMask() const;
		EntityId getEntityId() const;

		void refresh(MaskStorage& storage, ComponentDeleterTable& table);
		void destroy();
		
		void sortChildrenByPrefabUUIDs(const std::vector<UUID>& uuids);

		void setWorldPartition(uint8_t partition);

		bool isEmpty() const;

	private:
		// !!! WARNING !!!
		// The order of elements in this class was carefully chosen to maximise cache performance!
		// Be SURE to verify that no performance-critical fields get bumped to a worse cacheline if you change anything here

		// Cacheline 0
		Vector<std::pair<int, Component*>> components;
		uint8_t liveComponents = 0;
		bool dirty : 1;
		bool alive : 1;
		bool serializable : 1;
		bool reloaded : 1;
		
		uint8_t childrenRevision = 0;
		uint8_t worldPartition = 0;

		FamilyMaskType mask;
		Entity* parent = nullptr;
		EntityId entityId;
		Vector<Entity*> children; // Cacheline 1 starts 16 bytes into this

		// Cacheline 1
		Vector<MessageEntry> inbox;
		String name;

		// Cacheline 2
		UUID instanceUUID;
		UUID prefabUUID;
		std::shared_ptr<const Prefab> prefab;

		uint8_t hierarchyRevision = 0;

		Entity();
		void destroyComponents(ComponentDeleterTable& storage);

		template <typename T>
		Entity& addComponent(World& world, T* component)
		{
			addComponent(component, T::componentIndex);
			TypeDeleter<T>::initialize(getComponentDeleterTable(world));

			markDirty(world);
			return *this;
		}

		template <typename T>
		Entity& removeComponent(World& world)
		{
			removeComponentById(world, T::componentIndex);
			return *this;
		}

		void addComponent(Component* component, int id);
		void removeComponentAt(int index);
		void removeComponentById(World& world, int id);
		void removeAllComponents(World& world);
		void deleteComponent(Component* component, int id, ComponentDeleterTable& table);
		void keepOnlyComponentsWithIds(const std::vector<int>& ids, World& world);

		void onReady();

		void markDirty(World& world);
		ComponentDeleterTable& getComponentDeleterTable(World& world);

		Entity* getParent() const { return parent; }
		void setParent(Entity* parent, bool propagate = true, size_t childIdx = -1);
		const std::vector<Entity*>& getChildren() const { return children; }
		void addChild(Entity& child);
		void detachChildren();
		void markHierarchyDirty();
		void propagateChildrenChange();
		void propagateChildWorldPartition(uint8_t newWorldPartition);

		void doDestroy(bool updateParenting);

		bool hasBit(World& world, int index) const;
	};

	class EntityRef;
	
	class EntityRefIterable {
	public:
		class Iterator {
		public:
			Iterator(std::vector<Entity*>::const_iterator iter, World& world)
				: iter(iter)
				, world(world)
			{}

			Iterator& operator++()
			{
				++iter;
				return *this;
			}

			bool operator==(const Iterator& other)
			{
				return iter == other.iter;
			}

			bool operator!=(const Iterator& other)
			{
				return iter != other.iter;
			}

			EntityRef operator*() const;

		private:
			std::vector<Entity*>::const_iterator iter;
			World& world;
		};
		
		EntityRefIterable(const std::vector<Entity*>& entities, World& world)
			: entities(entities)
			, world(world)
		{}

		Iterator begin() const
		{
			return Iterator(entities.begin(), world);
		}

		Iterator end() const
		{
			return Iterator(entities.end(), world);
		}

	private:
		const std::vector<Entity*>& entities;
		World& world;
	};

	class EntityRef
	{
	public:
		EntityRef() = default;
		EntityRef(const EntityRef& other) = default;
		EntityRef(EntityRef&& other) noexcept = default;

		EntityRef& operator=(const EntityRef& other) = default;
		EntityRef& operator=(EntityRef&& other) noexcept = default;

		EntityRef(Entity& e, World& w)
			: entity(&e)
			, world(&w)
		{}

		~EntityRef() = default;

		template <typename T>
		EntityRef& addComponent(T&& component)
		{
			Expects(entity != nullptr);
			
			static_assert(!std::is_pointer<T>::value, "Cannot pass pointer to component");
			static_assert(!std::is_same<T, Component>::value, "Cannot add base class Component to entity, make sure type isn't being erased");
			static_assert(std::is_base_of<Component, T>::value, "Components must extend the Component class");
			static_assert(!std::is_polymorphic<T>::value, "Components cannot be polymorphic (i.e. they can't have virtual methods)");
			static_assert(std::is_default_constructible<T>::value, "Components must have a default constructor");

			auto c = new T(std::move(component));
			entity->addComponent(*world, c);

			if constexpr (HasOnAddedToEntityMember<T>::value) {
				c->onAddedToEntity(*this);
			}

			return *this;
		}

		template <typename T>
		EntityRef& removeComponent()
		{
			Expects(entity != nullptr);
			Expects(world != nullptr);
			entity->removeComponent<T>(*world);
			return *this;
		}

		EntityRef& removeComponentById(int id)
		{
			Expects(entity != nullptr);
			Expects(world != nullptr);
			entity->removeComponentById(*world, id);
			return *this;
		}

		EntityRef& removeAllComponents()
		{
			Expects(entity != nullptr);
			Expects(world != nullptr);
			entity->removeAllComponents(*world);
			return *this;
		}

		template <typename T>
		T& getComponent()
		{
			Expects(entity != nullptr);
			return entity->getComponent<T>();
		}

		template <typename T>
		const T& getComponent() const
		{
			Expects(entity != nullptr);
			return entity->getComponent<T>();
		}

		template <typename T>
		T* tryGetComponent()
		{
			Expects(entity != nullptr);
			return entity->tryGetComponent<T>();
		}

		template <typename T>
		const T* tryGetComponent() const
		{
			Expects(entity != nullptr);
			return entity->tryGetComponent<T>();
		}

		EntityId getEntityId() const
		{
			Expects(entity != nullptr);
			return entity->getEntityId();
		}

		template <typename T>
		bool hasComponent() const
		{
			Expects(entity != nullptr);
			return entity->hasComponent<T>(*world);
		}

		template <typename T>
		bool hasComponentInTree() const
		{
			if (hasComponent<T>()) {
				return true;
			}
			for (auto& c: getRawChildren()) {
				if (EntityRef(*c, getWorld()).hasComponentInTree<T>()) {
					return true;
				}
			}
			return false;
		}

		const String& getName() const
		{
			Expects(entity != nullptr);
			return entity->name;
		}

		void setName(String name)
		{
			Expects(entity != nullptr);
			entity->name = std::move(name);
		}

		const UUID& getInstanceUUID() const
		{
			Expects(entity != nullptr);
			return entity->instanceUUID;
		}

		const UUID& getPrefabUUID() const
		{
			Expects(entity != nullptr);
			return entity->prefabUUID;
		}
		
		void keepOnlyComponentsWithIds(const std::vector<int>& ids)
		{
			Expects(entity != nullptr);
			entity->keepOnlyComponentsWithIds(ids, *world);
		}

		bool hasParent() const
		{
			Expects(entity != nullptr);
			return entity->getParent() != nullptr;
		}
		
		EntityRef getParent() const
		{
			Expects(entity != nullptr);
			return EntityRef(*entity->getParent(), *world);
		}

		std::optional<EntityRef> tryGetParent() const
		{
			Expects(entity != nullptr);
			const auto parent = entity->getParent();
			return parent != nullptr ? EntityRef(*parent, *world) : std::optional<EntityRef>();
		}

		void setParent(EntityRef& parent, size_t childIdx = -1)
		{
			Expects(entity != nullptr);
			entity->setParent(parent.entity, true, childIdx);
		}

		void setParent()
		{
			Expects(entity != nullptr);
			entity->setParent(nullptr);
		}

		const std::vector<Entity*>& getRawChildren() const
		{
			Expects(entity != nullptr);
			return entity->getChildren();
		}

		EntityRefIterable getChildren() const
		{
			Expects(entity != nullptr);
			return EntityRefIterable(entity->getChildren(), *world);
		}

		bool hasChildren() const
		{
			Expects(entity != nullptr);
			return !entity->getChildren().empty();
		}

		void addChild(EntityRef& child)
		{
			Expects(entity != nullptr);
			entity->addChild(*child.entity);
		}

		void detachChildren()
		{
			Expects(entity != nullptr);
			entity->detachChildren();
		}

		uint8_t getHierarchyRevision() const
		{
			Expects(entity != nullptr);
			return entity->hierarchyRevision;
		}

		uint8_t getChildrenRevision() const
		{
			Expects(entity != nullptr);
			return entity->childrenRevision;
		}

		uint8_t getWorldPartition() const
		{
			Expects(entity != nullptr);
			return entity->worldPartition;
		}

		void setWorldPartition(uint8_t partition)
		{
			Expects(entity != nullptr);
			entity->setWorldPartition(partition);
		}

		bool isValid() const
		{
			return entity != nullptr;
		}

		bool operator==(const EntityRef& other) const
		{
			return entity == other.entity && world == other.world;
		}

		bool operator!=(const EntityRef& other) const
		{
			return !(*this == other);
		}

		World& getWorld() const
		{
			return *world;
		}

		size_t getNumComponents() const
		{
			Expects(entity);
			return static_cast<size_t>(entity->liveComponents);
		}

		std::pair<int, Component*> getRawComponent(size_t idx) const
		{
			Expects(entity);
			return entity->components[idx];
		}

		std::vector<std::pair<int, Component*>>::iterator begin() const
		{
			Expects(entity);
			return entity->components.begin();
		}

		std::vector<std::pair<int, Component*>>::iterator end() const
		{
			Expects(entity);
			return entity->components.begin() + entity->liveComponents;
		}

		EntityRef& setSerializable(bool serializable)
		{
			Expects(entity);
			entity->serializable = serializable;
			return *this;
		}

		bool isSerializable() const
		{
			Expects(entity);
			return entity->serializable;
		}

		void setReloaded();

		bool wasReloaded()
		{
			Expects(entity);
			return entity->reloaded;
		}

		void sortChildrenByPrefabUUIDs(const std::vector<UUID>& uuids)
		{
			Expects(entity);
			entity->sortChildrenByPrefabUUIDs(uuids);
		}

		void setPrefab(std::shared_ptr<const Prefab> prefab, UUID prefabUUID)
		{
			Expects(entity);
			Expects(!prefab || prefabUUID.isValid());
			entity->prefab = std::move(prefab);
			entity->prefabUUID = prefabUUID;
		}

		const std::shared_ptr<const Prefab>& getPrefab() const
		{
			Expects(entity);
			return entity->prefab;
		}

		bool isEmpty() const
		{
			return !entity || entity->isEmpty();
		}

	private:
		friend class World;

		Entity* entity = nullptr;
		World* world = nullptr;
	};
	
	class ConstEntityRef
	{
	public:
		ConstEntityRef() = default;
		ConstEntityRef(const ConstEntityRef& other) = default;
		ConstEntityRef(ConstEntityRef&& other) = default;

		ConstEntityRef& operator=(const ConstEntityRef& other) = default;
		ConstEntityRef& operator=(ConstEntityRef&& other) = default;

		ConstEntityRef(Entity& e, const World& w)
			: entity(&e)
			, world(&w)
		{}
		
		template <typename T>
		const T& getComponent() const
		{
			return entity->getComponent<T>();
		}

		template <typename T>
		const T* tryGetComponent() const
		{
			return entity->tryGetComponent<T>();
		}

		EntityId getEntityId() const
		{
			return entity->getEntityId();
		}

		template <typename T>
		bool hasComponent() const
		{
			return entity->hasComponent<T>(*world);
		}

		const String& getName() const
		{
			return entity->name;
		}

		const UUID& getInstanceUUID() const
		{
			return entity->instanceUUID;
		}

		const UUID& getPrefabUUID() const
		{
			return entity->prefabUUID;
		}

		bool hasParent() const
		{
			return entity->getParent() != nullptr;
		}

		ConstEntityRef getParent() const
		{
			return ConstEntityRef(*entity->getParent(), *world);
		}

		std::optional<ConstEntityRef> tryGetParent() const
		{
			const auto parent = entity->getParent();
			return parent != nullptr ? ConstEntityRef(*parent, *world) : std::optional<ConstEntityRef>();
		}

		[[deprecated]] const std::vector<Entity*>& getRawChildren() const
		{
			return entity->getChildren();
		}

		uint8_t getHierarchyRevision() const
		{
			return entity->hierarchyRevision;
		}

		uint8_t getChildrenRevision() const
		{
			return entity->childrenRevision;
		}

		size_t getNumComponents() const
		{
			Expects(entity);
			return size_t(entity->liveComponents);
		}

		std::pair<int, Component*> getRawComponent(size_t idx) const
		{
			Expects(entity);
			return entity->components[idx];
		}

		std::vector<std::pair<int, Component*>>::iterator begin() const
		{
			Expects(entity);
			return entity->components.begin();
		}

		std::vector<std::pair<int, Component*>>::iterator end() const
		{
			Expects(entity);
			return entity->components.begin() + entity->liveComponents;
		}

		bool isSerializable() const
		{
			Expects(entity);
			return entity->serializable;
		}

	private:
		friend class World;

		Entity* entity;
		const World* world;
	};

	inline EntityRef EntityRefIterable::Iterator::operator*() const
	{
		return EntityRef(*(*iter), world);
	}
	
}
