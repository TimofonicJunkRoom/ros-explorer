/*
 * Copyright 2003, 2004, 2005 Martin Fuchs
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
 // Explorer clone, lean version
 //
 // entries.cpp
 //
 // Martin Fuchs, 23.07.2003
 //


#include "precomp.h"


 // allocate and initialise a directory entry
ShellEntry::ShellEntry()
{
	_up = NULL;
	_next = NULL;
	_down = NULL;
	_expanded = false;
	_scanned = false;
	_level = 0;
	_icon_id = ICID_UNKNOWN;
	_display_name = _data.cFileName;
}

ShellEntry::ShellEntry(LPITEMIDLIST shell_path)
 : _pidl(shell_path)
{
	_up = NULL;
	_next = NULL;
	_down = NULL;
	_expanded = false;
	_scanned = false;
	_level = 0;
	_icon_id = ICID_UNKNOWN;
	_display_name = _data.cFileName;
}

ShellEntry::ShellEntry(const ShellPath& shell_path)
 : _pidl(shell_path)
{
	_up = NULL;
	_next = NULL;
	_down = NULL;
	_expanded = false;
	_scanned = false;
	_level = 0;
	_icon_id = ICID_UNKNOWN;
	_display_name = _data.cFileName;
}


ShellEntry::ShellEntry(ShellDirectory* parent, const ShellPath& shell_path)
 :	_up(parent),
	_pidl(shell_path)
{
	_next = NULL;
	_down = NULL;
	_expanded = false;
	_scanned = false;
	_level = 0;
	_icon_id = ICID_UNKNOWN;
	_display_name = _data.cFileName;
}

ShellEntry::ShellEntry(ShellDirectory* parent, LPITEMIDLIST shell_path)
 :	_up(parent),
	_pidl(shell_path)
{
	_next = NULL;
	_down = NULL;
	_expanded = false;
	_scanned = false;
	_level = 0;
	_icon_id = ICID_UNKNOWN;
	_display_name = _data.cFileName;
}

ShellEntry::ShellEntry(const ShellEntry& other)
 :	_pidl(other._pidl)
{
	_next = NULL;
	_down = NULL;
	_up = NULL;

	assert(!other._next);
	assert(!other._down);
	assert(!other._up);

	_expanded = other._expanded;
	_scanned = other._scanned;
	_level = other._level;

	_data = other._data;

	_shell_attribs = other._shell_attribs;
	_display_name = other._display_name==other._data.cFileName? _data.cFileName: _tcsdup(other._display_name);

	_icon_id = other._icon_id;
}

 // free a directory entry
ShellEntry::~ShellEntry()
{
	if (_icon_id > ICID_NONE)
		g_Globals._icon_cache.free_icon(_icon_id);

	if (_display_name != _data.cFileName)
		free(_display_name);
}


void ShellDirectory::read_directory(SORT_ORDER sortOrder, int scan_flags)
{
	CONTEXT("ShellEntry::read_directory(SORT_ORDER)");

	 // call into subclass
	read_directory(scan_flags);

#ifndef ROSSHELL
	if (g_Globals._prescan_nodes) {	//@todo _prescan_nodes should not be used for reading the start menu.
		for(ShellEntry*entry=_down; entry; entry=entry->_next)
			if (entry->_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				ShellDirectory* dir = static_cast<ShellDirectory*>(entry);

				dir->read_directory(scan_flags);
				dir->sort_directory(sortOrder);
			}
	}
#endif

	sort_directory(sortOrder);
}


Root::Root()
{
	memset(this, 0, sizeof(Root));
}

Root::~Root()
{
	if (_entry)
		_entry->free_subentries();
}


 // directories first...
static int compareType(const WIN32_FIND_DATA* fd1, const WIN32_FIND_DATA* fd2)
{
	int order1 = fd1->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
	int order2 = fd2->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;

	/* Handle "." and ".." as special case and move them at the very first beginning. */
	if (order1 && order2) {
		order1 = fd1->cFileName[0]!='.'? 1: fd1->cFileName[1]=='.'? 2: fd1->cFileName[1]=='\0'? 3: 1;
		order2 = fd2->cFileName[0]!='.'? 1: fd2->cFileName[1]=='.'? 2: fd2->cFileName[1]=='\0'? 3: 1;
	}

	return order2==order1? 0: order2<order1? -1: 1;
}


static int compareNothing(const void* arg1, const void* arg2)
{
	return -1;
}

static int compareName(const void* arg1, const void* arg2)
{
	const WIN32_FIND_DATA* fd1 = &(*(ShellEntry**)arg1)->_data;
	const WIN32_FIND_DATA* fd2 = &(*(ShellEntry**)arg2)->_data;

	int cmp = compareType(fd1, fd2);
	if (cmp)
		return cmp;

	return lstrcmpi(fd1->cFileName, fd2->cFileName);
}

static int compareExt(const void* arg1, const void* arg2)
{
	const WIN32_FIND_DATA* fd1 = &(*(ShellEntry**)arg1)->_data;
	const WIN32_FIND_DATA* fd2 = &(*(ShellEntry**)arg2)->_data;
	const TCHAR *name1, *name2, *ext1, *ext2;

	int cmp = compareType(fd1, fd2);
	if (cmp)
		return cmp;

	name1 = fd1->cFileName;
	name2 = fd2->cFileName;

	ext1 = _tcsrchr(name1, TEXT('.'));
	ext2 = _tcsrchr(name2, TEXT('.'));

	if (ext1)
		++ext1;
	else
		ext1 = TEXT("");

	if (ext2)
		++ext2;
	else
		ext2 = TEXT("");

	cmp = lstrcmpi(ext1, ext2);
	if (cmp)
		return cmp;

	return lstrcmpi(name1, name2);
}

