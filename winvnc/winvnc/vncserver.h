//  Copyright (C) 2002 UltraVNC Team Members. All Rights Reserved.
//  Copyright (C) 2000-2002 Const Kaplinsky. All Rights Reserved.
//  Copyright (C) 2002 RealVNC Ltd. All Rights Reserved.
//  Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//
//  This file is part of the VNC system.
//
//  The VNC system is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//
// If the source code for the VNC system is not available from the place 
// whence you received this file, check http://www.uk.research.att.com/vnc or contact
// the authors on vnc@uk.research.att.com for information on obtaining it.


// vncServer.h

// vncServer class handles the following functions:
// - Allowing clients to be dynamically added and removed
// - Propagating updates from the local vncDesktop object
//   to all the connected clients
// - Propagating mouse movements and keyboard events from
//   clients to the local vncDesktop
// It also creates the vncSockConnect
// servers, which respectively allow connections via sockets
// and via the ORB interface
extern bool			fShutdownOrdered;
class vncServer;
class VirtualDisplay;

#if (!defined(_WINVNC_VNCSERVER))
#define _WINVNC_VNCSERVER

// Custom
#include "vncsockconnect.h"
#include "vnchttpconnect.h"
#include "vncclient.h"
#include "rfbRegion.h"
#include "vncpasswd.h"

// Includes
#include "stdhdrs.h"
#include <omnithread.h>
#include <list>

// adzm - 2010-07 - Extended clipboard
#include "common/Clipboard.h"

typedef BOOL (WINAPI *WTSREGISTERSESSIONNOTIFICATION)(HWND, DWORD);
typedef BOOL (WINAPI *WTSUNREGISTERSESSIONNOTIFICATION)(HWND);
#define WM_WTSSESSION_CHANGE            0x02B1
#define WTS_CONSOLE_CONNECT                0x1
#define WTS_CONSOLE_DISCONNECT             0x2
#define WTS_REMOTE_CONNECT                 0x3
#define WTS_REMOTE_DISCONNECT              0x4
#define WTS_SESSION_LOGON                  0x5
#define WTS_SESSION_LOGOFF                 0x6
#define WTS_SESSION_LOCK                   0x7
#define WTS_SESSION_UNLOCK                 0x8
#define WTS_SESSION_REMOTE_CONTROL         0x9

// Define a datatype to handle lists of windows we wish to notify
typedef std::list<HWND> vncNotifyList;

// Some important constants;
const int MAX_CLIENTS = 128;

// The vncServer class itself

class vncServer
{
public:

	friend class vncClientThread;
	HANDLE m_impersonationtoken;

	// Constructor/destructor
	vncServer();
	~vncServer();

	// Client handling functions
	virtual vncClientId AddClient(VSocket *socket, BOOL auth, BOOL shared, BOOL outgoing);
	virtual vncClientId AddClient(VSocket *socket, BOOL auth, BOOL shared, rfbProtocolVersionMsg *protocolMsg, BOOL outgoing);
	virtual vncClientId AddClient(VSocket *socket,
		BOOL auth, BOOL shared, int capability,rfbProtocolVersionMsg *protocolMsg, BOOL outgoing);
	
	// adzm 2009-07-05
	// adzm 2009-08-02
	virtual vncClientId AddClient(VSocket *socket,
		BOOL auth, BOOL shared, int capability,rfbProtocolVersionMsg *protocolMsg, VString szRepeaterID, VString szHost, VCard port, BOOL outgoing);

	virtual BOOL Authenticated(vncClientId client);
	virtual void KillClient(vncClientId client);
	virtual void KillClient(LPSTR szClientName); // sf@2002
	virtual void TextChatClient(LPSTR szClientName); // sf@2002
	bool IsUltraVNCViewer();
	bool AreThereMultipleViewers();
	bool singleExtendRequested();

	virtual UINT AuthClientCount();
	virtual UINT UnauthClientCount();

	virtual void KillAuthClients();
	virtual void ListAuthClients(HWND hListBox);
	virtual void WaitUntilAuthEmpty();

