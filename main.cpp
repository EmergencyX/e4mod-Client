/*
	Emergency 4 (Deluxe) ModInstaller 
	Copyright (c) 2009 sixteen tons entertainment/Promotion Software GmbH (www.sixteen-tons.de)

	This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <windows.h>
#include <commctrl.h>
#include <map>
#include <vector>
#include <string>
#include <list>
#include <cstdio>
#include "resource.h"

#include "thirdparty/tinyxml/tinyxml.h"
#include "thirdparty/zlib/zlib.h"

#pragma warning(disable: 4267 4244)

//#define COMPRESS_PACKAGE
#define EM4_DELUXE

#define FILEVERSION 0x00000101

#ifdef COMPRESS_PACKAGE
	#define FileType gzFile
	#define Open gzopen
	#define Close gzclose
	#define Seek gzseek
	#define Tell gztell
#else
	#define FileType FILE*
	#define Open fopen
	#define Close fclose
	#define Seek fseek
	#define Tell ftell
#endif

#pragma comment (lib, "comctl32.lib")

typedef std::vector<std::string> StringList;
typedef std::vector<struct ModListInfo*> ModList;

std::string EM3InstallDir;
ModList TopLayerMods;
HWND Dialog = NULL, Progress = NULL;

struct ModInfo
{
	std::string Name;
	std::string Author;
	std::string Comment;
};

struct FileEntry
{
	std::string Fullpath;
	std::string Name;
	int DataOffset;
	int DataSize;
};

struct ModContents
{
	std::string Name;
	std::list<FileEntry*> Files;
	std::list<ModContents*> SubFolders;
};

struct ModListInfo
{
	ModInfo Info;
	std::string Path;
	std::string InfoFile;
	ModContents Contents;
};

struct ModPackageHeader
{
	char ID[5];		// E3MP
	int Version;	// 0x00000101	
};

bool InitMods();

class CComputerCheckSum
{
public:
	CComputerCheckSum()
	{
		for (int i = 0; i < 4; i++)
			mSeeds[i] = 0;
	}
	void AddSeed(DWORD value)
	{
		mSeeds[0] += value;
		GetUL();
	}
	void AddSeed(char* string)
	{
		while (*string)
		{
			mSeeds[0] += *string;
			string++;
			GetUL();
		}
	}
	unsigned long GetComputerChecksum()
	{
		// Checksumme errechnet sich aus CPUID
		DWORD word1 = 0, word2 = 0, word3 = 0, word4 = 0;
		_asm 
		{	
			push ebx
			push ecx
			push edx
			// get the vendor string
			xor eax, eax
			cpuid
			mov word1, eax
			mov word2, ebx
			mov word3, edx
			mov word4, ecx
			pop ecx
			pop ebx
			pop edx
		}
		// Get Computer Name
		DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
		char buffer[MAX_COMPUTERNAME_LENGTH + 1];
		GetComputerName(buffer,&size);
		
		AddSeed(word1);
		AddSeed(word2);
		AddSeed(word3);
		AddSeed(word4);
		AddSeed(buffer);
		return GetUL();
	}
	unsigned long GetUL()
	{
		unsigned long r = 0xcb72b0f5;
		r += 0x185d9e6d * (mSeeds[0] ^ 0xc7b347cf);
		mSeeds[0] = mSeeds[1];
		r += 0x019e7f31 * (mSeeds[1] ^ 0x37cacc44);
		mSeeds[1] = mSeeds[2];
		r += 0xe724da75 * (mSeeds[2] ^ 0xc37bd473);
		mSeeds[2] = mSeeds[3];
		r += 0xd61ca29d * (mSeeds[3] ^ 0x1dc1b026);
		r = (r << 1) | (r >> 31);
		mSeeds[3] = r;
		return r;
	}

private:
	unsigned long mSeeds[4];
};


int Write(FileType f, const void *data, int size)
{
#ifdef COMPRESS_PACKAGE
	return gzwrite(f, data, size);
#else
	return fwrite(data, 1, size, f);
#endif
}

int Read(FileType f, void *data, int size)
{
#ifdef COMPRESS_PACKAGE
	return gzread(f, data, size);
#else
	return fread(data, 1, size, f);
#endif
}

int Rewind(FileType f)
{
#ifdef COMPRESS_PACKAGE
	return gzrewind(f);
#else
	rewind(f);
	return 1;
#endif
}

bool GetInstallDir()
{

	static char instdir[MAX_PATH];
	int len = MAX_PATH;

	// read registry
	HKEY key;
#ifndef EM4_DELUXE
	if (RegOpenKeyEx(HKEY_CURRENT_USER,"SOFTWARE\\sixteen tons entertainment\\Emergency 4",0,KEY_READ,&key) != ERROR_SUCCESS)		
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,"SOFTWARE\\sixteen tons entertainment\\Emergency 4",0,KEY_READ,&key) != ERROR_SUCCESS)
#else
	if (RegOpenKeyEx(HKEY_CURRENT_USER,"SOFTWARE\\sixteen tons entertainment\\Emergency 4 Deluxe",0,KEY_READ,&key) != ERROR_SUCCESS)		
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,"SOFTWARE\\sixteen tons entertainment\\Emergency 4 Deluxe",0,KEY_READ,&key) != ERROR_SUCCESS)
#endif
			return false;
	long ret = RegQueryValueEx(key,"InstallDir",NULL,NULL,reinterpret_cast<BYTE *>(instdir),reinterpret_cast<LPDWORD>(&len));
	RegCloseKey(key);
	if (ret == ERROR_SUCCESS)
	{
		EM3InstallDir = instdir;
		return true;

	}

#ifdef DEBUG
	EM3InstallDir = "D:\\Emergency4\\Em4Game";
	return true;
#else
	return false;
#endif
}

std::string ChooseDestinationPackage()
{
	TCHAR fn[_MAX_PATH];
	fn[0]=0;
	OPENFILENAME ofn;
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = Dialog;
	ofn.hInstance = GetModuleHandle(NULL);
	ofn.lpstrFilter = "Emergency 4 mod packages\0*.e4mod\0\0";
	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxCustFilter = 0;
	ofn.nFilterIndex = 0;
	ofn.lpstrFile = fn;
	ofn.nMaxFile = _MAX_PATH;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = ".";
	ofn.lpstrTitle = "Create package";
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
	ofn.lpstrDefExt = "e4mod";
	if(GetSaveFileName(&ofn))
		return fn;
	
	return "";
}

bool ScanSubFolder(ModContents *Target, const std::string &Path)
{
	assert(Target);
	if(!Target)
		return false;
	
	std::string Pattern = Path + "\\*.*";	
	WIN32_FIND_DATA fd;
	HANDLE h = FindFirstFile(Pattern.c_str(), &fd);
	if(h != INVALID_HANDLE_VALUE)
	{
		do 
		{
			if(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if(_stricmp(fd.cFileName, ".") && _stricmp(fd.cFileName, ".."))
				{
					ModContents *con = new ModContents;
					con->Name = fd.cFileName;
					Target->SubFolders.push_back(con);
					ScanSubFolder(con, Path + "\\" + fd.cFileName);
				}
			} else
			{
				FileEntry *e = new FileEntry;
				e->Fullpath = Path + "\\" + fd.cFileName;
				e->Name = fd.cFileName;
				e->DataOffset = e->DataSize = 0;
				Target->Files.push_back(e);
			}
		}
		while(FindNextFile(h, &fd));
		FindClose(h);
	}
	
	return true;
}

bool ScanModContents(int Item)
{
	ModListInfo *mli = 0;
	LPARAM value = SendMessage(GetDlgItem(Dialog, IDC_MODLIST), LB_GETITEMDATA, (WPARAM)Item, 0);
	if(value != LB_ERR)
		mli = reinterpret_cast<ModListInfo*>(value);

	assert(mli);
	if(!mli)
		return false;
		
	WIN32_FIND_DATA fd;
	std::string pattern = mli->Path + "\\*.*";
	HANDLE h = FindFirstFile(pattern.c_str(), &fd);
	if(h != INVALID_HANDLE_VALUE)
	{
		do 
		{
			if(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if(_stricmp(fd.cFileName, ".") && _stricmp(fd.cFileName, ".."))
				{
					ModContents *con = new ModContents;
					con->Name = fd.cFileName;
					mli->Contents.SubFolders.push_back(con);
					ScanSubFolder(con, mli->Path + "\\" + fd.cFileName);
				}
			} else
			{
				FileEntry *e = new FileEntry;
				e->Name = fd.cFileName;
				e->DataOffset = e->DataSize = 0;
				e->Fullpath = mli->Path + "\\" + fd.cFileName;
				mli->Contents.Files.push_back(e);
			}
		}
		while(FindNextFile(h, &fd));
		FindClose(h);
	}
	
	return true;
}

bool ReadModInfo(ModListInfo *mli)
{
	assert(mli);
	TiXmlDocument doc(mli->InfoFile.c_str());
	if(!doc.LoadFile())
		return false;
	
	TiXmlElement *root = doc.RootElement();
	if(!root)
		return false;
	
	TiXmlElement *info = root->FirstChildElement("mod");
	if(info)
	{		
		const wchar_t *n = info->Attribute("name");
		const wchar_t *a = info->Attribute("author");
		const wchar_t *c = info->Attribute("comment");
		
		static char temp[0xffff];
		WideCharToMultiByte(CP_ACP, 0, n, -1, temp, 0xffff, NULL, NULL);
		mli->Info.Name = temp;
		WideCharToMultiByte(CP_ACP, 0, a, -1, temp, 0xffff, NULL, NULL);
		mli->Info.Author = temp;
		WideCharToMultiByte(CP_ACP, 0, c, -1, temp, 0xffff, NULL, NULL);
		mli->Info.Comment = temp;
		/*
		mli->Info.Name = info->Attribute("name");;		
		mli->Info.Author = info->Attribute("author");		
		mli->Info.Comment = info->Attribute("comment");
		*/
	}
	
	return true;
}