static int compareSize(const void* arg1, const void* arg2)
{
	WIN32_FIND_DATA* fd1 = &(*(ShellEntry**)arg1)->_data;
	WIN32_FIND_DATA* fd2 = &(*(ShellEntry**)arg2)->_data;

	int cmp = compareType(fd1, fd2);
	if (cmp)
		return cmp;

	cmp = fd2->nFileSizeHigh - fd1->nFileSizeHigh;

	if (cmp < 0)
		return -1;
	else if (cmp > 0)
		return 1;

	cmp = fd2->nFileSizeLow - fd1->nFileSizeLow;

	return cmp<0? -1: cmp>0? 1: 0;
}

static int compareDate(const void* arg1, const void* arg2)
{
	WIN32_FIND_DATA* fd1 = &(*(ShellEntry**)arg1)->_data;
	WIN32_FIND_DATA* fd2 = &(*(ShellEntry**)arg2)->_data;

	int cmp = compareType(fd1, fd2);
	if (cmp)
		return cmp;

	return CompareFileTime(&fd2->ftLastWriteTime, &fd1->ftLastWriteTime);
}


static int (*sortFunctions[])(const void* arg1, const void* arg2) = {
	compareNothing,	// SORT_NONE
	compareName,	// SORT_NAME
	compareExt, 	// SORT_EXT
	compareSize,	// SORT_SIZE
	compareDate 	// SORT_DATE
};


void ShellDirectory::sort_directory(SORT_ORDER sortOrder)
{
	if (sortOrder != SORT_NONE) {
		ShellEntry* entry = _down;
		ShellEntry** array, **p;
		int len;

		len = 0;
		for(entry=_down; entry; entry=entry->_next)
			++len;

		if (len) {
			array = (ShellEntry**) alloca(len*sizeof(ShellEntry*));

			p = array;
			for(entry=_down; entry; entry=entry->_next)
				*p++ = entry;

			 // call qsort with the appropriate compare function
			qsort(array, len, sizeof(array[0]), sortFunctions[sortOrder]);

			_down = array[0];

			for(p=array; --len; p++)
				(*p)->_next = p[1];

			(*p)->_next = 0;
		}
	}
}


void ShellDirectory::smart_scan(int scan_flags)
{
	CONTEXT("ShellEntry::smart_scan()");

	if (!_scanned) {
		free_subentries();
		read_directory(SORT_NAME, scan_flags);	// we could use IShellFolder2::GetDefaultColumn to determine sort order
	}
}


int ShellEntry::extract_icon()
{
	TCHAR path[MAX_PATH];

	ICON_ID icon_id = ICID_NONE;

	if (get_path(path, COUNTOF(path)))
		icon_id = g_Globals._icon_cache.extract(path);

	if (icon_id == ICID_NONE) {
		IExtractIcon* pExtract;
		if (SUCCEEDED(GetUIObjectOf(0, IID_IExtractIcon, (LPVOID*)&pExtract))) {
			unsigned flags;
			int idx;

			if (SUCCEEDED(pExtract->GetIconLocation(GIL_FORSHELL, path, MAX_PATH, &idx, &flags))) {
				if (flags & GIL_NOTFILENAME)
					icon_id = g_Globals._icon_cache.extract(pExtract, path, idx);
				else {
					if (idx == -1)
						idx = 0;	// special case for some control panel applications ("System")

					icon_id = g_Globals._icon_cache.extract(path, idx);
				}

			/* using create_absolute_pidl() [see below] results in more correct icons for some control panel applets ("NVidia").
				if (icon_id == ICID_NONE) {
					SHFILEINFO sfi;

					if (SHGetFileInfo(path, 0, &sfi, sizeof(sfi), SHGFI_ICON|SHGFI_SMALLICON))
						icon_id = g_Globals._icon_cache.add(sfi.hIcon)._id;
				} */
			/*
				if (icon_id == ICID_NONE) {
					LPBYTE b = (LPBYTE) alloca(0x10000);
					SHFILEINFO sfi;

					FILE* file = fopen(path, "rb");
					if (file) {
						int l = fread(b, 1, 0x10000, file);
						fclose(file);

						if (l)
							icon_id = g_Globals._icon_cache.add(CreateIconFromResourceEx(b, l, TRUE, 0x00030000, 16, 16, LR_DEFAULTCOLOR));
					}
				} */
			}
		}

		if (icon_id == ICID_NONE) {
			SHFILEINFO sfi;

			const ShellPath& pidl_abs = create_absolute_pidl();
			LPCITEMIDLIST pidl = pidl_abs;

			HIMAGELIST himlSys = (HIMAGELIST) SHGetFileInfo((LPCTSTR)pidl, 0, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX|SHGFI_PIDL|SHGFI_SMALLICON);
			if (himlSys)
				icon_id = g_Globals._icon_cache.add(sfi.iIcon);
			/*
			if (SHGetFileInfo((LPCTSTR)pidl, 0, &sfi, sizeof(sfi), SHGFI_PIDL|SHGFI_ICON|SHGFI_SMALLICON))
				icon_id = g_Globals._icon_cache.add(sfi.hIcon)._id;
			*/
		}
	}

	return icon_id;
}

int ShellEntry::safe_extract_icon()
{
	try {
		return extract_icon();
	} catch(COMException&) {
		// ignore unexpected exceptions while extracting icons
	}

	return ICID_NONE;
}


 // recursively free all child entries
void ShellEntry::free_subentries()
{
	ShellEntry *entry, *next=_down;

	if (next) {
		_down = 0;

		do {
			entry = next;
			next = entry->_next;

			entry->free_subentries();
			delete entry;
		} while(next);
	}
}