	virtual void KillUnauthClients();
	virtual void ListUnauthClients(HWND hListBox); // adzm 2009-07-05
	virtual void WaitUntilUnauthEmpty();

	// Are any clients ready to send updates?
	virtual BOOL UpdateWanted();

	// Has at least one client had a remote event?
	virtual BOOL RemoteEventReceived();

	// Client info retrieval/setup
	virtual vncClient* GetClient(vncClientId clientid);
	virtual vncClientList ClientList();

	virtual void SetCapability(vncClientId client, int capability);
	virtual void SetKeyboardEnabled(vncClientId client, BOOL enabled);
	virtual void SetPointerEnabled(vncClientId client, BOOL enabled);

	virtual int GetCapability(vncClientId client);
	virtual const char* GetClientName(vncClientId client);

	// Let a client remove itself
	virtual void RemoveClient(vncClientId client);

	// Connect/disconnect notification
	virtual BOOL AddNotify(HWND hwnd);
	virtual BOOL RemNotify(HWND hwnd);

	virtual vncDesktop* GetDesktopPointer() {return m_desktop;}
	virtual void SetNewSWSize(long w,long h,BOOL desktop);
	virtual void SetBufferOffset(int x,int y);
	virtual void SetScreenOffset(int x,int y, bool single_display); //never locked
	virtual void InitialUpdate(bool value);
	virtual void AutoCapt(int autocapt);
	virtual int AutoCapt() { return m_autocapt; }
	short getOldestViewer();
	int getNumberViewers();

	virtual BOOL All_clients_initialalized();
	void initialCapture_done();

	// Lock to protect the client list from concurrency - lock when reading/updating client list
	omni_mutex			m_clientsLock;
	omni_mutex			m_clientsLockBlackList;
	omni_mutex			m_clientsLock_notifyList;

	UINT				m_port;
	UINT				m_port_http; // TightVNC 1.2.7
	int					m_autocapt;

	virtual void ShutdownServer();
	HANDLE retryThreadHandle;

protected:
	// Send a notification message
	

public:
	int AutoReconnect_counter;
	virtual void DoNotify(UINT message, WPARAM wparam, LPARAM lparam);
	// Update handling, used by the screen server
	virtual rfb::UpdateTracker &GetUpdateTracker() {return m_update_tracker;};
	virtual void UpdateMouse();
	// adzm - 2010-07 - Extended clipboard
	//virtual void UpdateClipText(const char* text);
	virtual void UpdateClipTextEx(HWND hwndOwner, vncClient* excludeClient = NULL);
	virtual void UpdatePalette(bool lock);
	virtual void UpdateLocalFormat(bool lock);

	// Polling mode handling
	virtual void PollUnderCursor(BOOL enable) {m_poll_undercursor = enable;};
	virtual BOOL PollUnderCursor() {return m_poll_undercursor;};
	virtual void PollForeground(BOOL enable) {m_poll_foreground = enable;};
	virtual BOOL PollForeground() {return m_poll_foreground;};
	virtual void PollFullScreen(BOOL enable) {m_poll_fullscreen = enable;};
	virtual BOOL PollFullScreen() {return m_poll_fullscreen;};

	virtual void Driver(BOOL enable);
	virtual BOOL Driver() {return m_driver;};

	BOOL DriverWanted;
	BOOL HookWanted;
	BOOL DriverWantedSet;

	virtual void Hook(BOOL enable);
	virtual BOOL Hook() {return m_hook;};
	virtual void Virtual(BOOL enable) {m_virtual = enable;};
	virtual BOOL Virtual() {return m_virtual;};
	virtual void SetHookings();

