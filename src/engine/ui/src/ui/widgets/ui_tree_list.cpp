#include <utility>


#include "widgets/ui_tree_list.h"

#include <cassert>


#include "widgets/ui_image.h"
#include "widgets/ui_label.h"
using namespace Halley;


UITreeList::UITreeList(String id, UIStyle style)
	: UIList(std::move(id), std::move(style))
{
	setupEvents();
}

void UITreeList::addTreeItem(const String& id, const String& parentId, size_t childIndex, const LocalisedString& label, const String& labelStyleName, Sprite icon, bool forceLeaf)
{
	auto listItem = std::make_shared<UIListItem>(id, *this, style.getSubStyle("item"), int(getNumberOfItems()), style.getBorder("extraMouseBorder"));

	// Controls
	const auto treeControls = std::make_shared<UITreeListControls>(id, style.getSubStyle("controls"));
	listItem->add(treeControls, 0, {}, UISizerFillFlags::Fill);

	// Icon
	const auto root = std::make_shared<UIWidget>("root", Vector2f(), UISizer());
	std::shared_ptr<UIImage> iconWidget;
	if (icon.hasMaterial()) {
		iconWidget = std::make_shared<UIImage>(icon);
		root->add(iconWidget, 0, {}, UISizerAlignFlags::Centre);
	}

	// Label
	const auto& labelStyle = style.getSubStyle(labelStyleName);
	auto labelWidget = std::make_shared<UILabel>(id + "_label", labelStyle.getTextRenderer("normal"), label);
	if (labelStyle.hasTextRenderer("selected")) {
		labelWidget->setSelectable(labelStyle.getTextRenderer("normal"), labelStyle.getTextRenderer("selected"));
	}
	if (labelStyle.hasTextRenderer("disabled")) {
		labelWidget->setDisablable(labelStyle.getTextRenderer("normal"), labelStyle.getTextRenderer("disabled"));
	}
	root->add(labelWidget, 0, style.getBorder("labelBorder"), UISizerFillFlags::Fill);

	listItem->add(root, 1);
	listItem->setDraggableSubWidget(root.get());

	// Logical item
	auto treeItem = std::make_unique<UITreeListItem>(id, listItem, treeControls, labelWidget, iconWidget, forceLeaf);
	auto& parentItem = getItemOrRoot(parentId);
	parentItem.addChild(std::move(treeItem), childIndex);

	addItem(listItem, Vector4f(), UISizerAlignFlags::Left | UISizerFillFlags::FillVertical);
	needsRefresh = true;
}

void UITreeList::removeItem(const String& id)
{
	auto item = root.removeFromTree(id);
	if (item) {
		removeTree(*item);
	}
	needsRefresh = true;
}

void UITreeList::removeTree(const UITreeListItem& tree)
{
	getSizer().remove(*tree.getListItem());
	removeChild(*tree.getListItem());

	items.erase(std::remove_if(items.begin(), items.end(), [&] (const std::shared_ptr<UIListItem>& i)
	{
		return i->getId() == tree.getId();
	}), items.end());

	for (auto& subTree: tree.getChildren()) {
		removeTree(*subTree);
	}
}

void UITreeList::setLabel(const String& id, const LocalisedString& label, Sprite icon)
{
	auto item = root.tryFindId(id);
	if (item) {
		item->setLabel(label);
		item->setIcon(icon);
	}
}

void UITreeList::clear()
{
	UIList::clear();
	root = UITreeListItem();
	needsRefresh = true;
}

void UITreeList::update(Time t, bool moved)
{
	UIList::update(t, moved);
	if (needsRefresh) {
		root.updateTree(*this);
		needsRefresh = false;
	}
}

void UITreeList::draw(UIPainter& painter) const
{
	UIList::draw(painter);
	if (insertCursor.hasMaterial()) {
		painter.draw(insertCursor);
	}
}

void UITreeList::onItemDragging(UIListItem& item, int index, Vector2f pos)
{
	auto elem = root.tryFindId(item.getId());
	if (elem) {
		elem->setExpanded(false);
	}

	const auto res = root.findPosition(*this, pos + item.getRect().getSize() / 2);
	if (res) {
		const auto& resData = res.value();
		auto rect = resData.rect;
		if (rect.getHeight() < 1) {
			rect = Rect4f(rect.getTopLeft() - Vector2f(0, 1), rect.getTopRight() + Vector2f(0, 1));
		}
		
		insertCursor = style.getSubStyle("cursor").getSprite(resData.type == UITreeListItem::PositionType::OnTop ? "over" : "beforeAfter");
		insertCursor.setPos(rect.getTopLeft()).scaleTo(rect.getSize());
	}
}

