/*
 * Copyright 2003 Martin Fuchs
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


 //
 // Explorer and Desktop clone
 //
 // startmenu.h
 //
 // Martin Fuchs, 16.08.2003
 //


#define	CLASSNAME_STARTMENU		_T("ReactosStartmenuClass")
#define	TITLE_STARTMENU			_T("Start Menu")


#define	STARTMENU_WIDTH_MIN		120
#define	STARTMENU_LINE_HEIGHT	22
#define	STARTMENU_SEP_HEIGHT	(STARTMENU_LINE_HEIGHT/2)


 // private message constants
#define	PM_STARTMENU_CLOSED		(WM_APP+0x11)
#define	PM_STARTENTRY_FOCUSED	(WM_APP+0x12)
#define	PM_STARTENTRY_LAUNCHED	(WM_APP+0x13)


struct StartMenuDirectory
{
	StartMenuDirectory(const ShellDirectory& dir, bool subfolders=true)
	 :	_dir(dir), _subfolders(subfolders)
	{
	}

	~StartMenuDirectory()
	{
		_dir.free_subentries();
	}

	ShellDirectory _dir;
	bool	_subfolders;
};

typedef list<StartMenuDirectory> StartMenuShellDirs;

struct StartMenuEntry
{
	StartMenuEntry() : _entry(NULL), _hIcon(0) {}

	String	_title;
	HICON	_hIcon;
	const ShellEntry* _entry;
};


 /**
	StartMenuButton draws to face of a StartMenuCtrl button control.
 */
struct StartMenuButton : public OwnerdrawnButton
{
	typedef OwnerdrawnButton super;

	StartMenuButton(HWND hwnd, HICON hIcon, bool hasSubmenu)
	 :	super(hwnd), _hIcon(hIcon), _hasSubmenu(hasSubmenu) {}

	static int GetTextWidth(LPCTSTR title, HWND hwnd);

protected:
	LRESULT	WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);
	virtual void DrawItem(LPDRAWITEMSTRUCT dis);

	HICON	_hIcon;
	bool	_hasSubmenu;
};


 /**
	To create a Startmenu button control, construct a StartMenuCtrl object.
 */
struct StartMenuCtrl : public Button
{
	StartMenuCtrl(HWND parent, int x, int y, int w, LPCTSTR title,
					UINT id, HICON hIcon=0, bool hasSubmenu=false, DWORD style=WS_VISIBLE|WS_CHILD|BS_OWNERDRAW, DWORD exStyle=0)
	 :	Button(parent, title, x, y, w, STARTMENU_LINE_HEIGHT, id, style, exStyle)
	{
		*new StartMenuButton(_hwnd, hIcon, hasSubmenu);

		SetWindowFont(_hwnd, GetStockFont(DEFAULT_GUI_FONT), FALSE);
	}
};


struct StartMenuSeparator : public Static
{
	StartMenuSeparator(HWND parent, int x, int y, int w, DWORD style=WS_VISIBLE|WS_CHILD|WS_DISABLED|SS_ETCHEDHORZ, DWORD exStyle=0)
	 :	Static(parent, NULL, x, y+STARTMENU_SEP_HEIGHT/2-1, w, 2, -1, style, exStyle)
	{
	}
};


typedef list<ShellPath> StartMenuFolders;
#define STARTMENU_CREATOR(WND_CLASS) WINDOW_CREATOR_INFO(WND_CLASS, StartMenuFolders)

typedef map<UINT, StartMenuEntry> ShellEntryMap;


 /**
	Startmenu window
 */
struct StartMenu : public OwnerDrawParent<Dialog>
{
	typedef OwnerDrawParent<Dialog> super;

