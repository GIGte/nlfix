/*
**	Copyright (c) 2015 GIG <bigig@live.ru>
*/

#pragma warning(disable: 4996)

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
	#include <Windows.h>
#endif


#ifdef _WIN32
#define DIRSEP '\\'
#define DIRSEP_STR "\\"
#else//if defined(__unix__)
#define DIRSEP '/'
#define DIRSEP_STR "/"
#endif

#define NLF_MAXFILENAME 128
#define NLF_MAXPATH 512
#define NLF_BUFSIZE 16384

#define CR '\r'
#define LF '\n'

#define MODE_UNIX	(1)
#define MODE_DOS	(2)
#define MODE_MAC	(3)

bool	SearchFiles;

int		Mode;
bool	SkipErrors	= false;
bool	ShowLogs = true;

const char	*searchpath;
const char	*extensions;

HANDLE	tempfile;
char	tempfile_name[MAX_PATH];


bool findFlag(const char *name, int argc, char *const argv[])
{
	while (--argc > 0)
	{
		if (strcmp(argv[argc], name) == 0)
		{
			return true;
		}
	}

	return false;
}

const char *findParam(const char *name, int argc, char *const argv[])
{
	while (--argc > 1)
	{
		if (strcmp(argv[argc - 1], name) == 0)
		{
			return argv[argc];
		}
	}

	return NULL;
}

int error(const char *errormsg)
{
	fprintf(stderr, "%s\n", errormsg);
	return 1;
}


typedef struct stackentry
{
	char name[NLF_MAXFILENAME];
	bool first_in_level;//stackentry *parent;
	stackentry *prev;
} stackentry, *dir;

typedef struct SimpleStack
{
	// TODO: dynamic array instead of linked list

	stackentry *top = NULL;
	//int length = 0;

	/*stackentry *top()
	{
		return data + length;
	}*/

	void push(stackentry *entry)
	{
		entry->prev = top;
		top = entry;
		//length++;
	}

	void pop()
	{
		stackentry *entry = top;
		top = entry->prev;
		delete entry;
		//length--;
	}
	void pop_all()
	{
		while (top != NULL)//length >= 0)
			pop();
	}
} SimpleStack;

stackentry *new_entry(const char *name, bool isfirst)//stackentry *parent)
{
	stackentry *entry = new stackentry();
	strncpy(entry->name, name, NLF_MAXFILENAME);//entry->name = name;
	entry->first_in_level = isfirst;//entry->parent = parent;
	return entry;
}


int processArgs(int argc, char *argv[])
{
	searchpath = argv[1];

	SearchFiles = argc < 3 ? false : argv[2][0] != '-';

	if (SearchFiles)
	{
		extensions = argv[2];
	}
	else
	{

	}

	const char *param;

	{
		param = findParam("-mode", argc, argv);

		if (param == NULL)
		{
			const char *program_name = strrchr(argv[0], DIRSEP) + 1;

			if (memcmp(program_name, "tounix", 6) == 0)
				Mode = MODE_UNIX;
			else if (memcmp(program_name, "todos", 5) == 0)
				Mode = MODE_DOS;
			else if (memcmp(program_name, "tomac", 5) == 0)
				Mode = MODE_MAC;
			else
				return error("mode not specified");
		}
		else if (strcmp(param, "unix") == 0)
			Mode = MODE_UNIX;
		else if (strcmp(param, "dos") == 0)
			Mode = MODE_DOS;
		else if (strcmp(param, "mac") == 0)
			Mode = MODE_MAC;
		else
			return error("unknown mode");
	}

	SkipErrors = findFlag("-skiperrors", argc, argv);
	ShowLogs = !findFlag("-nolog", argc, argv);

	return 0;
}

