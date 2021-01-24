#pragma once
#include "../ui_widget.h"
#include "halley/core/graphics/sprite/sprite.h"

namespace Halley {
	class UIImage : public UIWidget {
	public:
		explicit UIImage(Sprite sprite, std::optional<UISizer> sizer = {}, Vector4f innerBorder = {});
		explicit UIImage(String id, Sprite sprite, std::optional<UISizer> sizer = {}, Vector4f innerBorder = {});

		void draw(UIPainter& painter) const override;
		void update(Time t, bool moved) override;

		void setSprite(Sprite sprite);
		Sprite& getSprite();
		const Sprite& getSprite() const;

		void setLayerAdjustment(int adjustment);
		void setWorldClip(std::optional<Rect4f> worldClip);
		void setHoverable(Colour4f normalColour, Colour4f selColour);
		void setHoverable(Sprite normalSprite, Sprite selectedSprite);
		void setSelectable(Colour4f normalColour, Colour4f selColour);
		void setSelectable(Sprite normalSprite, Sprite selectedSprite);
		void setDisablable(Colour4f normalColour, Colour4f disabledColour);

	private:
		Sprite sprite;
		Vector2f topLeftBorder;
		Vector2f bottomRightBorder;
		int layerAdjustment = 0;
		bool dirty = true;
		std::optional<Rect4f> worldClip;
	};
}
