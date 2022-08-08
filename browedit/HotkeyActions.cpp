#include <browedit/BrowEdit.h>
#include <browedit/HotkeyRegistry.h>
#include <browedit/Map.h>
#include <browedit/util/ResourceManager.h>
#include <browedit/gl/Texture.h>
#include <browedit/components/Rsw.h>
#include <browedit/components/Rsm.h>
#include <browedit/components/RsmRenderer.h>
#include <browedit/Lightmapper.h>
#include <browedit/Node.h>
#include <browedit/util/Util.h>
#include <GLFW/glfw3.h>

void BrowEdit::registerActions()
{
	auto hasActiveMapView = [this]() {return activeMapView != nullptr; };
	auto hasActiveMapViewObjectMode = [this]() { return activeMapView != nullptr && editMode == EditMode::Object; };
	auto hasActiveMapViewTextureMode = [this]() { return activeMapView != nullptr && editMode == EditMode::Texture; };

	HotkeyRegistry::registerAction(HotkeyAction::Global_HotkeyPopup,	[this]() { 
		windowData.showHotkeyPopup = true;
		std::cout << "Opening hotkey popup" << std::endl;
	});

	HotkeyRegistry::registerAction(HotkeyAction::Global_New, 			[this]() { windowData.showNewMapPopup = true; });
	HotkeyRegistry::registerAction(HotkeyAction::Global_Save,			[this]() { saveMap(activeMapView->map); }, hasActiveMapView);
	HotkeyRegistry::registerAction(HotkeyAction::Global_Load,			[this]() { showOpenWindow(); });
	HotkeyRegistry::registerAction(HotkeyAction::Global_Exit,			[this]() { glfwSetWindowShouldClose(window, 1); });

	HotkeyRegistry::registerAction(HotkeyAction::Global_Undo,			[this]() { activeMapView->map->undo(this); }, hasActiveMapView);
	HotkeyRegistry::registerAction(HotkeyAction::Global_Redo,			[this]() { activeMapView->map->redo(this); }, hasActiveMapView);

	HotkeyRegistry::registerAction(HotkeyAction::Global_Settings,		[this]() { windowData.configVisible = true; });
	HotkeyRegistry::registerAction(HotkeyAction::Global_ReloadTextures, [this]() { for (auto t : util::ResourceManager<gl::Texture>::getAll()) { t->reload(); } });
	HotkeyRegistry::registerAction(HotkeyAction::Global_ReloadModels,	[this]() { 
		for (auto rsm : util::ResourceManager<Rsm>::getAll())
			rsm->reload();
		for (auto m : maps)
			m->rootNode->traverse([](Node* n) {
			auto rsmRenderer = n->getComponent<RsmRenderer>();
			if (rsmRenderer)
				rsmRenderer->begin();
				});
		});
	HotkeyRegistry::registerAction(HotkeyAction::Global_Copy,					[this]() { if (editMode == EditMode::Object) { activeMapView->map->copySelection(); } }, hasActiveMapView);
	HotkeyRegistry::registerAction(HotkeyAction::Global_Paste,					[this]() { if (editMode == EditMode::Object) { activeMapView->map->pasteSelection(this); } }, hasActiveMapView);
	HotkeyRegistry::registerAction(HotkeyAction::Global_PasteChangeHeight,		[this]() { if (editMode == EditMode::Object) { newNodeHeight = !newNodeHeight; } }, hasActiveMapView);

	HotkeyRegistry::registerAction(HotkeyAction::Global_CalculateQuadtree,		[this]() { activeMapView->map->recalculateQuadTree(this); }, hasActiveMapView);
	HotkeyRegistry::registerAction(HotkeyAction::Global_CalculateLightmaps,		[this]() {
		lightmapper = new Lightmapper(activeMapView->map, this);
		windowData.openLightmapSettings = true; 
	}, [this]() { return activeMapView && !lightmapper;});
	


	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_FlipHorizontal,		[this]() { activeMapView->map->flipSelection(0, this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_FlipVertical,		[this]() { activeMapView->map->flipSelection(2, this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_Delete,				[this]() { activeMapView->map->deleteSelection(this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_FocusOnSelection,	[this]() { activeMapView->focusSelection(); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_HighlightInObjectPicker,	[this]() { 
		if (activeMapView->map->selectedNodes.size() < 1)
			return;
		auto selected = activeMapView->map->selectedNodes[0]->getComponent<RswModel>();
		if (!selected)
			return;
		std::filesystem::path path(util::utf8_to_iso_8859_1(selected->fileName));
		std::string dir = path.parent_path().string();
		windowData.objectWindowSelectedTreeNode = util::FileIO::directoryNode("data\\model\\" + dir);
		windowData.objectWindowScrollToModel = "data\\model\\" + util::utf8_to_iso_8859_1(selected->fileName);


	}, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_InvertSelection,	[this]() { activeMapView->map->selectInvert(this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_SelectAll,			[this]() { activeMapView->map->selectAll(this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_SelectAllModels,	[this]() { activeMapView->map->selectAll<RswModel>(this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_SelectAllEffects,	[this]() { activeMapView->map->selectAll<RswEffect>(this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_SelectAllSounds,	[this]() { activeMapView->map->selectAll<RswSound>(this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_SelectAllLights,	[this]() { activeMapView->map->selectAll<RswLight>(this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_Move,				[this]() { activeMapView->gadget.mode = Gadget::Mode::Translate; }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_Rotate,				[this]() { activeMapView->gadget.mode = Gadget::Mode::Rotate; }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_Scale,				[this]() { activeMapView->gadget.mode = Gadget::Mode::Scale; }, hasActiveMapViewObjectMode);

	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_InvertScaleX,		[this]() { activeMapView->map->invertScale(0, this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_InvertScaleY,		[this]() { activeMapView->map->invertScale(1, this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_InvertScaleZ,		[this]() { activeMapView->map->invertScale(2, this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_CreatePrefab,		[this]() { windowData.openPrefabPopup = true; }, hasActiveMapViewObjectMode);

	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_NudgeXNeg,	[this]() { activeMapView->map->nudgeSelection(0, -1, this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_NudgeXPos,	[this]() { activeMapView->map->nudgeSelection(0, 1, this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_NudgeYNeg,	[this]() { activeMapView->map->nudgeSelection(1, -1, this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_NudgeYPos,	[this]() { activeMapView->map->nudgeSelection(1, 1, this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_NudgeZNeg,	[this]() { activeMapView->map->nudgeSelection(2, -1, this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_NudgeZPos,	[this]() { activeMapView->map->nudgeSelection(2, 1, this); }, hasActiveMapViewObjectMode);

	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_RotXNeg,	[this]() { activeMapView->map->rotateSelection(0, -1, this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_RotXPos,	[this]() { activeMapView->map->rotateSelection(0, 1, this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_RotYNeg,	[this]() { activeMapView->map->rotateSelection(1, -1, this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_RotYPos,	[this]() { activeMapView->map->rotateSelection(1, 1, this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_RotZNeg,	[this]() { activeMapView->map->rotateSelection(2, -1, this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_RotZPos,	[this]() { activeMapView->map->rotateSelection(2, 1, this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_ToggleObjectWindow, [this]() { windowData.objectWindowVisible = !windowData.objectWindowVisible; }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::TextureEdit_ToggleTextureWindow, [this]() { windowData.textureManageWindowVisible = !windowData.textureManageWindowVisible; }, hasActiveMapViewTextureMode);

	HotkeyRegistry::registerAction(HotkeyAction::EditMode_Height,		[this]() { editMode = EditMode::Height; });
	HotkeyRegistry::registerAction(HotkeyAction::EditMode_Texture,		[this]() { editMode = EditMode::Texture; });
	HotkeyRegistry::registerAction(HotkeyAction::EditMode_Object,		[this]() { editMode = EditMode::Object; });
	HotkeyRegistry::registerAction(HotkeyAction::EditMode_Wall,			[this]() { editMode = EditMode::Wall; });
	HotkeyRegistry::registerAction(HotkeyAction::EditMode_Gat,			[this]() { editMode = EditMode::Gat; });
	
	HotkeyRegistry::registerAction(HotkeyAction::View_ShadowMap,		[this]() { activeMapView->viewLightmapShadow = !activeMapView->viewLightmapShadow; }, hasActiveMapView);
	HotkeyRegistry::registerAction(HotkeyAction::View_ColorMap,			[this]() { activeMapView->viewLightmapColor = !activeMapView->viewLightmapColor; }, hasActiveMapView);
	HotkeyRegistry::registerAction(HotkeyAction::View_TileColors,		[this]() { activeMapView->viewColors = !activeMapView->viewColors; }, hasActiveMapView);
	HotkeyRegistry::registerAction(HotkeyAction::View_Lighting,			[this]() { activeMapView->viewLighting = !activeMapView->viewLighting; }, hasActiveMapView);
	HotkeyRegistry::registerAction(HotkeyAction::View_Textures,			[this]() { activeMapView->viewTextures = !activeMapView->viewTextures; }, hasActiveMapView);
	HotkeyRegistry::registerAction(HotkeyAction::View_SmoothColormap,	[this]() { activeMapView->smoothColors = !activeMapView->smoothColors; }, hasActiveMapView);
	HotkeyRegistry::registerAction(HotkeyAction::View_EmptyTiles,		[this]() { activeMapView->viewEmptyTiles = !activeMapView->viewEmptyTiles; }, hasActiveMapView);
	HotkeyRegistry::registerAction(HotkeyAction::View_GatTiles,			[this]() { if (editMode == BrowEdit::EditMode::Gat) { activeMapView->viewGatGat = !activeMapView->viewGatGat; } else { activeMapView->viewGat = !activeMapView->viewGat; } }, hasActiveMapView);

}

bool BrowEdit::hotkeyMenuItem(const std::string& title, HotkeyAction action)
{
	auto hotkey = HotkeyRegistry::getHotkey(action);
	bool clicked = ImGui::MenuItem(title.c_str(), hotkey.hotkey.toString().c_str());

	if (clicked && hotkey.callback)
		hotkey.callback();
	return clicked;
}
bool BrowEdit::hotkeyButton(const std::string& title, HotkeyAction action)
{
	auto hotkey = HotkeyRegistry::getHotkey(action);
	bool clicked = ImGui::Button(title.c_str());
	if (ImGui::IsItemHovered() && hotkey.hotkey.keyCode != 0)
		ImGui::SetTooltip(hotkey.hotkey.toString().c_str());

	if (clicked && hotkey.callback)
		hotkey.callback();
	return clicked;
}