#pragma once
#include "entity_editor.h"
#include "entity_list.h"
#include "halley/ui/ui_widget.h"
#include "scene_editor_canvas.h"
#include "halley/tools/dll/dynamic_library.h"
#include "undo_stack.h"

namespace Halley {
	class ProjectWindow;
	class AssetBrowserTabs;
	class HalleyAPI;
	class Project;
	class UIFactory;
	class EntityFactory;

	class SceneEditorWindow final : public UIWidget, public IDynamicLibraryListener, public ISceneEditorWindow {
	public:
		SceneEditorWindow(UIFactory& factory, Project& project, const HalleyAPI& api, ProjectWindow& projectWindow);
		~SceneEditorWindow();

		void onAddedToRoot() override;
		
		void loadScene(const String& sceneName);
		void loadPrefab(const String& name);
		void loadScene(AssetType type, const Prefab& prefab);
		void unloadScene();

		void onEntityAdded(const String& id, const String& parentId, int childIndex) override;
		void onEntityRemoved(const String& id, const String& parentId, int childIndex, const EntityData& prevData) override;
		void onEntityModified(const String& id, const EntityData& prevData, const EntityData& newData) override;
		void onEntityMoved(const String& id, const String& prevParentId, int prevChildIndex, const String& newParentId, int newChildIndex) override;
		void onComponentRemoved(const String& name) override;
		void onFieldChangedByGizmo(const String& componentName, const String& fieldName) override;

		void addNewEntity();
		void addNewPrefab();
		void addNewPrefab(const String& prefabName);
		void addEntity(EntityData data);
		void addEntity(const String& referenceEntityId, bool childOfReference, EntityData data);
		void addEntity(const String& parentId, int childIndex, EntityData data);
		void removeEntity();
		void removeEntity(const String& entityId) override;
		void selectEntity(const String& id);
		void selectEntity(const std::vector<UUID>& candidates);
		void modifyEntity(const String& id, const EntityDataDelta& delta);
		void moveEntity(const String& id, const String& newParent, int childIndex);

		void setTool(SceneEditorTool tool);
		void setTool(SceneEditorTool tool, const String& componentName, const String& fieldName, ConfigNode options);

		std::shared_ptr<const Prefab> getGamePrefab(const String& id) const;

		void copyEntityToClipboard(const String& id);
		void pasteEntityFromClipboard(const String& referenceId);
		String copyEntity(const String& id);
		void pasteEntity(const String& data, const String& referenceId);
		void duplicateEntity(const String& id);
		void openEditPrefabWindow(const String& name);

		const std::shared_ptr<ISceneData>& getSceneData() const override;

		void markModified() override;
		void clearModifiedFlag();
		bool isModified() const;
		const EntityIcons& getEntityIcons() const;

	protected:
		void update(Time t, bool moved) override;

		bool onKeyPress(KeyboardKeyPress key) override;

		void onUnloadDLL() override;
		void onLoadDLL() override;

	private:
		const HalleyAPI& api;
		UIFactory& uiFactory;
		Project& project;
		ProjectWindow& projectWindow;

		std::shared_ptr<SceneEditorGameBridge> gameBridge;
		std::shared_ptr<SceneEditorCanvas> canvas;
		std::shared_ptr<EntityList> entityList;
		std::shared_ptr<EntityEditor> entityEditor;
		std::shared_ptr<UIList> toolMode;
		std::shared_ptr<EntityIcons> entityIcons;

		Path assetPath;
		std::shared_ptr<ISceneData> sceneData;
		std::shared_ptr<Prefab> prefab;
		AssetType origPrefabAssetType;
		std::shared_ptr<EntityFactory> entityFactory;
		std::optional<EntityScene> currentEntityScene;

		String currentEntityId;

		std::shared_ptr<UIWidget> curCustomUI;
		std::shared_ptr<UIWidget> curToolUI;
		SceneEditorTool curTool = SceneEditorTool::None;
		String curComponentName;

		int toolModeTimeout = 0; // Hack

		UndoStack undoStack;
		bool modified = false;

		void makeUI();
		void onEntitySelected(const String& id);
		void panCameraToEntity(const String& id);
		void saveScene();

		String findParent(const String& entityId) const;
		const String* findParent(const String& entityId, const EntityTree& tree, const String& prev) const;

		void setCustomUI(std::shared_ptr<UIWidget> ui);
		void setToolUI(std::shared_ptr<UIWidget> ui);

		void decayTool();

		void setModified(bool enabled);

		String serializeEntity(const EntityData& node) const;
		std::optional<EntityData> deserializeEntity(const String& data) const;

		void assignUUIDs(EntityData& node);
		bool isValidEntityTree(const ConfigNode& node) const;

		void toggleConsole();
		void setupConsoleCommands();
	};
}
