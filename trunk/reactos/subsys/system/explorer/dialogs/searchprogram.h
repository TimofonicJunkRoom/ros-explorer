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
 // Explorer clone
 //
 // searchprogram.h
 //
 // Explorer dialogs
 //
 // Martin Fuchs, 02.10.2003
 //

#include <stack>


typedef void (*COLLECT_CALLBACK)(ShellFolder& folder, ShellEntry* shell_entry, void* param);
typedef stack<ShellDirectory*> ShellDirectoryStack;

struct CollectProgramsThread : public Thread
{
	CollectProgramsThread(COLLECT_CALLBACK callback, HWND hwnd, void* para)
	 :	_cache_valid(false),
		_callback(callback),
		_hwnd(hwnd),
		_para(para)
	{
	}

	~CollectProgramsThread()
	{
		free_dirs();
	}

	int		Run();
	void	free_dirs();

	bool	_cache_valid;

protected:
	COLLECT_CALLBACK _callback;
	HWND	_hwnd;
	void*	_para;
	ShellDirectoryStack _dirs;

	void	collect_programs(const ShellPath& path);
};


struct FPDEntry
{
	ShellEntry* _shell_entry;
	int		_idxIcon;
	String	_menu_path;
	String	_path;
};

typedef list<FPDEntry> FPDCache;


struct FindProgramDlg : public ResizeController<Dialog>
{
	typedef ResizeController<Dialog> super;

	FindProgramDlg(HWND hwnd);
	~FindProgramDlg();

protected:
	CommonControlInit _usingCmnCtrl;
	HWND	_list_ctrl;
	HACCEL	_haccel;
	HIMAGELIST _himl;
	int		_idxNoIcon;	// replacement icon
	String	_lwr_filter;

	CollectProgramsThread _thread;
	FPDCache _cache;

	String	_common_programs, _user_programs;

	ListSort _sort;

	virtual int	Command(int id, int code);
	virtual int	Notify(int id, NMHDR* pnmh);

	void	Refresh(bool delete_cache=false);
	void	add_entry(const FPDEntry& cache_entry);
	void	LaunchSelected();

	static void collect_programs_callback(ShellFolder& folder, ShellEntry* entry, void* param);
	static int CALLBACK CompareFunc(LPARAM lparam1, LPARAM lparam2, LPARAM lparamSort);
};