void UpdateModInfoDisplay(int Item)
{
	LPARAM value = SendMessage(GetDlgItem(Dialog, IDC_MODLIST), LB_GETITEMDATA, (WPARAM)Item, 0);
	if(value != LB_ERR)
	{
		ModListInfo *mli = reinterpret_cast<ModListInfo*>(value);
		assert(mli);
		if(mli)
		{
			std::string str = mli->Info.Name + "\r\nAuthor: "+mli->Info.Author+"\r\n\r\n"+mli->Info.Comment;
			#ifdef DEBUG
				str += "\r\n";
				str += mli->Path;
			#endif
			SendMessage(GetDlgItem(Dialog, IDC_MODINFO), WM_SETTEXT, 0, (LPARAM)str.c_str());
		}
	}
}

bool CopyFiles(FileType File, ModContents *Node)
{
	assert(File);
	assert(Node);
	
	MSG msg;
	if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		if(!IsDialogMessage(Dialog, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}		

	for(std::list<FileEntry*>::iterator i = Node->Files.begin(); i != Node->Files.end(); i++)
	{
		FILE* input = fopen((*i)->Fullpath.c_str(), "rb");
		if(!input)
			return false;
		
		int Offs = Tell(File);
		fseek(input, 0, SEEK_END);
		int Size = ftell(input);
		fseek(input, 0, SEEK_SET);

		(*i)->DataOffset = Offs;
		(*i)->DataSize = Size;

		static unsigned char buffer[0xffff];
		static unsigned char compbuffer[0x12000];
		int r = fread(buffer, 1, 0xffff, input);
		do 
		{
			uLongf sc = 0x12000;
			if(compress(compbuffer, &sc, buffer, r) != Z_OK)
			{
				MessageBox(Dialog, "Error while compressing input data", "Fatal error", MB_OK | MB_ICONSTOP);
				return false;
			}
			
			//int w = Write(File, buffer, r);
			Write(File, &sc, sizeof(uLongf));	// compressed size
			Write(File, &r, sizeof(int));		// uncompressed size
			int w = Write(File, compbuffer, sc);
			//if(w != r)
			if(w != sc)
				return false;
			r = fread(buffer, 1, 0xffff, input);
		} while(r > 0);
		
		fclose(input);
	}
	
	for(std::list<ModContents*>::iterator i = Node->SubFolders.begin(); i != Node->SubFolders.end(); i++)
		CopyFiles(File, (*i));
		
	return true;
}

void WritePackageInfo(FileType File, ModContents *Node)
{
	assert(File);
	assert(Node);
	int l = Node->Name.length()+1;
	Write(File, &l, sizeof(int));
	Write(File, Node->Name.c_str(), l);
	
	int NumFolders = Node->SubFolders.size();
	Write(File, &NumFolders, sizeof(int));
	for(std::list<ModContents*>::iterator i = Node->SubFolders.begin(); i != Node->SubFolders.end(); i++)
	{
		int l = (*i)->Name.length()+1;
		Write(File, &l, sizeof(int));
		Write(File, (*i)->Name.c_str(), l);
	}
	int NumFiles = Node->Files.size();
	Write(File, &NumFiles, sizeof(int));
	for(std::list<FileEntry*>::iterator i = Node->Files.begin(); i != Node->Files.end(); i++)
	{
		int l = (*i)->Name.length()+1;
		Write(File, &l, sizeof(int));
		Write(File, (*i)->Name.c_str(), l);
		Write(File, &(*i)->DataOffset, sizeof(int));
		Write(File, &(*i)->DataSize, sizeof(int));
	}

	for(std::list<ModContents*>::iterator i = Node->SubFolders.begin(); i != Node->SubFolders.end(); i++)
		WritePackageInfo(File, *i);
}

bool UnpackFiles(FileType f, std::vector<FileEntry*> &Files)
{
	for(std::vector<FileEntry*>::iterator i = Files.begin(); i != Files.end(); i++)
	{
		MSG msg;
		if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if(!IsDialogMessage(Dialog, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}		
		Rewind(f);
		FileEntry *e = *i;
		Seek(f, e->DataOffset, SEEK_SET);
		
		FILE *out = fopen(e->Fullpath.c_str(), "wb");
		if(!out)
			return false;
		
		static unsigned char compbuffer[0x12000];
		unsigned char *uncompbuffer = NULL;
		
		uLongf compsize;
		int uncompsize;
		Read(f, &compsize, sizeof(uLongf));
		Read(f, &uncompsize, sizeof(int));

		int todo = e->DataSize;
		int toread = compsize;
		int r = Read(f, compbuffer, toread);
		do 
		{
			if(uncompsize > 0)
			{
				uncompbuffer = new unsigned char[uncompsize];
				uLongf decompsize = uncompsize;
				int Result = uncompress(uncompbuffer, &decompsize, compbuffer, r);
				assert(decompsize==uncompsize);
				if(Result!=Z_OK)
				{
					MessageBox(Dialog, "Error while decompressing data", "Fatal error", MB_OK | MB_ICONSTOP);
					return false;
				}
				int w = fwrite(uncompbuffer, 1, decompsize, out);
				delete [] uncompbuffer;
				assert(w == decompsize);
				if(w != decompsize)
					return false;
				
				todo -= decompsize;
				r = toread = 0;
				if(todo > 0)
				{
					Read(f, &compsize, sizeof(uLongf));
					Read(f, &uncompsize, sizeof(int));
					toread = compsize;
					if(toread > 0)
						r = Read(f, compbuffer, toread);
				}
									
				assert(r == toread);
				assert((r!=0 && todo!=0)||(r==0&&todo==0));
			}
		} while(todo > 0 && r > 0);
		fclose(out);
		
		delete e;
	}
	
	return true;
}

bool CreateStructure(FileType f, const std::string &MyName, const std::string &InstallPath, std::vector<FileEntry*> &Files)
{
	std::vector<std::string> FolderList;
	
	int NumFolders;
	Read(f, &NumFolders, sizeof(int));
	for(int i=0; i<NumFolders; i++)
	{
		int l = 0;
		Read(f, &l, sizeof(int));
		char *temp = new char[l];
		Read(f, temp, l);
		std::string tpath = InstallPath + "\\" + temp;
		int cd = CreateDirectory(tpath.c_str(), NULL);
		assert(cd != 0);
		FolderList.push_back(temp);
		delete [] temp;
	}

	int NumFiles;
	Read(f, &NumFiles, sizeof(int));
	for(int i=0; i<NumFiles; i++)
	{
		int l = 0;
		Read(f, &l, sizeof(int));
		char *temp = new char[l];
		Read(f, temp, l);
		int Offs, Size;
		Read(f, &Offs, sizeof(int));
		Read(f, &Size, sizeof(int));
		
		FileEntry *e = new FileEntry;
		e->DataOffset = Offs;
		e->DataSize = Size;
		e->Name = temp;
		e->Fullpath = InstallPath + "\\" + e->Name;
		Files.push_back(e);
		delete [] temp;
	}
	
	for(std::vector<std::string>::iterator i = FolderList.begin(); i != FolderList.end(); i++)
	{
		MSG msg;
		if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if(!IsDialogMessage(Dialog, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}		

		int l = 0;
		Read(f, &l, sizeof(int));
		if(l==0)
			return false;
		static char Name[MAX_PATH];
		Read(f, Name, l);
		std::string Path = InstallPath + "\\" + Name;
		SetCurrentDirectory(Path.c_str());
		CreateStructure(f, *i, Path, Files);
	}
	return true;
}

bool UnpackPackage(FileType f, const std::string &InstallPath)
{
	ModPackageHeader h;
	Read(f, &h.ID, 5);
	Read(f, &h.Version, sizeof(int));
	
	if(strcmp(h.ID, "E4MP"))
	{
		MessageBox(Dialog, "This is not a valid modification package.", "Corrupt file", MB_OK | MB_ICONSTOP);
		return false;
	}
	
	if(h.Version > FILEVERSION)
	{
		MessageBox(Dialog, "This modification package format is newer than the latest supported version. Please go to the Emergency 4 website to obtain an Emergency 4 program update.", "File too new", MB_OK | MB_ICONSTOP);
		return false;
	}
	
	int l = 0;
	Read(f, &l, sizeof(int));
	if(l==0)
		return false;
	static char MyName[MAX_PATH];
	Read(f, MyName, l);
	SetCurrentDirectory(InstallPath.c_str());
	if(SetCurrentDirectory(MyName)!=0)
	{
		MessageBox(Dialog, "There appears to be a modification with the same name installed. Can't install modification.\n\nRemove or rename the existent modification folder in order to install this modification package.", "Error", MB_OK | MB_ICONSTOP);
		return false;
	}
	
	int cd = CreateDirectory(MyName, NULL);
	assert(cd != 0);	
	int scd = SetCurrentDirectory(MyName);
	assert(scd != 0);
	
	std::string Outpath = InstallPath + "\\" + MyName;
	std::vector<FileEntry*> Files;
	CreateStructure(f, MyName, Outpath, Files);
	
	ShowWindow(Progress, SW_SHOW);
	if(!UnpackFiles(f, Files))
	{
		MessageBox(Dialog, "The mod package is corrupted", "Fatal Error", MB_OK | MB_ICONSTOP);
		return false;
	}
	ShowWindow(Progress, SW_HIDE);
	SetActiveWindow(Dialog);
	BringWindowToTop(Dialog);
	Files.clear();

	// Create Key File
	std::string keyFileName = InstallPath + "\\" + MyName + "\\e4mod.key" ;
	FILE* keyFile = fopen(keyFileName.c_str(), "wb");
	if (!keyFile)
	{
		MessageBox(Dialog, "Could not create key file. Aborting.", "Fatal Error", MB_OK | MB_ICONSTOP);
		return false;
	}

	CComputerCheckSum generator;
	unsigned long checkSum = generator.GetComputerChecksum();
	fwrite(&checkSum,1,sizeof(checkSum),keyFile);
	fclose(keyFile);
	return true;
}

bool MakePackage(int Item)
{
	LPARAM value = SendMessage(GetDlgItem(Dialog, IDC_MODLIST), LB_GETITEMDATA, (WPARAM)Item, 0);
	if(value == LB_ERR)
		return false;

	ModListInfo *mli = reinterpret_cast<ModListInfo*>(value);
	assert(mli);
	if(!mli)
		return false;
		
	std::string filename = ChooseDestinationPackage();
	if(filename.length()==0)
		return false;
	FileType f = Open(filename.c_str(), "wb");
	assert(f);
	if(!f)
		return false;
	
	ModPackageHeader h;
	strcpy(h.ID, "E4MP");
	h.ID[4]=0;
	h.Version = FILEVERSION;
	Write(f, h.ID, 5);
	Write(f, &h.Version, sizeof(int));
	
	EnableWindow(Dialog, FALSE);
	ShowWindow(Progress, SW_SHOW);
	static char defdir[1024];
	int r = mli->Path.find_last_of('\\', mli->Path.length())+1;
	int l = mli->Path.length() - r;
	mli->Contents.Name = mli->Path.substr(r, l);
	WritePackageInfo(f, &mli->Contents);
	int null = 0;
	Write(f, &null, sizeof(int));
	CopyFiles(f, &mli->Contents);
	Seek(f, 9, SEEK_SET);
	
	// nochmal, diesmal mit korrekten fileoffsets
	WritePackageInfo(f, &mli->Contents);
	ShowWindow(Progress, SW_HIDE);
	SetActiveWindow(Dialog);
	BringWindowToTop(Dialog);
	
	Close(f);
	EnableWindow(Dialog, TRUE);
	return true;
}

bool UnInstall(int Item)
{
	LPARAM value = SendMessage(GetDlgItem(Dialog, IDC_MODLIST), LB_GETITEMDATA, (WPARAM)Item, 0);
	if(value == LB_ERR)
		return false;
	
	ModListInfo *mli = reinterpret_cast<ModListInfo*>(value);
	assert(mli);
	if(!mli)
		return false;
	
	static char message[2048];
	sprintf(message, "Sure to uninstall the modification '%s'? This will remove the entire modification, including saved games!", mli->Info.Name.c_str());
	if(MessageBox(Dialog, message, "Uninstall modification", MB_YESNO | MB_ICONQUESTION) == IDYES)
	{
		EnableWindow(Dialog, FALSE);
		SHFILEOPSTRUCT fos;
		fos.hwnd = Dialog;
		fos.wFunc = FO_DELETE;
		
		static char filebuffer[MAX_PATH+2];
		sprintf(filebuffer, "%s\0\0", mli->Path.c_str());
		fos.pFrom = filebuffer;
		fos.pTo = 0;
		fos.fFlags = FOF_SIMPLEPROGRESS | FOF_NOCONFIRMATION;
		fos.hNameMappings = 0;
		fos.lpszProgressTitle = "Uninstalling";
		
		if(SHFileOperation(&fos)==0)
			MessageBox(Dialog, "Modification successfully uninstalled", "Completed", MB_OK | MB_ICONINFORMATION);
		else
			MessageBox(Dialog, "Error during uninstall", "Error", MB_OK | MB_ICONSTOP);

		EnableWindow(Dialog, TRUE);
	}
		
	return true;
}


std::string ChoosePackage()
{
	TCHAR fn[_MAX_PATH];
	fn[0]=0;
	OPENFILENAME ofn;
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = Dialog;
	ofn.hInstance = GetModuleHandle(NULL);
	ofn.lpstrFilter = "Emergency 4 mod packages\0*.e4mod\0\0";
	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxCustFilter = 0;
	ofn.nFilterIndex = 0;
	ofn.lpstrFile = fn;
	ofn.nMaxFile = _MAX_PATH;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = ".";
	ofn.lpstrTitle = "Select package";
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = "e4mod";
	if(GetOpenFileName(&ofn))
		return fn;
	
	return "";
}


bool InstallPackage(const std::string &Path)
{
	std::string LocalPath = Path;
	if(LocalPath.find('"', 0)==0)
		LocalPath = Path.substr(1, Path.length()-2);
		
	FileType f = Open(LocalPath.c_str(), "rb");
	if(!f)
	{
		MessageBox(Dialog, "Could not open source file. Aborting.", "Fatal Error", MB_OK | MB_ICONSTOP);
		return false;
	}
	
	EnableWindow(Dialog, FALSE);
	std::string dest = EM3InstallDir + "\\mods";
	bool Result = UnpackPackage(f, dest);
	EnableWindow(Dialog, TRUE);
	Close(f);
	return Result;
}

BOOL CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_INITDIALOG :
		{
			HICON hIcon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
			if(hIcon)
			{
				SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
				SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
			}
			return TRUE;
		}

		case WM_CLOSE :
		{
			PostQuitMessage(0);
			return TRUE;
		}
		
		case WM_COMMAND :
		{
			switch(LOWORD(wParam))
			{
				case IDC_MODLIST :
				{
					if(HIWORD(wParam)==LBN_SELCHANGE)
					{
						int SelItem = SendMessage(GetDlgItem(Dialog, IDC_MODLIST), LB_GETCURSEL, 0, 0);
						EnableWindow(GetDlgItem(Dialog, IDC_MAKE), TRUE);
						EnableWindow(GetDlgItem(Dialog, IDC_UNINSTALL), TRUE);
						if(SelItem != LB_ERR)
						{
							UpdateModInfoDisplay(SelItem);
						}						
						return FALSE;
					}
					break;
				}
				
				case IDC_EXIT :
				{
					if(HIWORD(wParam)==BN_CLICKED)
					{
						PostQuitMessage(0);
						return FALSE;
					}
					
					break;
				}
				
				case IDC_MAKE :
				{
					if(HIWORD(wParam)==BN_CLICKED)
					{
						int SelItem = SendMessage(GetDlgItem(Dialog, IDC_MODLIST), LB_GETCURSEL, 0, 0);
						if(SelItem != LB_ERR)
						{
							ScanModContents(SelItem);
							if(MakePackage(SelItem))
								MessageBox(Dialog, "Package successfully created", "Operation completed", MB_OK | MB_ICONINFORMATION);
							else
								MessageBox(Dialog, "Could not create package", "Operation failed", MB_OK | MB_ICONSTOP);
							InitMods();
						}
						
						return FALSE;
					}
					break;
				}
				
				case IDC_UNINSTALL :
				{
					if(HIWORD(wParam)==BN_CLICKED)
					{
						int SelItem = SendMessage(GetDlgItem(Dialog, IDC_MODLIST), LB_GETCURSEL, 0, 0);
						if(SelItem != LB_ERR)
						{
							UnInstall(SelItem);
						}
						InitMods();
						return FALSE;
					}
					break;
				}
				
				case IDC_INSTALL :
				{
					if(HIWORD(wParam)==BN_CLICKED)
					{
						std::string pack = ChoosePackage();
						if(pack!="")
						{
							if(InstallPackage(pack))
								MessageBox(Dialog, "Package successfully installed", "Success", MB_OK | MB_ICONINFORMATION);
							InitMods();
						}
						return FALSE;
					}
					break;
				}
			}
			
			break;
		}
	}
	return FALSE;
}

bool InitDialog()
{
	InitCommonControls();
	Dialog = CreateDialog(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_MAIN), NULL, DialogProc);

	if(!Dialog)
	{
		MessageBox(NULL, "Could not create the application window", "Fatal Error", MB_OK | MB_ICONSTOP);
		return false;
	}
	
	Progress = CreateDialog(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_PROGRESS), NULL, DialogProc);
	SetActiveWindow(Dialog);
	return true;
}

