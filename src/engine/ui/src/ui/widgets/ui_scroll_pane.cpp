#include "widgets/ui_scroll_pane.h"
#include "ui_style.h"
#include "halley/support/logger.h"

using namespace Halley;

UIScrollPane::UIScrollPane(String id, Vector2f clipSize, UISizer&& sizer, bool scrollHorizontal, bool scrollVertical)
	: UIWidget(std::move(id), Vector2f(), std::move(sizer))
	, clipSize(clipSize)
	, scrollSpeed(50.0f)
	, scrollHorizontal(scrollHorizontal)
	, scrollVertical(scrollVertical)
{
	setHandle(UIEventType::MouseWheel, [this] (const UIEvent& event)
	{
		onMouseWheel(event);
	});

	setHandle(UIEventType::MakeAreaVisible, [this] (const UIEvent& event)
	{
		scrollToShow(event.getRectData() + getBasePosition(event.getSourceId()), false);
	});

	setHandle(UIEventType::MakeAreaVisibleCentered, [this] (const UIEvent& event)
	{
		scrollToShow(event.getRectData() + getBasePosition(event.getSourceId()), true);
	});
}

UIScrollPane::UIScrollPane(Vector2f clipSize, UISizer&& sizer, bool scrollHorizontal, bool scrollVertical)
: UIScrollPane("", clipSize, std::move(sizer), scrollHorizontal, scrollVertical) {

}

Vector2f UIScrollPane::getScrollPosition() const
{
	return scrollPos;
}

Vector2f UIScrollPane::getRelativeScrollPosition() const
{
	return scrollPos / std::max(contentsSize, Vector2f(1.0f, 1.0f));
}

Vector2f UIScrollPane::getRelativeScrollEndPosition() const
{
	return (scrollPos + clipSize) / std::max(contentsSize, Vector2f(1.0f, 1.0f));
}

void UIScrollPane::scrollTo(Vector2f position)
{	
	if (scrollHorizontal) {
		scrollPos.x = clamp2(position.x, 0.0f, contentsSize.x - getSize().x);
	}
	
	if (scrollVertical) {
		scrollPos.y = clamp2(position.y, 0.0f, contentsSize.y - getSize().y);
	}
}

void UIScrollPane::scrollBy(Vector2f delta)
{
	scrollTo(scrollPos + delta);
}

void UIScrollPane::setScrollSpeed(float speed)
{
	scrollSpeed = speed;
}

void UIScrollPane::update(Time t, bool moved)
{
	refresh();
}

bool UIScrollPane::canScroll(UIScrollDirection direction) const
{
	auto contentsSize = UIWidget::getLayoutMinimumSize(false);
	if (direction == UIScrollDirection::Horizontal) {
		return scrollHorizontal && getSize().x < contentsSize.x;
	} else {
		return scrollVertical && getSize().y < contentsSize.y;
	}
}

float UIScrollPane::getCoverageSize(UIScrollDirection direction) const
{
	auto contentsSize = UIWidget::getLayoutMinimumSize(false);
	if (direction == UIScrollDirection::Horizontal) {
		return getSize().x / contentsSize.x;
	} else {
		return getSize().y / contentsSize.y;
	}
}

void UIScrollPane::setScrollWheelEnabled(bool enabled)
{
	scrollWheelEnabled = enabled;
}

bool UIScrollPane::isScrollWheelEnabled() const
{
	return scrollWheelEnabled;
}

void UIScrollPane::refresh(bool force)
{
	if (!scrollHorizontal) {
		clipSize.x = getSize().x;
		scrollPos.x = 0;
	}
	if (!scrollVertical) {
		clipSize.y = getSize().y;
		scrollPos.y = 0;
	}
	contentsSize = UIWidget::getLayoutMinimumSize(false);

	setMouseClip(getRect(), force);
	scrollTo(getScrollPosition());
}

void UIScrollPane::drawChildren(UIPainter& painter) const
{
	auto p = painter.withClip(getRect());
	UIWidget::drawChildren(p);
}

Vector2f UIScrollPane::getLayoutMinimumSize(bool force) const
{
	auto size = UIWidget::getLayoutMinimumSize(false);
	if (scrollHorizontal) {
		size.x = std::min(size.x, clipSize.x);
	}
	if (scrollVertical) {
		size.y = std::min(size.y, clipSize.y);
	}
	return size;
}

bool UIScrollPane::canInteractWithMouse() const
{
	return true;
}

void UIScrollPane::onLayout()
{
	refresh();
}

void UIScrollPane::onMouseWheel(const UIEvent& event)
{
	if (scrollWheelEnabled) {
		scrollBy(Vector2f(0.0f, -scrollSpeed * float(event.getIntData())));
	}
}

Vector2f UIScrollPane::getBasePosition(const String& widgetId)
{
	auto widget = tryGetWidget(widgetId);
	if (widget) {
		return widget->getPosition() + scrollPos - getPosition();
	} else {
		return Vector2f();
	}
}

Vector2f UIScrollPane::getLayoutOriginPosition() const
{
	return getPosition() - scrollPos.floor();
}

void UIScrollPane::scrollToShow(Rect4f rect, bool center)
{
	auto size = getSize();

	float maxX = rect.getLeft();
	float minX = rect.getRight() - size.x;
	float maxY = rect.getTop();
	float minY = rect.getBottom() - size.y;
	auto target = center ? (rect.getCenter() - 0.5f * size) : scrollPos;
	scrollTo(Vector2f(clamp(target.x, minX, maxX), clamp(target.y, minY, maxY)));
}

float UIScrollPane::getScrollSpeed() const
{
	return scrollSpeed;
}

void UIScrollPane::setRelativeScroll(float position, UIScrollDirection direction)
{
	int axis = direction == UIScrollDirection::Horizontal ? 0 : 1;
	auto target = scrollPos;
	target[axis] = position * contentsSize[axis];
	scrollTo(target);
}

std::optional<float> UIScrollPane::getMaxChildWidth() const
{
	if (scrollHorizontal) {
		return std::optional<float>();
	} else {
		return getSize().x;
	}
}

bool UIScrollPane::ignoreClip() const
{
	return true;
}

void UIScrollPane::onChildrenAdded()
{
	refresh(true);
}

void UIScrollPane::onChildrenRemoved()
{
	refresh(true);
}