void UITreeList::onItemDoneDragging(UIListItem& item, int index, Vector2f pos)
{
	auto res = root.findPosition(*this, pos + item.getRect().getSize() / 2);
	if (res) {
		const auto& resData = res.value();

		const String& itemId = item.getId();
		String newParentId;
		size_t newChildIndex;
		
		if (resData.type == UITreeListItem::PositionType::OnTop) {
			newParentId = resData.item->getId();
			newChildIndex = resData.item->getNumberOfChildren();
		} else if (resData.type == UITreeListItem::PositionType::End) {
			newParentId = "";
			newChildIndex = root.getNumberOfChildren();
		} else {
			newParentId = resData.item->getParentId();
			const auto parent = root.tryFindId(newParentId);
			const auto siblingIndex = parent->getChildIndex(resData.item->getId());
			newChildIndex = siblingIndex + (resData.type == UITreeListItem::PositionType::Before ? 0 : 1);
		}

		reparentItem(itemId, newParentId, int(newChildIndex));
	}
	insertCursor = Sprite();
}

UITreeListItem& UITreeList::getItemOrRoot(const String& id)
{
	const auto res = root.tryFindId(id);
	if (res) {
		return *res;
	}
	return root;
}

void UITreeList::setupEvents()
{
	setHandle(UIEventType::TreeCollapse, [=] (const UIEvent& event)
	{
		auto elem = root.tryFindId(event.getStringData());
		if (elem) {
			elem->setExpanded(false);
		}
		needsRefresh = true;
	});

	setHandle(UIEventType::TreeExpand, [=](const UIEvent& event)
	{
		auto elem = root.tryFindId(event.getStringData());
		if (elem) {
			elem->setExpanded(true);
		}
		needsRefresh = true;
	});
}

void UITreeList::reparentItem(const String& itemId, const String& newParentId, int newChildIndex)
{
	if (itemId == newParentId) {
		return;
	}
	
	const auto& curItem = *root.tryFindId(itemId);
	const String& oldParentId = curItem.getParentId();
	auto& oldParent = *root.tryFindId(oldParentId);
	const size_t oldChildIndex = int(oldParent.getChildIndex(itemId));

	if (oldParentId != newParentId || oldChildIndex != newChildIndex) {
		int realNewChildIndex = newChildIndex;
		if (oldParentId == newParentId) {
			oldParent.moveChild(oldChildIndex, newChildIndex);

			// This requires some explanation:
			// The index returned here assumes that the item is still present, so it's inflated by one if moving forwards in the same child
			// We'll subtract one before reporting as an event
			if (realNewChildIndex > oldChildIndex) {
				--realNewChildIndex;
			}
		} else {
			auto& newParent = *root.tryFindId(newParentId);
			newParent.addChild(oldParent.removeChild(itemId), newChildIndex);
		}
		sortItems();
		needsRefresh = true;

		sendEvent(UIEvent(UIEventType::TreeItemReparented, getId(), itemId, newParentId, realNewChildIndex));
	}
}

void UITreeList::sortItems()
{
	// Store previous curOption
	const auto oldOption = curOption >= 0 && curOption < gsl::narrow_cast<int>(items.size()) ? items[curOption]->getId() : "";
	
	// Update list representation
	items.clear();
	root.collectItems(items);
	reassignIds();

	// Restore curOption
	setSelectedOptionId(oldOption);

	// Update sizer
	getSizer().sortItems([&] (const UISizerEntry& a, const UISizerEntry& b)
	{
		const auto& itemA = std::dynamic_pointer_cast<UIListItem>(a.getPointer());
		const auto& itemB = std::dynamic_pointer_cast<UIListItem>(b.getPointer());

		return itemA->getAbsoluteIndex() < itemB->getAbsoluteIndex();
	});
}

void UITreeList::setSingleRoot(bool enabled)
{
	singleRoot = enabled;
}

bool UITreeList::isSingleRoot() const
{
	return singleRoot;
}

bool UITreeList::canDragListItem(const UIListItem& listItem)
{
	return isDragEnabled() && (!singleRoot || listItem.getAbsoluteIndex() != 0);
}

UITreeListControls::UITreeListControls(String id, UIStyle style)
	: UIWidget(std::move(id), Vector2f(), UISizer(UISizerType::Horizontal, 0))
	, style(std::move(style))
{
	setupUI();
}

