#include "p2Log.h"
#include "UI_Image.h"
#include "j1Render.h"
#include "j1Window.h"
#include "j1Gui.h"
#include "UiItem.h"
#include "j1App.h"

UI_Image::UI_Image(SDL_Rect rect, UI_Item* parent)
{
	sprite_rect = rect;
	this->parent = parent;
}


bool UI_Image::PostUpdate()
{
	App->render->Blit(App->gui->GetAtlas(), position.x, position.y, &sprite_rect);
	return true;
}