bool createTempFile()
{
	char tempfile_path[MAX_PATH];

	int res;

	res = GetTempPathA(MAX_PATH, tempfile_path);

	if (res > MAX_PATH || res == 0)
		return false;

	res = GetTempFileNameA(tempfile_path, NULL, 0, tempfile_name);

	if (res == 0)
		return false;

	// FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE
	tempfile = CreateFileA(tempfile_name, GENERIC_WRITE, FILE_SHARE_READ,
		NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (tempfile == INVALID_HANDLE_VALUE)
		return false;

	return true;
}

void deleteTempFile()
{
	CloseHandle(tempfile);
	DeleteFileA(tempfile_name); // FILE_FLAG_DELETE_ON_CLOSE?
}

bool replaceFile(const char *topath, const char *frompath)
{
	/*if (!CloseHandle(tempfile))
		puts("fcl");

	BOOL res = ReplaceFileA(topath, frompath, NULL, 0, NULL, NULL);

	tempfile = CreateFileA(tempfile_name, GENERIC_WRITE, 0,
		NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);*/

	BOOL res = CopyFileA(frompath, topath, FALSE);

	return res != 0;
}

/*bool processFile(char *path)
{
	HANDLE hfile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hfile == INVALID_HANDLE_VALUE)
		return false;
	
	SetFilePointer(tempfile, 0, NULL, FILE_BEGIN);

	char c, lastc = '\0';

	while (true)
	{
		DWORD bytes;

		if (!ReadFile(hfile, &c, 1, &bytes, NULL))
		{
			CloseHandle(hfile);
			return false;
		}

		if (bytes == 0)
			break;

		if (c == LF && lastc == CR) // skip DOS
		{
			lastc = LF;
			continue;
		}

		lastc = c;

		BOOL res = true;
		
		if (Mode == MODE_UNIX)
		{
			if (c == CR) // replace DOS and MAC
				c = LF;
		}
		else if (Mode == MODE_DOS)
		{
			if (c == CR || c == LF) // replace UNIX and MAC
			{
				static char crlf[] = {CR, LF};
				res = WriteFile(tempfile, crlf, 2, &bytes, NULL);

				goto check_res;
			}
		}
		else //if (Mode == MODE_MAC)
		{
			if (c == LF) // replace UNIX
				c = CR;
		}

		res = WriteFile(tempfile, &c, 1, &bytes, NULL);

	check_res:
		if (!res)
		{
			CloseHandle(hfile);
			printf("%d\n", GetLastError());
			return false;
		}
	}

	SetEndOfFile(tempfile);

	CloseHandle(hfile);

	// TODO: check if replace isn't needed
	// (i.e. file already has the EOL format wanted)
	
	if (!replaceFile(path, tempfile_name))
		return false;

	return true;
}*/

bool flushBuffer(const char *buffer, DWORD size)
{
	DWORD bytes;

	if (!WriteFile(tempfile, buffer, size, &bytes, NULL))
	{
		printf("%d\n", GetLastError());
		return false;
	}
	
	return true;
}

bool writeBuffer(const char *inbuf, char *outbuf, DWORD size, DWORD *posp, bool isfinal)
{
	DWORD pos = *posp;

	char c, lastc = '\0';

	while (size-- > 0)
	{
		c = *inbuf++;

		if (c == LF && lastc == CR) // skip DOS
		{
			lastc = LF;
			continue;
		}

		lastc = c;

		if (Mode == MODE_UNIX)
		{
			if (c == CR) // replace DOS and MAC
				c = LF;
		}
		else if (Mode == MODE_DOS)
		{
			if (c == CR || c == LF) // replace UNIX and MAC
			{
				outbuf[pos++] = CR;
				c = LF;
			}
		}
		else //if (Mode == MODE_MAC)
		{
			if (c == LF) // replace UNIX
				c = CR;
		}

		if (pos == NLF_BUFSIZE)
		{
			if (!flushBuffer(outbuf, pos))
				return false;

			pos = 0;
		}

		outbuf[pos++] = c;
	}

	if (pos == NLF_BUFSIZE || (isfinal && pos != 0))
	{
		if (!flushBuffer(outbuf, pos))
			return false;

		pos = 0;
	}

	*posp = pos;

	return true;
}

bool processFile(const char *path)
{
	HANDLE hfile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hfile == INVALID_HANDLE_VALUE)
		return false;

	SetFilePointer(tempfile, 0, NULL, FILE_BEGIN);

	static char inbuf[NLF_BUFSIZE];
	static char outbuf[NLF_BUFSIZE];

	DWORD bytes;
	DWORD pos = 0;

	while (true)
	{
		if (!ReadFile(hfile, inbuf, NLF_BUFSIZE, &bytes, NULL))
			goto reading_failed;

		if (bytes < NLF_BUFSIZE)
			break;

		if (!writeBuffer(inbuf, outbuf, bytes, &pos, false))
			goto reading_failed;
	}

	if (bytes != 0 || pos != 0)
	{
		if (!writeBuffer(inbuf, outbuf, bytes, &pos, true))
			goto reading_failed;
	}

	/*if (SetFilePointer(hfile, 0, NULL, FILE_CURRENT)
		== SetFilePointer(tempfile, 0, NULL, FILE_CURRENT))
	{
		
	}*/

	SetEndOfFile(tempfile);

	CloseHandle(hfile);

	// TODO: check if replace isn't needed
	// (i.e. file already has the EOL format wanted)
	//
	// UPD: LF <-> CF check wasn't done

	if (Mode == MODE_DOS
		&& GetFileSize(hfile, NULL) == GetFileSize(tempfile, NULL))
	{
		//puts("(unchanged) ");
		return true;
	}

	if (!replaceFile(path, tempfile_name))
		return false;

	return true;

reading_failed:
	CloseHandle(hfile);

	return false;
}