float UITreeListControls::updateGuides(const std::vector<int>& itemsLeftPerDepth, bool hasChildren, bool expanded)
{
	auto getSprite = [&](size_t depth) -> Sprite
	{
		const bool leaf = depth == itemsLeftPerDepth.size();
		if (leaf) {
			return style.getSprite("leaf");
		} else {
			const bool deepest = depth == itemsLeftPerDepth.size() - 1;
			const auto left = itemsLeftPerDepth[depth];
			if (deepest) {
				if (left == 1) {
					return style.getSprite("guide_l");
				} else {
					return style.getSprite("guide_t");
				}
			} else {
				if (left == 1) {
					return Sprite().setSize(Vector2f(22, 22));
				} else {
					return style.getSprite("guide_i");
				}
			}
		}
	};

	const bool hadChildren = !!expandButton;

	if (waitingConstruction || itemsLeftPerDepth.size() != lastDepth || hasChildren != hadChildren) {
		clear();
		guides.clear();
		lastDepth = itemsLeftPerDepth.size();

		for (size_t i = 1; i < itemsLeftPerDepth.size() + (hasChildren ? 0 : 1); ++i) {
			guides.push_back(std::make_shared<UIImage>(getSprite(i)));
			add(guides.back(), 0, Vector4f(0, -1, 0, 0));
		}

		if (hasChildren) {
			expandButton = std::make_shared<UIButton>("expand", style.getSubStyle("expandButton"));
			collapseButton = std::make_shared<UIButton>("collapse", style.getSubStyle("collapseButton"));

			expandButton->setActive(!expanded);
			collapseButton->setActive(expanded);

			add(expandButton, 0, Vector4f(), UISizerAlignFlags::Centre);
			add(collapseButton, 0, Vector4f(), UISizerAlignFlags::Centre);
		} else if (hadChildren) {
			expandButton->destroy();
			expandButton.reset();
			collapseButton->destroy();
			collapseButton.reset();
		}

		waitingConstruction = false;
		totalIndent = getLayoutMinimumSize(false).x;
	} else {
		// Update guides
		for (size_t i = 1; i < itemsLeftPerDepth.size() + (hasChildren ? 0 : 1); ++i) {
			guides[i - 1]->setSprite(getSprite(i));
		}
	}

	return totalIndent;
}

void UITreeListControls::setExpanded(bool expanded)
{
	if (expandButton) {
		expandButton->setActive(!expanded);
	}
	if (collapseButton) {
		collapseButton->setActive(expanded);
	}
}

void UITreeListControls::setupUI()
{
	setHandle(UIEventType::ButtonClicked, "expand", [=] (const UIEvent& event)
	{
		sendEvent(UIEvent(UIEventType::TreeExpand, getId(), getId()));
	});
	setHandle(UIEventType::ButtonClicked, "collapse", [=](const UIEvent& event)
	{
		sendEvent(UIEvent(UIEventType::TreeCollapse, getId(), getId()));
	});
}

UITreeListItem::UITreeListItem() = default;

UITreeListItem::UITreeListItem(String id, std::shared_ptr<UIListItem> listItem, std::shared_ptr<UITreeListControls> treeControls, std::shared_ptr<UILabel> label, std::shared_ptr<UIImage> icon, bool forceLeaf)
	: id(std::move(id))
	, listItem(std::move(listItem))
	, label(std::move(label))
	, icon(std::move(icon))
	, treeControls(std::move(treeControls))
	, forceLeaf(forceLeaf)
{}

UITreeListItem* UITreeListItem::tryFindId(const String& id)
{
	if (id == this->id) {
		return this;
	}

	for (auto& c: children) {
		const auto res = c->tryFindId(id);
		if (res) {
			return res;
		}
	}

	return nullptr;
}

void UITreeListItem::addChild(std::unique_ptr<UITreeListItem> item, size_t pos)
{
	Expects(!forceLeaf);
	
	if (children.empty()) {
		expanded = true;
	}
	item->parentId = id;
	
	children.insert(children.begin() + std::min(children.size(), pos), std::move(item));
}

std::unique_ptr<UITreeListItem> UITreeListItem::removeChild(const String& id)
{
	Expects(!forceLeaf);
	
	const size_t n = children.size();
	for (size_t i = 0; i < n; ++i) {
		if (children[i]->id == id) {
			auto item = std::move(children[i]);
			children.erase(children.begin() + i);
			item->parentId = "";
			return item;
		}
	}
	throw Exception("No child with id \"" + id + "\"", HalleyExceptions::UI);
}

void UITreeListItem::moveChild(size_t startIndex, size_t targetIndex)
{
	Expects(!forceLeaf);
	
	// If moving forwards, subtract one to account for the fact that the currently occupied slot will be removed
	const size_t finalIndex = targetIndex > startIndex ? targetIndex - 1 : targetIndex;

	// Swap from start to end
	const int dir = finalIndex > startIndex ? 1 : -1;
	for (size_t i = startIndex; i != finalIndex; i += dir) {
		std::swap(children[i], children[i + dir]);
	}
}

void UITreeListItem::setLabel(const LocalisedString& text)
{
	if (label) {
		label->setText(text);
	}
}

void UITreeListItem::setIcon(Sprite sprite)
{
	if (icon) {
		icon->setSprite(sprite);
	}
}

void UITreeListItem::setExpanded(bool e)
{
	if (!children.empty()) {
		expanded = e;
		treeControls->setExpanded(e);
	}
}

