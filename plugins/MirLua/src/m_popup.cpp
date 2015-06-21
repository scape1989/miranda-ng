#include "stdafx.h"

static POPUPDATA2* MakePopupData(lua_State *L)
{
	POPUPDATA2 *ppd = (POPUPDATA2*)mir_calloc(sizeof(POPUPDATA2));
	ppd->cbSize = sizeof(POPUPDATA2);

	lua_pushstring(L, "Flags");
	lua_gettable(L, -2);
	ppd->flags = lua_tointeger(L, -1);
	lua_pop(L, 1);

	if (!(ppd->flags & PU2_TCHAR))
		ppd->flags |= PU2_TCHAR;

	lua_pushstring(L, "Title");
	lua_gettable(L, -2);
	ppd->lptzTitle = mir_utf8decodeT((char*)lua_tostring(L, -1));
	lua_pop(L, 1);

	lua_pushstring(L, "Text");
	lua_gettable(L, -2);
	ppd->lptzText = mir_utf8decodeT((char*)luaL_checkstring(L, -1));
	lua_pop(L, 1);

	lua_pushstring(L, "hContact");
	lua_gettable(L, -2);
	ppd->lchContact = lua_tointeger(L, -1);
	lua_pop(L, 1);

	lua_pushstring(L, "hEvent");
	lua_gettable(L, -2);
	ppd->lchEvent = lua_touserdata(L, -1);
	lua_pop(L, 1);

	lua_pushstring(L, "Timestamp");
	lua_gettable(L, -2);
	ppd->dwTimestamp = lua_tonumber(L, -1);
	lua_pop(L, 1);

	lua_pushstring(L, "Timeout");
	lua_gettable(L, -2);
	ppd->iSeconds = lua_tointeger(L, -1);
	lua_pop(L, 1);

	return ppd;
}

static int lua_AddPoput(lua_State *L)
{
	if (lua_type(L, 1) != LUA_TTABLE)
	{
		lua_pushlightuserdata(L, 0);
		return 1;
	}

	mir_ptr<POPUPDATA2> ppd(MakePopupData(L));

	INT_PTR res = ::CallService(MS_POPUP_ADDPOPUP2, (WPARAM)ppd, 0);
	lua_pushinteger(L, res);

	return 1;
}

static luaL_Reg popupApi[] =
{
	{ "AddPopup", lua_AddPoput },

	{ NULL, NULL }
};

LUAMOD_API int luaopen_m_popup(lua_State *L)
{
	luaL_newlib(L, popupApi);

	return 1;
}