	virtual void PollConsoleOnly(BOOL enable) {m_poll_consoleonly = enable;};
	virtual BOOL PollConsoleOnly() {return m_poll_consoleonly;};
	virtual void PollOnEventOnly(BOOL enable) {m_poll_oneventonly = enable;};
	virtual BOOL PollOnEventOnly() {return m_poll_oneventonly;};
	virtual void MaxCpu(LONG maxcpu) {m_MaxCpu = maxcpu;};
	virtual LONG MaxCpu() {return m_MaxCpu;};
	virtual void MaxFPS(LONG maxFPS) { m_MaxFPS = maxFPS; };
	virtual LONG MaxFPS() { return m_MaxFPS; };

	// Client manipulation of the clipboard
	virtual void UpdateLocalClipText(LPSTR text);
	// adzm - 2010-07 - Extended clipboard
	virtual void UpdateLocalClipTextEx(ExtendedClipboardDataMessage& extendedClipboardDataMessage, vncClient* sourceClient);

	// Name and port number handling
	// TightVNC 1.2.7
	virtual void SetName(const char * name);
	virtual void SetPorts(const UINT port_rfb, const UINT port_http);
	virtual UINT GetPort() { return m_port; };
	virtual UINT GetHttpPort() { return m_port_http; };
	// RealVNC method
	/*
	virtual void SetPort(const UINT port);
	virtual UINT GetPort();
	*/
	virtual void KillSockConnect();
	virtual void SetAutoPortSelect(const BOOL autoport) {
	    if (autoport && !m_autoportselect)
	    {
		BOOL sockconnect = SockConnected();
		SockConnect(FALSE);
		m_autoportselect = autoport;
		SockConnect(sockconnect);
	    }
		else
		{
			m_autoportselect = autoport;
		}
	};
	virtual BOOL AutoPortSelect() {return m_autoportselect;};

	// Password set/retrieve.  Note that these functions now handle the encrypted
	// form, not the plaintext form.  The buffer passwed MUST be MAXPWLEN in size.
	virtual void SetPassword(const char *passwd);
	virtual void GetPassword(char *passwd);
	virtual void SetPassword2(const char *passwd2); //PGM
	virtual void GetPassword2(char *passwd2); //PGM

	// Remote input handling
	virtual void EnableRemoteInputs(BOOL enable);
	virtual BOOL RemoteInputsEnabled();

	// Local input handling
	virtual void DisableLocalInputs(BOOL disable);
	virtual bool LocalInputsDisabled();
	virtual BOOL JapInputEnabled();
	virtual BOOL UnicodeInputEnabled();
	virtual void EnableJapInput(BOOL enable);
	virtual void EnableUnicodeInput(BOOL enable);
	//virtual void Enable(BOOL enable);
	virtual BOOL Win8HelperEnabled();
	virtual void Win8HelperEnabled(BOOL enable);
	virtual void Clearconsole(BOOL enable);

	// General connection handling
	virtual void SetConnectPriority(UINT priority) {m_connect_pri = priority;};
	virtual UINT ConnectPriority() {return m_connect_pri;};

	// Socket connection handling
	virtual BOOL SockConnect(BOOL on);
	virtual BOOL SockConnected();
	virtual BOOL SetLoopbackOnly(BOOL loopbackOnly);
	virtual BOOL LoopbackOnly();

	void SetSendExtraMouse(BOOL i_fSendExtraMouse);
	BOOL SendExtraMouse();

	// Tray icon disposition
	virtual BOOL SetDisableTrayIcon(BOOL disableTrayIcon);
	virtual BOOL GetDisableTrayIcon();
	virtual BOOL SetRdpmode(BOOL Rdpmode);
	virtual BOOL GetRdpmode();
	virtual BOOL SetNoScreensaver(BOOL NoScreensaver);
	virtual BOOL GetNoScreensaver();
	virtual BOOL SetAllowEditClients(BOOL AllowEditClients);
	virtual BOOL GetAllowEditClients();


	// HTTP daemon handling
	virtual BOOL EnableHTTPConnect(BOOL enable);
	virtual BOOL HTTPConnectEnabled() {return m_enableHttpConn;};



	virtual void GetScreenInfo(int &width, int &height, int &depth);

	// Allow connections if no password is set?
	virtual void SetAuthRequired(BOOL reqd) {m_passwd_required = reqd;};
	virtual BOOL AuthRequired() {return m_passwd_required;};

