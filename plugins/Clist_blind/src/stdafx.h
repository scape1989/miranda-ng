/*

Miranda NG: the free IM client for Microsoft* Windows*

Copyright (�) 2012-17 Miranda NG project (https://miranda-ng.org)
Copyright (c) 2000-05 Miranda ICQ/IM project,
all portions of this codebase are copyrighted to the people
listed in contributors.txt.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include <windows.h>
#include <vssym32.h>
#include <Uxtheme.h>

#include <newpluginapi.h>
#include <m_clist.h>
#include <m_database.h>
#include <m_langpack.h>
#include <m_options.h>
#include <m_protosvc.h>
#include <win2k.h>

#include "resource.h"
#include "version.h"

struct ClcContact : public ClcContactBase {};

struct ClcData : public ClcDataBase
{
	HWND hwnd_list;
	BOOL need_rebuild;
};

// shared vars
extern HINSTANCE g_hInst;
extern CLIST_INTERFACE coreCli;
extern int g_bSortByStatus, g_bSortByProto;


int CompareContacts(const ClcContact *contact1, const ClcContact *contact2);

/* most free()'s are invalid when the code is executed from a dll, so this changes
 all the bad free()'s to good ones, however it's still incorrect code. The reasons for not
 changing them include:

  * db_free has a CallService() lookup
  * free() is executed in some large loops to do with clist creation of group data
  * easy search and replace

*/
