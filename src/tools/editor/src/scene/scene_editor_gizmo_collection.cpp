#include "scene_editor_gizmo_collection.h"
#include "gizmos/translate_gizmo.h"
#include "gizmos/selected_bounds_gizmo.h"
#include "gizmos/polygon_gizmo.h"
#include "gizmos/selection_box_gizmo.h"
#include "gizmos/vertex_gizmo.h"
#include "halley/core/graphics/camera.h"
using namespace Halley;

SceneEditorGizmoCollection::SceneEditorGizmoCollection(UIFactory& factory, Resources& resources)
	: factory(factory)
	, resources(resources)
{
	// TODO: read this elsewhere
	snapRules.grid = GridSnapMode::Pixel;
	snapRules.line = LineSnapMode::IsometricAxisAligned;
	
	selectedBoundsGizmo = std::make_unique<SelectedBoundsGizmo>(snapRules, resources);
	selectionBoxGizmo = std::make_unique<SelectionBoxGizmo>(snapRules, resources);
}

bool SceneEditorGizmoCollection::update(Time time, const Camera& camera, const SceneEditorInputState& inputState, SceneEditorOutputState& outputState)
{
	selectedBoundsGizmo->setCamera(camera);
	selectedBoundsGizmo->update(time, inputState);
	selectionBoxGizmo->setCamera(camera);
	selectionBoxGizmo->update(time, inputState);
	
	if (activeGizmo) {
		activeGizmo->setCamera(camera);
		activeGizmo->setOutputState(outputState);
		activeGizmo->update(time, inputState);

		return activeGizmo->isHighlighted();
	}
	return false;
}

void SceneEditorGizmoCollection::draw(Painter& painter)
{
	selectedBoundsGizmo->draw(painter);
	selectionBoxGizmo->draw(painter);
	
	if (activeGizmo) {
		activeGizmo->draw(painter);
	}
}

void SceneEditorGizmoCollection::setSelectedEntity(const std::optional<EntityRef>& entity, EntityData& data)
{
	selectedEntity = entity;
	entityData = &data;
	
	selectedBoundsGizmo->setSelectedEntity(entity, *entityData);
	
	if (activeGizmo) {
		activeGizmo->setSelectedEntity(entity, *entityData);
	}
}

std::shared_ptr<UIWidget> SceneEditorGizmoCollection::setTool(SceneEditorTool tool, const String& componentName, const String& fieldName, const ConfigNode& options)
{
	currentTool = tool;
	activeGizmo.reset();
	
	switch (tool) {
	case SceneEditorTool::Translate:
		activeGizmo = std::make_unique<TranslateGizmo>(snapRules);
		break;

	case SceneEditorTool::Polygon:
		activeGizmo = std::make_unique<PolygonGizmo>(snapRules, componentName, fieldName, options, factory);
		break;

	case SceneEditorTool::Vertex:
		activeGizmo = std::make_unique<VertexGizmo>(snapRules, componentName, fieldName);
		break;
	}

	if (activeGizmo) {
		activeGizmo->setSelectedEntity(selectedEntity, *entityData);
		return activeGizmo->makeUI();
	}

	return {};
}

void SceneEditorGizmoCollection::deselect()
{
	if (activeGizmo) {
		activeGizmo->deselect();
	}
}