	// Handling of per-client connection authorisation
	virtual void SetAuthHosts(const char *hostlist);
	virtual char *AuthHosts();
	enum AcceptQueryReject {aqrAccept, aqrQuery, aqrReject};
	virtual AcceptQueryReject VerifyHost(const char *hostname);

	// Blacklisting of machines which fail connection attempts too often
	// Such machines will fail VerifyHost for a short period
	virtual void AddAuthHostsBlacklist(const char *machine);
	virtual void RemAuthHostsBlacklist(const char *machine);

	// Connection querying settings
	virtual void SetQuerySetting(const UINT setting) {m_querysetting = setting;};
	virtual UINT QuerySetting() {return m_querysetting;};
	virtual void SetQueryAccept(const UINT setting) {m_queryaccept = setting;};
	virtual UINT QueryAccept() 
	{
		if (m_queryaccept==2) return 0;
		else return m_queryaccept;
	};
	virtual UINT QueryAcceptForSave() 
	{
		return m_queryaccept;
	};
	virtual UINT QueryAcceptLocked() 
	{
		if (m_queryaccept==2) return 0;
		else return 1;
	};
	virtual void SetQueryTimeout(const UINT setting) {m_querytimeout = setting;};
	virtual UINT QueryTimeout() {return m_querytimeout;};

	virtual void SetQueryDisableTime(const UINT setting) {m_querydisabletime = setting;};
	virtual UINT QueryDisableTime() {return m_querydisabletime;};

	// marscha@2006 - Is AcceptDialog required even if no user is logged on
    virtual void SetQueryIfNoLogon(const UINT setting) {m_queryifnologon = (setting != 0);};
	virtual BOOL QueryIfNoLogon() {return m_queryifnologon;};

	UINT getMaxViewerSetting()
	{
		return m_maxViewerSetting;
	};

	void setMaxViewerSetting(const UINT setting)
	{
		m_maxViewerSetting = setting;
	};

	BOOL getCollabo()
	{
		return m_Collabo;
	};

	void setCollabo(const BOOL setting)
	{
		m_Collabo = setting;
	};

	UINT getMaxViewers()
	{
		return m_maxViewers;
	};

	void setMaxViewers(const UINT setting)
	{
		m_maxViewers = setting;
	};

	// Whether or not to allow connections from the local machine
	virtual void SetLoopbackOk(BOOL ok) {m_loopback_allowed = ok;};
	virtual BOOL LoopbackOk() {return m_loopback_allowed;};
#ifdef IPV6V4
	// Whether or not to allow connections from the local machine
	virtual void SetIPV6(BOOL ok) { G_ipv6_allowed = ok; };
	virtual BOOL IPV6() { return G_ipv6_allowed; };
#endif
	// Whether or not to shutdown or logoff when the last client leaves
	virtual void SetLockSettings(int ok) {m_lock_on_exit = ok;};
	virtual int LockSettings() {return m_lock_on_exit;};

	// Timeout for automatic disconnection of idle connections
	virtual void SetAutoIdleDisconnectTimeout(const UINT timeout) {m_idle_timeout = timeout;};
	virtual UINT AutoIdleDisconnectTimeout() {return m_idle_timeout;};

	// Removal of desktop wallpaper, etc
	virtual void EnableRemoveWallpaper(const BOOL enable) {m_remove_wallpaper = enable;};
	virtual BOOL RemoveWallpaperEnabled() {return m_remove_wallpaper;};
	// adzm - 2010-07 - Disable more effects or font smoothing
	virtual void EnableRemoveEffects(const BOOL enable) {m_remove_effects = enable;};
	virtual BOOL RemoveEffectsEnabled() {return m_remove_effects;};
	virtual void EnableRemoveFontSmoothing(const BOOL enable) {m_remove_fontsmoothing = enable;};
	virtual BOOL RemoveFontSmoothingEnabled() {return m_remove_fontsmoothing;};
	// Removal of desktop composit desktop, etc
	virtual void EnableRemoveAero(const BOOL enable) {m_remove_Aero = enable;};
	virtual BOOL RemoveAeroEnabled() {return m_remove_Aero;};