	StartMenu(HWND hwnd);
	StartMenu(HWND hwnd, const StartMenuFolders& info);
	~StartMenu();

//	static HWND Create(int x, int y, HWND hwndParent=0);
	static HWND Create(int x, int y, const StartMenuFolders&, HWND hwndParent=0, CREATORFUNC creator=s_def_creator);
	static CREATORFUNC s_def_creator;

protected:
	 // overridden member functions
	LRESULT	Init(LPCREATESTRUCT pcs);
	LRESULT	WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);
	int		Command(int id, int code);

	 // window class
	static BtnWindowClass s_wcStartMenu;

	 // data members
	int		_next_id;
	ShellEntryMap _entries;
	StartMenuShellDirs _dirs;

	UINT	_submenu_id;
	WindowHandle _submenu;

	int		_border_left;	// left border in pixels

	 // member functions
	void	ResizeButtons(int cx);

	virtual void AddEntries();

	StartMenuEntry& AddEntry(LPCTSTR title, HICON hIcon=0, UINT id=(UINT)-1);
	StartMenuEntry& AddEntry(const ShellFolder folder, const ShellEntry* entry);

	void	AddShellEntries(const ShellDirectory& dir, int max=-1, bool subfolders=true);

	void	AddButton(LPCTSTR title, HICON hIcon=0, bool hasSubmenu=false, UINT id=(UINT)-1, DWORD style=WS_VISIBLE|WS_CHILD|BS_OWNERDRAW);
	void	AddSeparator();

	bool	CloseOtherSubmenus(int id);
	void	CreateSubmenu(int id, const StartMenuFolders& new_folders, CREATORFUNC creator=s_def_creator);
	void	CreateSubmenu(int id, int folder1, int folder2, CREATORFUNC creator=s_def_creator);
	void	CreateSubmenu(int id, int folder, CREATORFUNC creator=s_def_creator);
	void	ActivateEntry(int id, ShellEntry* entry);
	void	CloseStartMenu(int id=0);
};


 // declare shell32's "Run..." dialog export function
typedef	void (WINAPI* RUNFILEDLG)(HWND hwndOwner, HICON hIcon, LPCSTR lpstrDirectory, LPCSTR lpstrTitle, LPCSTR lpstrDescription, UINT uFlags);

 //
 // Flags for RunFileDlg
 //

#define	RFF_NOBROWSE		0x01	// Removes the browse button. 
#define	RFF_NODEFAULT		0x02	// No default item selected. 
#define	RFF_CALCDIRECTORY	0x04	// Calculates the working directory from the file name.
#define	RFF_NOLABEL			0x08	// Removes the edit box label. 
#define	RFF_NOSEPARATEMEM	0x20	// Removes the Separate Memory Space check box (Windows NT only).


 // declare more undocumented shell32 functions
typedef	void (WINAPI* EXITWINDOWSDLG)(HWND hwndOwner);
typedef	int (WINAPI* RESTARTWINDOWSDLG)(HWND hwndOwner, LPCWSTR reason, UINT flags);
typedef	BOOL (WINAPI* SHFINDFILES)(LPCITEMIDLIST pidlRoot, LPCITEMIDLIST pidlSavedSearch);
typedef	BOOL (WINAPI* SHFINDCOMPUTER)(LPCITEMIDLIST pidlRoot, LPCITEMIDLIST pidlSavedSearch);


 // Startmenu root window
struct StartMenuRoot : public StartMenu
{
	typedef StartMenu super;

	StartMenuRoot(HWND hwnd);

	static HWND Create(HWND hwndDesktopBar);

protected:
	LRESULT	Init(LPCREATESTRUCT pcs);
	LRESULT	WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);
	int		Command(int id, int code);

	static void	ShowLaunchDialog(HWND hwndDesktopBar);
	static void	ShowExitWindowsDialog(HWND hwndOwner);
	static void	ShowRestartDialog(HWND hwndOwner, UINT flags);
	static void	ShowSearchDialog();
	static void	ShowSearchComputer();

	SIZE	_logo_size;
};


struct RecentStartMenu : public StartMenu
{
	typedef StartMenu super;

	RecentStartMenu(HWND hwnd, const StartMenuFolders& info);

protected:
	virtual void AddEntries();
};