void UnInitContents(ModContents *Contents)
{
	for(std::list<ModContents*>::iterator i = Contents->SubFolders.begin(); i != Contents->SubFolders.end(); i++)
	{
		UnInitContents(*i);
		delete (*i);
	}
	for(std::list<FileEntry*>::iterator i = Contents->Files.begin(); i != Contents->Files.end(); i++)
		delete (*i);
	Contents->SubFolders.clear();
	Contents->Files.clear();
}

void UnInitMods()
{
	for(ModList::const_iterator i = TopLayerMods.begin(); i != TopLayerMods.end(); i++)
	{
		ModContents &con = (*i)->Contents;
		UnInitContents(&con);
		delete (*i);
	}
	TopLayerMods.clear();
	SendMessage(GetDlgItem(Dialog, IDC_MODLIST), LB_RESETCONTENT, 0, 0);
}

void ScanForMods(ModList &Mods)
{
	std::string Path = EM3InstallDir+"\\"+"Mods\\*.*";
	WIN32_FIND_DATA fd;
	HANDLE h = FindFirstFile(Path.c_str(), &fd);
	if(h != INVALID_HANDLE_VALUE)
	{
		do 
		{
			if(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if(_stricmp(fd.cFileName, ".") && _stricmp(fd.cFileName, ".."))
				{
					std::string ModPath = EM3InstallDir+"\\Mods\\"+std::string(fd.cFileName);
					std::string teststr = EM3InstallDir+"\\Mods\\"+std::string(fd.cFileName) + "\\e4mod.info";
					std::FILE *f = std::fopen(teststr.c_str(), "rb");
					if(f)
					{
						std::fclose(f);
						ModListInfo *mli = new ModListInfo;
						mli->Path = ModPath;
						mli->InfoFile = teststr;
						if(ReadModInfo(mli))
							Mods.push_back(mli);
						else
							delete mli;
					}
				}
			}
		}
		while(FindNextFile(h, &fd));
		FindClose(h);
	}	
}