	// sf@2002 - v1.1.x - Server Default Scale
	virtual UINT GetDefaultScale();
	virtual BOOL SetDefaultScale(int nScale);
	virtual BOOL FileTransferEnabled();
	virtual BOOL EnableFileTransfer(BOOL fEnable);
	virtual BOOL BlankMonitorEnabled() {return m_fBlankMonitorEnabled;};
	virtual void BlankMonitorEnabled(BOOL fEnable) {m_fBlankMonitorEnabled = fEnable;};
	virtual BOOL BlankInputsOnly() {return m_fBlankInputsOnly;}; //PGM
	virtual void BlankInputsOnly(BOOL fEnable) {m_fBlankInputsOnly = fEnable;}; //PGM
	virtual BOOL Secure();
	virtual BOOL Secure(BOOL fEnable);
	virtual BOOL MSLogonRequired();
	virtual BOOL RequireMSLogon(BOOL fEnable);
	virtual BOOL GetNewMSLogon();
	virtual BOOL SetNewMSLogon(BOOL fEnable);

	virtual BOOL Primary() {return m_PrimaryEnabled;};
	virtual void Primary(BOOL fEnable) {m_PrimaryEnabled = fEnable;};
	virtual BOOL Secondary() {return m_SecondaryEnabled;};
	virtual void Secondary(BOOL fEnable) {m_SecondaryEnabled = fEnable;};

	// sf@2002 - DSM Plugin
	virtual BOOL IsDSMPluginEnabled();
	virtual void EnableDSMPlugin(BOOL fEnable);
	virtual char* GetDSMPluginName();
	virtual void SetDSMPluginName(char* szDSMPlugin);
	virtual BOOL SetDSMPlugin(BOOL fForceReload);
	virtual CDSMPlugin* GetDSMPluginPointer() { return m_pDSMPlugin;};
	//adzm 2010-05-12 - dsmplugin config
	virtual void SetDSMPluginConfig(char* szDSMPluginConfig);
	virtual char* GetDSMPluginConfig() { return m_szDSMPluginConfig;};

	// sf@2002 - Cursor handling
	virtual void EnableXRichCursor(BOOL fEnable);
	virtual BOOL IsXRichCursorEnabled() {return m_fXRichCursor;}; 

	// sf@2002
	virtual void DisableCacheForAllClients();
	virtual bool IsThereASlowClient();
	virtual bool IsThereAUltraEncodingClient();
	virtual bool IsThereAUltra2EncodingClient();
	virtual bool IsEncoderSet();
	virtual bool IsThereFileTransBusy();

	// sf@2002 - Turbo Mode
	virtual void TurboMode(BOOL fEnabled) { m_TurboMode = fEnabled; };
	virtual BOOL TurboMode() { return m_TurboMode; };

	// sf@2003 - AutoReconnect
	virtual BOOL AutoReconnect()
	{
		if (fShutdownOrdered) return false;
		return m_fAutoReconnect;
	};
	virtual BOOL IdReconnect(){return m_fIdReconnect;};
	virtual UINT AutoReconnectPort(){return m_AutoReconnectPort;};
	virtual char* AutoReconnectAdr(){return m_szAutoReconnectAdr;}
	virtual char* AutoReconnectId(){return m_szAutoReconnectId;}
	virtual void AutoReconnect(BOOL fEnabled){m_fAutoReconnect = fEnabled;};
	virtual void IdReconnect(BOOL fEnabled){m_fIdReconnect = fEnabled;};
	virtual void AutoReconnectPort(UINT nPort){m_AutoReconnectPort = nPort;};
	virtual void AutoReconnectAdr(char* szAdr){strcpy_s(m_szAutoReconnectAdr,255, szAdr);}
	virtual void AutoReconnectId(char* szId){strcpy_s(m_szAutoReconnectId,MAX_PATH, szId);}
	virtual void AutoConnectRetry( );
	void actualRetryThread();
	VSocket *retrysock;

