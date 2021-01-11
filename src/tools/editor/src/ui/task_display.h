#pragma once

#include "halley/tools/tasks/editor_task.h"
#include "prec.h"

namespace Halley
{
	class TaskBar;
	class EditorTaskSet;

	class TaskDisplay : public UIWidget
	{
	public:
		TaskDisplay(UIFactory& factory, std::shared_ptr<EditorTaskAnchor> task, TaskBar& taskBar);

		void update(Time t, bool moved) override;
		bool updateTask(Time time, float targetDisplaySlot);
		
		const std::shared_ptr<EditorTaskAnchor>& getTask() const { return task; }
		void setTask(std::shared_ptr<EditorTaskAnchor> task);

		void onMakeUI() override;
		float getDisplaySlot() const { return displaySlot; }

		void onMouseOver(Vector2f mousePos) override;
		bool canInteractWithMouse() const override;
	
	private:
		UIFactory& factory;
		std::shared_ptr<EditorTaskAnchor> task;
		TaskBar& taskBar;

		EditorTaskStatus lastStatus = EditorTaskStatus::WaitingToStart;

		float progressDisplay = 0;
		float completeTime = 0;
		float displaySlot = -1;

		float opacity = 0;
		Time elapsedTime = 0;

		std::shared_ptr<UILabel> name;
		std::shared_ptr<UILabel> desc;
		std::shared_ptr<UIImage> bg;
		std::shared_ptr<UIImage> bgFill;
		std::shared_ptr<UIImage> icon;
		std::shared_ptr<UIImage> iconBg;

		Colour nameCol;
		Colour descCol;
	};
}