bool InitMods()
{
	UnInitMods();
	ScanForMods(TopLayerMods);
	for(ModList::const_iterator i = TopLayerMods.begin(); i != TopLayerMods.end(); i++)
	{		
		int item = SendMessage(GetDlgItem(Dialog, IDC_MODLIST), LB_ADDSTRING, 0, (LPARAM)(*i)->Info.Name.c_str());
		SendMessage(GetDlgItem(Dialog, IDC_MODLIST), LB_SETITEMDATA, item, (LPARAM)(*i));
	}
	
	return true;
}

int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	if(!GetInstallDir())
	{
#ifndef EM4_DELUXE
		MessageBox(NULL, "Emergency 4 is not properly installed. Please reinstall.", "Error", MB_OK | MB_ICONSTOP);
#else
		MessageBox(NULL, "Emergency 4 Deluxe is not properly installed. Please reinstall.", "Error", MB_OK | MB_ICONSTOP);
#endif
		return -1;
	}
	if(!InitDialog())
		return -1;
	if(!InitMods())
		return -1;
	
	std::string AutoInstall = lpCmdLine;
	if(AutoInstall.length() > 0)
	{
		static char str[2048];
		sprintf(str, "This will install the modification from package %s to %s\\Mods.\n\nContinue?", lpCmdLine, EM3InstallDir.c_str());
		if(MessageBox(Dialog, str, "Install package?", MB_YESNO | MB_ICONQUESTION)==IDYES)
		{
			InstallPackage(lpCmdLine);
			InitMods();
			SetActiveWindow(Dialog);
			BringWindowToTop(Dialog);
		}
	}
	
	MSG msg;
	while(GetMessage(&msg, NULL, 0, 0))
	{
		if(!IsDialogMessage(Dialog, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	EndDialog(Dialog, 0);
	UnInitMods();
	return 0;
}