	// sf@2007 - Vista / XP FUS special modes
	virtual BOOL RunningFromExternalService(){return m_fRunningFromExternalService;};
	virtual void RunningFromExternalService(BOOL fEnabled){m_fRunningFromExternalService = fEnabled;};

	virtual BOOL RunningFromExternalServiceRdp(){return m_fRunningFromExternalServiceRdp;};
	virtual void RunningFromExternalServiceRdp(BOOL fEnabled){m_fRunningFromExternalServiceRdp = fEnabled;};

	virtual void AutoRestartFlag(BOOL fOn){m_fAutoRestart = fOn;};
	virtual BOOL AutoRestartFlag(){return m_fAutoRestart;};

	// sf@2005 - FTUserImpersonation
	virtual BOOL FTUserImpersonation(){return m_fFTUserImpersonation;};
	virtual void FTUserImpersonation(BOOL fEnabled){m_fFTUserImpersonation = fEnabled;};

	virtual void Clear_Update_Tracker();
	virtual void UpdateCursorShape();

	bool IsClient(vncClient* pClient);

    void EnableServerStateUpdates(bool newstate) { m_fEnableStateUpdates = newstate; }
    bool DoServerStateUpdates() { return m_fEnableStateUpdates; }
    void NotifyClients_StateChange(CARD32 state, CARD32 value);
    int  GetFTTimeout() { return m_ftTimeout; }
    int  GetKeepAliveInterval () { return m_keepAliveInterval; }
	int  GetIdleInputTimeout() { return m_IdleInputTimeout; }
	void SetIdleInputTimeout(int secs) { m_IdleInputTimeout = secs; }

    void SetFTTimeout(int msecs);
    void EnableKeepAlives(bool newstate) { m_fEnableKeepAlive = newstate; }
    bool DoKeepAlives() { return m_fEnableKeepAlive; }
    void SetKeepAliveInterval(int secs) { 
        m_keepAliveInterval = secs; 
    if (m_keepAliveInterval >= (m_ftTimeout - KEEPALIVE_HEADROOM))
        m_keepAliveInterval = m_ftTimeout  - KEEPALIVE_HEADROOM;
    }

	void TriggerUpdate();
	void SetHasMouse();

	bool OS_Shutdown;
	void StopReconnectAll();
	int m_virtualDisplaySupported;
	VirtualDisplay *virtualDisplay;

protected:
	// The vncServer UpdateTracker class
	// Behaves like a standard UpdateTracker, but propagates update
	// information to active clients' trackers

	class ServerUpdateTracker : public rfb::UpdateTracker {
	public:
		ServerUpdateTracker() : m_server(0) {};

		virtual void init(vncServer *server) {m_server=server;};

		virtual void add_changed(const rfb::Region2D &region);
		virtual void add_cached(const rfb::Region2D &region);
		virtual void add_copied(const rfb::Region2D &dest, const rfb::Point &delta);
	protected:
		vncServer *m_server;
	};

	friend class ServerUpdateTracker;

	ServerUpdateTracker	m_update_tracker;

	// Internal stuffs
protected:
	static void*	pThis;

	// Connection servers
	vncSockConnect		*m_socketConn;
	vncHTTPConnect		*m_httpConn;
	BOOL				m_enableHttpConn;


	// The desktop handler
	vncDesktop			*m_desktop;

	// General preferences
//	UINT				m_port;
//	UINT				m_port_http; // TightVNC 1.2.7
	BOOL				m_autoportselect;
	char				m_password[MAXPWLEN];
	char				m_password2[MAXPWLEN]; //PGM
	BOOL				m_passwd_required;
	BOOL				m_loopback_allowed;	
	BOOL				m_loopbackOnly;
	char				*m_auth_hosts;
	BOOL				m_enable_remote_inputs;
	BOOL				m_disable_local_inputs;
	BOOL				m_enable_jap_input;
	BOOL				m_enable_unicode_input;
	BOOL				m_enable_win8helper;
	int					m_lock_on_exit;
	int					m_connect_pri;
	UINT				m_querysetting;
	UINT				m_queryaccept;
	UINT				m_querytimeout;
	UINT				m_querydisabletime;
	BOOL				m_queryifnologon;
 	UINT				m_idle_timeout;