std::unique_ptr<UITreeListItem> UITreeListItem::removeFromTree(const String& id)
{
	for (size_t i = 0; i < children.size(); ++i) {
		if (children[i]->id == id) {
			auto res = std::move(children[i]);
			children.erase(children.begin() + i);
			return res;
		}
	}

	for (auto& child: children) {
		auto result = child->removeFromTree(id);
		if (result) {
			return result;
		}
	}

	return {};
}

std::optional<UITreeListItem::FindPositionResult> UITreeListItem::findPosition(UITreeList& tree, Vector2f pos) const
{
	return doFindPosition(tree, pos, 0, true);
}

std::optional<UITreeListItem::FindPositionResult> UITreeListItem::doFindPosition(UITreeList& tree, Vector2f pos, int depth, bool lastBranch) const
{
	if (listItem) {
		const bool isLastItem = lastBranch && (!expanded || children.empty());
		const bool isSingleRootTree = tree.isSingleRoot();
		const bool isRootOfSingleRootTree = depth <= 1 && isSingleRootTree;
	
		const auto r = listItem->getRect();
		const auto b = listItem->getClickableInnerBorder();
		const float x0 = r.getLeft() + b.x;
		const float x1 = r.getRight() - b.z;
		const float y0 = r.getTop() + b.y;
		const float y1 = r.getBottom() - b.w + 1;
		const float h = y1 - y0;
		const float y = pos.y;
		
		if (y >= y0 && y < y1) {
			float threshold0, threshold1;
			if (forceLeaf) {
				threshold0 = y0 + h / 2;
				threshold1 = y0 + h / 2;
			} else if (isRootOfSingleRootTree) {
				threshold0 = y1;
				threshold1 = y0;
			} else {
				threshold0 = y0 + h / 4;
				threshold1 = y0 + 3 * h / 4;
			}
			
			if (y < threshold0 && !isRootOfSingleRootTree) {
				return FindPositionResult(PositionType::Before, this, Rect4f(x0, y0, x1 - x0, 0));
			} else if ((y > threshold1 && !isRootOfSingleRootTree) || forceLeaf) {
				return FindPositionResult(PositionType::After, this, Rect4f(x0, y1, x1 - x0, 0));
			} else {
				assert(!forceLeaf);
				return FindPositionResult(PositionType::OnTop, this, Rect4f(x0, y0, x1 - x0, y1 - y0));
			}
		} else if (y >= y1 && isLastItem && !isSingleRootTree) {
			return FindPositionResult(PositionType::End, nullptr, Rect4f(0, y1, 20, 0));
		}
	}

	if (expanded) {
		for (size_t i = 0; i < children.size(); ++i) {
			auto res = children[i]->doFindPosition(tree, pos, depth + 1, lastBranch && i + 1 == children.size());
			if (res) {
				return *res;
			}
		}
	}
	
	return {};
}

const String& UITreeListItem::getId() const
{
	return id;
}

const String& UITreeListItem::getParentId() const
{
	return parentId;
}

size_t UITreeListItem::getNumberOfChildren() const
{
	return children.size();
}

size_t UITreeListItem::getChildIndex(const String& id) const
{
	for (size_t i = 0; i < children.size(); ++i) {
		if (children[i]->id == id) {
			return i;
		}
	}
	return 0;
}

std::shared_ptr<UIListItem> UITreeListItem::getListItem() const
{
	return listItem;
}

const std::vector<std::unique_ptr<UITreeListItem>>& UITreeListItem::getChildren() const
{
	return children;
}

bool UITreeListItem::canHaveChildren() const
{
	return !forceLeaf;
}

void UITreeListItem::updateTree(UITreeList& treeList)
{
	std::vector<int> itemsLeftPerDepth;
	doUpdateTree(treeList, itemsLeftPerDepth, expanded);
}

void UITreeListItem::collectItems(std::vector<std::shared_ptr<UIListItem>>& items)
{
	if (listItem) {
		items.push_back(listItem);
	}

	for (auto& c: children) {
		c->collectItems(items);
	}
}

void UITreeListItem::doUpdateTree(UITreeList& treeList, std::vector<int>& itemsLeftPerDepth, bool treeExpanded)
{
	treeList.setItemActive(id, treeExpanded);

	if (listItem && treeControls && treeExpanded) {
		const float totalIndent = treeControls->updateGuides(itemsLeftPerDepth, !children.empty(), expanded);
		listItem->setClickableInnerBorder(Vector4f(totalIndent, 0, 0, 0));
	}
	
	itemsLeftPerDepth.push_back(int(children.size()));
	for (auto& c: children) {
		c->doUpdateTree(treeList, itemsLeftPerDepth, expanded && treeExpanded);
		itemsLeftPerDepth.back()--;
	}
	itemsLeftPerDepth.pop_back();
}
