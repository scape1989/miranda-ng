/*

Minecraft Dynmap plugin for Miranda Instant Messenger
_____________________________________________

Copyright � 2015-17 Robert P�sel

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "stdafx.h"

MinecraftDynmapProto::MinecraftDynmapProto(const char* proto_name, const wchar_t* username) :
	PROTO<MinecraftDynmapProto>(proto_name, username), m_interval(0), hConnection(0), hEventsConnection(0),
	m_updateRate(5000), m_nick("")
{
	this->signon_lock_ = CreateMutex(NULL, FALSE, NULL);
	this->send_message_lock_ = CreateMutex(NULL, FALSE, NULL);
	this->connection_lock_ = CreateMutex(NULL, FALSE, NULL);
	this->events_loop_lock_ = CreateMutex(NULL, FALSE, NULL);
	this->events_loop_event_ = CreateEvent(NULL, FALSE, FALSE, NULL);

	// Group chats
	CreateProtoService(PS_JOINCHAT, &MinecraftDynmapProto::OnJoinChat);
	CreateProtoService(PS_LEAVECHAT, &MinecraftDynmapProto::OnLeaveChat);

	CreateProtoService(PS_CREATEACCMGRUI, &MinecraftDynmapProto::SvcCreateAccMgrUI);

	// HookProtoEvent(ME_OPT_INITIALISE, &MinecraftDynmapProto::OnOptionsInit);
	HookProtoEvent(ME_GC_EVENT, &MinecraftDynmapProto::OnChatEvent);

	// Create standard network connection
	wchar_t descr[512];
	mir_snwprintf(descr, TranslateT("%s server connection"), m_tszUserName);

	NETLIBUSER nlu = {};
	nlu.flags = NUF_INCOMING | NUF_OUTGOING | NUF_HTTPCONNS | NUF_UNICODE;
	nlu.szSettingsModule = m_szModuleName;
	nlu.szDescriptiveName.w = descr;
	m_hNetlibUser = Netlib_RegisterUser(&nlu);
	if (m_hNetlibUser == NULL) {
		wchar_t error[200];
		mir_snwprintf(error, TranslateT("Unable to initialize Netlib for %s."), m_tszUserName);
		MessageBox(NULL, error, L"Miranda NG", MB_OK | MB_ICONERROR);
	}

	// Client instantiation
	this->error_count_ = 0;
	this->chatHandle_ = NULL;
}

MinecraftDynmapProto::~MinecraftDynmapProto()
{
	Netlib_CloseHandle(m_hNetlibUser);

	WaitForSingleObject(this->signon_lock_, IGNORE);
	WaitForSingleObject(this->send_message_lock_, IGNORE);	
	WaitForSingleObject(this->connection_lock_, IGNORE);
	WaitForSingleObject(this->events_loop_lock_, IGNORE);

	CloseHandle(this->signon_lock_);
	CloseHandle(this->send_message_lock_);	
	CloseHandle(this->connection_lock_);
	CloseHandle(this->events_loop_lock_);
	CloseHandle(this->events_loop_event_);
}

//////////////////////////////////////////////////////////////////////////////

DWORD_PTR MinecraftDynmapProto::GetCaps(int type, MCONTACT)
{
	switch(type) {
	case PFLAGNUM_1:
		return PF1_CHAT;
	case PFLAGNUM_2:
		return PF2_ONLINE;
	case PFLAG_MAXLENOFMESSAGE:
		return MINECRAFTDYNMAP_MESSAGE_LIMIT;
	case PFLAG_UNIQUEIDTEXT:
		return (DWORD_PTR) Translate("Visible name");
	case PFLAG_UNIQUEIDSETTING:
		return (DWORD_PTR) "Nick";
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////////

int MinecraftDynmapProto::SetStatus(int new_status)
{
	// Routing not supported statuses
	switch (new_status) {
	case ID_STATUS_OFFLINE:
	case ID_STATUS_CONNECTING:
		new_status = ID_STATUS_OFFLINE;
		break;
	default:
		new_status = ID_STATUS_ONLINE;
		break;
	}

	m_iDesiredStatus = new_status;

	if (new_status == m_iStatus) {
		return 0;
	}

	if (m_iStatus == ID_STATUS_CONNECTING && new_status != ID_STATUS_OFFLINE) {
		return 0;
	}

	if (new_status == ID_STATUS_OFFLINE) {
		ForkThread(&MinecraftDynmapProto::SignOffWorker, this);
	} else {
		ForkThread(&MinecraftDynmapProto::SignOnWorker, this);
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////////

int MinecraftDynmapProto::OnEvent(PROTOEVENTTYPE event,WPARAM wParam,LPARAM lParam)
{
	switch(event) {
	case EV_PROTO_ONLOAD:
		return OnModulesLoaded(wParam, lParam);

	case EV_PROTO_ONEXIT:
		return OnPreShutdown  (wParam, lParam);

	// case EV_PROTO_ONOPTIONS:
		// return OnOptionsInit  (wParam, lParam);

	case EV_PROTO_ONCONTACTDELETED:
		return OnContactDeleted(wParam, lParam);
	}

	return 1;
}

//////////////////////////////////////////////////////////////////////////////
// EVENTS

INT_PTR MinecraftDynmapProto::SvcCreateAccMgrUI(WPARAM, LPARAM lParam)
{
	return (INT_PTR)CreateDialogParam(g_hInstance,MAKEINTRESOURCE(IDD_MinecraftDynmapACCOUNT), (HWND)lParam, MinecraftDynmapAccountProc, (LPARAM)this);
}

int MinecraftDynmapProto::OnModulesLoaded(WPARAM, LPARAM)
{
	// Register group chat
	GCREGISTER gcr = {};
	gcr.pszModule = m_szModuleName;
	gcr.ptszDispName = m_tszUserName;
	gcr.iMaxText = MINECRAFTDYNMAP_MESSAGE_LIMIT;
	Chat_Register(&gcr);
	return 0;
}

/*int MinecraftDynmapProto::OnOptionsInit(WPARAM wParam, LPARAM)
{
	OPTIONSDIALOGPAGE odp = { 0 };
	odp.hInstance   = g_hInstance;
	odp.szTitle.w   = m_tszUserName;
	odp.dwInitParam = LPARAM(this);
	odp.flags       = ODPF_BOLDGROUPS | ODPF_UNICODE | ODPF_DONTTRANSLATE;

	odp.position    = 271828;
	odp.szGroup.w   = LPGENW("Network");
	odp.szTab.w     = LPGENW("Account");
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPTIONS);
	odp.pfnDlgProc  = MinecraftDynmapOptionsProc;
	Options_AddPage(wParam, &odp);
	return 0;
}*/