	UINT				m_maxViewerSetting;
	UINT				m_maxViewers;
	BOOL				m_Collabo;
	BOOL				m_remove_wallpaper;
	// adzm - 2010-07 - Disable more effects or font smoothing
	BOOL				m_remove_effects;
	BOOL				m_remove_fontsmoothing;
	BOOL				m_remove_Aero;
	BOOL				m_disableTrayIcon;
	BOOL				m_Rdpmode;
	BOOL				m_NoScreensaver;
	BOOL				m_AllowEditClients;

	// Polling preferences
	BOOL				m_poll_fullscreen;
	BOOL				m_poll_foreground;
	BOOL				m_poll_undercursor;

	BOOL				m_poll_oneventonly;
	LONG				m_MaxCpu;
	LONG				m_MaxFPS;
	BOOL				m_poll_consoleonly;

	BOOL				m_driver;
	BOOL				m_hook;
	BOOL				m_virtual;
	BOOL				sethook;

	// Name of this desktop
	char				*m_name;

	// Blacklist structures
	struct BlacklistEntry {
		BlacklistEntry *_next;
		char *_machineName;
		LARGE_INTEGER _lastRefTime;
		UINT _failureCount;
		BOOL _blocked;
	};
	BlacklistEntry		*m_blacklist;
	
	// The client lists - list of clients being authorised and ones
	// already authorised
	vncClientList		m_unauthClients;
	vncClientList		m_authClients;
	vncClient			*m_clientmap[MAX_CLIENTS];
	vncClientId			m_nextid;

	omni_mutex			m_desktopLock;
	// Signal set when a client removes itself
	omni_condition		*m_clientquitsig;
	// Set of windows to send notifications to
	vncNotifyList		m_notifyList;

	// Modif sf@2002
	BOOL    m_TurboMode;

	// Modif sf@2002 - v1.1.x
	// BOOL    m_fQueuingEnabled;
	BOOL    m_fFileTransferEnabled;
	BOOL    m_fBlankMonitorEnabled;
	BOOL    m_fBlankInputsOnly; //PGM
	int     m_nDefaultScale;

	BOOL m_PrimaryEnabled;
	BOOL m_SecondaryEnabled;

	BOOL	m_Secure;
	BOOL    m_fMSLogonRequired;
	BOOL    m_fNewMSLogon;

	// sf@2002 - DSMPlugin
	BOOL m_fDSMPluginEnabled;
	char m_szDSMPlugin[128];
	CDSMPlugin *m_pDSMPlugin;
	//adzm 2010-05-12 - dsmplugin config
	char m_szDSMPluginConfig[512];

	// sf@2002 - Cursor handling
	BOOL m_fXRichCursor; 

	// sf@2003 - AutoReconnect
	BOOL m_fAutoReconnect;
	BOOL m_fIdReconnect;
	UINT m_AutoReconnectPort;
	char m_szAutoReconnectAdr[255];
	char m_szAutoReconnectId[MAX_PATH];

	// sf@2007
	BOOL m_fRunningFromExternalService;
	BOOL m_fRunningFromExternalServiceRdp;
	BOOL m_fAutoRestart;

	// sf@2005 - FTUserImpersonation
	BOOL m_fFTUserImpersonation;

	HINSTANCE   hWtsLib;
    bool m_fEnableStateUpdates;
    bool m_fEnableKeepAlive;
    int m_ftTimeout;
    int m_keepAliveInterval;
	int m_IdleInputTimeout;
	bool clearconsole;
	DWORD startTime;
    BOOL m_fSendExtraMouse;
	bool KillAuthClientsBuzy;	
};

#endif