bool checkExtension(const char *filename, const char *extensions)
{
	// TODO: masks instead of extensions

	const char *ext = strchr(filename, '.') + 1;

	if (ext == (char *)1) // no extension
		return false;

	int pos = 0;
	bool skip = false;

	do
	{
		if (skip)
		{
			if (*extensions == ';')
				skip = false;
		}
		else
		{
			if (ext[pos] == *extensions)
				pos++;
			else
			{
				if (ext[pos] == '\0' && *extensions == ';')
					return true;

				pos = 0;
				skip = true;
			}
		}
	}
	while (*++extensions != '\0');

	if (!skip && ext[pos] == '\0')
		return true;

	return false;
}

int searchRecursive(const char *root, const char *allowed_extensions)
{
	SimpleStack stack;

	stack.push(new_entry(root, false));

	WIN32_FIND_DATAA ffd;
	HANDLE hfind;

	char path[NLF_MAXPATH];
	char *path_w = path;

	while (stack.top != NULL)//stack.length != 0)
	{
		dir curdir = stack.top;

		// build a full path
		{
			// TODO: check path length!
			strcpy(path_w, curdir->name);
			strcat(path_w, DIRSEP_STR);
			strcat(path_w, "*");

			int namelen = strlen(curdir->name);
			path_w += namelen + 1;
		}

		// try to open the path
		hfind = FindFirstFileA(path, &ffd);

		if (hfind == INVALID_HANDLE_VALUE)
		{
			stack.pop_all();
			return error("failed to open directory");
		}

		// traverse the current dir
		do
		{
			if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
			{
				// do the job

				//printf("checking file %s, extensions: %s\n", ffd.cFileName, extensions);

				if (allowed_extensions[0] != '\0')
				{
					if (!checkExtension(ffd.cFileName, allowed_extensions))
						continue;
				}

				strcpy(path_w, ffd.cFileName);

				if (ShowLogs)
					printf("%s: ", path);

				if (!processFile(path))
				{
					if (ShowLogs)
						puts("error!");

					if (!SkipErrors)
					{
						stack.pop_all();
						return error("unable to open file");
					}
				}
				else if (ShowLogs)
					puts("done");
			}
			else if (strcmp(ffd.cFileName, ".") != 0
				&& strcmp(ffd.cFileName, "..") != 0)
			{
				dir newdir = new_entry(ffd.cFileName, stack.top == curdir);
				stack.push(newdir);
			}
		}
		while (FindNextFileA(hfind, &ffd) != 0);

		/*if (GetLastError() != ERROR_NO_MORE_FILES)
		{
			FindClose(hfind);
			return error("unknown reading error");
		}*/

		FindClose(hfind);

		if (stack.top == curdir)
		{
			path_w[-1] = '\0';
			path_w = strrchr(path, DIRSEP) + 1;

			while (stack.top->first_in_level)
			{
				path_w[-1] = '\0';
				path_w = strrchr(path, DIRSEP) + 1;
				stack.pop();
			}

			stack.pop();
		}
	}

	return 0;
}


int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		return error(
			"usage: [path] [extensions]\n"
			"\tOR [filename]\n"
			//"usage: [filenames]\n"
			//"\tOR [path] -ext [extensions]\n"
			"Available options are: \n"
			"  -mode         EOL mode (unix, dos, mac)\n"
			"  -skiperrors   skip any errors when processing files\n"
			"  -nolog        hide logs (can increase speed)"
			// TODO:
			//"  -depth        max directory depth (-1 by default)\n"
			//"  -b            make backups\n"
			//"  -bext         backup extension (\"bak\" by default)\n"
			//"  -bdir         backup directory\n"

			//"  -perf         output processing time\n"
		);
	}

	if (processArgs(argc, argv))
		return 1;

	if (!createTempFile())
		return error("failed to create a temporary file");

	int res = 0;

	if (SearchFiles)
	{
		res = searchRecursive(searchpath, extensions);
	}
	else
	{
		if (!processFile(searchpath))
			res = error("unable to open file");
	}

	deleteTempFile();

	return res;
}