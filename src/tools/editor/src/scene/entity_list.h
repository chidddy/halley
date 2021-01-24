#pragma once
#include "halley/ui/ui_widget.h"
#include "halley/ui/widgets/ui_tree_list.h"

namespace Halley {
	class EntityIcons;
	class SceneEditorWindow;
	class UIFactory;
	class EntityTree;
	class ISceneData;

	class EntityList final : public UIWidget {
	public:
		EntityList(String id, UIFactory& factory);

		void setSceneEditorWindow(SceneEditorWindow& sceneEditor);
		void setSceneData(std::shared_ptr<ISceneData> sceneData);

		void refreshList();
		void refreshNames();

		void onEntityModified(const String& id, const EntityData& node);
		void onEntityAdded(const String& id, const String& parentId, int childIndex, const EntityData& data);
		void onEntityRemoved(const String& id, const String& newSelectionId);
		void select(const String& id);

	protected:
		bool onKeyPress(KeyboardKeyPress key) override;
		
	private:
		UIFactory& factory;
		SceneEditorWindow* sceneEditor;
		const EntityIcons* icons = nullptr;

		std::shared_ptr<UITreeList> list;
		std::shared_ptr<ISceneData> sceneData;

		void makeUI();
		void addEntities(const EntityTree& entity, const String& parentId);
		void addEntity(const String& name, const String& id, const String& parentId, int childIndex, const String& prefab, const String& icon);
		void addEntityTree(const String& parentId, int childIndex, const EntityData& data);
		std::pair<String, Sprite> getEntityNameAndIcon(const EntityData& data) const;
		std::pair<String, Sprite> getEntityNameAndIcon(const String& name, const String& icon, const String& prefab) const;
	};
}