int MinecraftDynmapProto::OnPreShutdown(WPARAM, LPARAM)
{
	SetStatus(ID_STATUS_OFFLINE);
	return 0;
}

int MinecraftDynmapProto::OnContactDeleted(WPARAM, LPARAM)
{
	OnLeaveChat(NULL, NULL);
	return 0;
}

//////////////////////////////////////////////////////////////////////////////
// HELPERS

bool MinecraftDynmapProto::handleEntry(const std::string &method)
{
	debugLogA("   >> Entering %s()", method.c_str());
	return true;
}

bool MinecraftDynmapProto::handleSuccess(const std::string &method)
{
	debugLogA("   << Quitting %s()", method.c_str());
	reset_error();
	return true;
}

bool MinecraftDynmapProto::handleError(const std::string &method, const std::string &error, bool force_disconnect)
{	
	increment_error();
	debugLogA("!!!!! Quitting %s() with error: %s", method.c_str(), !error.empty() ? error.c_str() : "Something went wrong");

	if (!force_disconnect && error_count_ <= (UINT)db_get_b(NULL, m_szModuleName, MINECRAFTDYNMAP_KEY_TIMEOUTS_LIMIT, MINECRAFTDYNMAP_TIMEOUTS_LIMIT)) {
		return true;
	}

	reset_error();
	SetStatus(ID_STATUS_OFFLINE);
	return false;
}
