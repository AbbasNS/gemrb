/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/PluginMgr.cpp,v 1.16 2005/05/14 11:18:08 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "stdio.h"
#include "stdlib.h"
#include "PluginMgr.h"
#include "Plugin.h"
#include "Interface.h"

#ifdef WIN32
#include <io.h>
#include <windows.h>
#else
#include <sys/types.h>
#include <dirent.h>
#include <fnmatch.h>
#include <dlfcn.h>
#endif

typedef char*(* charvoid)(void);
typedef ClassDesc*(* cdint)(void);

PluginMgr::PluginMgr(char* pluginpath)
{
	printMessage( "PluginMgr", "Loading Plugins...\n", WHITE );
	char path[_MAX_PATH];
	strcpy( path, pluginpath );
	/*
	 Since Win32 and Linux uses two different methods for loading external Dynamic Link Libraries,
	 we have to take this in consideration using some #ifdef statements.
	 Win32 uses the _findfirst/_findnext functions for iterating through all the files in the
	 plugin directory.
	 Linux simply uses a readdir function to walk through the directory and a fnmatch function to
	 search files with ".so" extension.
	*/
#ifdef WIN32
	//The windows _findfirst/_findnext functions allow the use of wildcards so we'll use them :)
	struct _finddata_t c_file;
	long hFile;
	strcat( path, "*.dll" );
	printMessage( "PluginMgr", "Searching for plugins in: ", WHITE );
	printf( "%s\n", path );
	if (( hFile = ( long ) _findfirst( path, &c_file ) ) == -1L) //If there is no file matching our search
		#else
	{
		printMessage( "PluginMgr", "Searching for plugins in: ", WHITE );
	}
	printf( "%s\n", path );
	DIR* dir = opendir( path );
	if (dir == NULL) //If we cannot open the Directory
		#endif
	{
		return;
	} //Simply return
#ifndef WIN32  //Linux Statement
	struct dirent* de = readdir( dir );  //Lookup the first entry in the Directory
	if (de == NULL) //If no entry exists just return
	{
		return;
	}
#endif
	charvoid LibVersion;
	charvoid LibDescription;
	cdint LibClassDesc;
	do {
		//Iterate through all the available modules to load
#ifdef WIN32
		strcpy( path, pluginpath );
		//strcat(path, "plugins\\");
		strcat( path, c_file.name );
		printBracket( "PluginMgr", LIGHT_WHITE );
		printf( ": Loading: " );
		textcolor( LIGHT_WHITE );
		printf( "%s", c_file.name );
		textcolor( WHITE );
		printf( "..." );
		HMODULE hMod = LoadLibrary( path ); //Try to load the Module
		if (hMod == NULL) {
			printBracket( "ERROR", LIGHT_RED );
			printf( "\nCannot Load Module, Skipping...\n" );
			continue;
		}
		printStatus( "OK", LIGHT_GREEN );
		LibVersion = ( charvoid ) GetProcAddress( hMod, "LibVersion" );
		LibDescription = ( charvoid )
			GetProcAddress( hMod, "LibDescription" );
		LibClassDesc = ( cdint ) GetProcAddress( hMod, "LibClassDesc" );
#else
		if (fnmatch( "*.so", de->d_name, 0 ) != 0) //If the current file has no ".so" extension, skip it
			continue;
		strcpy( path, pluginpath );
		strcat( path, de->d_name );
		printBracket( "PluginMgr", LIGHT_WHITE );
		printf( ": Loading: " );
		textcolor( LIGHT_WHITE );
		printf( "%s", path );
		textcolor( WHITE );
		printf( "..." );
		void* hMod = dlopen( path, RTLD_NOW ); //Try to load the Module
		if (hMod == NULL) {
			printBracket( "ERROR", LIGHT_RED );
			printf( "\nCannot Load Module, Skipping...\n%s\n", dlerror() );
			continue;
		}
		printStatus( "OK", LIGHT_GREEN );
		/*
		GCC Version 3.2.x has changed the Export Names of the DLLs this statement is a simple
		hack to make GemRB run on every version.
		*/
#ifdef GCC_OLD
		LibVersion = ( charvoid ) dlsym( hMod, "LibVersion__Fv" );
		LibDescription = ( charvoid ) dlsym( hMod, "LibDescription__Fv" );
		LibClassDesc = ( cdint ) dlsym( hMod, "LibClassDesc__Fv" );
#else
		LibVersion = ( charvoid ) dlsym( hMod, "_Z10LibVersionv" );
		LibDescription = ( charvoid ) dlsym( hMod, "_Z14LibDescriptionv" );
		LibClassDesc = ( cdint ) dlsym( hMod, "_Z12LibClassDescv" );
#endif
#endif
		printMessage( "PluginMgr", "Checking Plugin Version...", WHITE );
		if (strcmp(LibVersion(), VERSION_GEMRB) ) {
			printStatus( "ERROR", LIGHT_RED );
			printf( "Plug-in Version not valid, Skipping...\n" );
#ifdef WIN32
			FreeLibrary(hMod);
#else
			dlclose(hMod);
#endif
			continue;
		}
		printStatus( "OK", LIGHT_GREEN );
		printMessage( "PluginMgr", "Loading Exports for ", WHITE );
		textcolor( LIGHT_WHITE );
		printf( "%s", LibDescription() );
		textcolor( WHITE );
		printf( "..." );
		printStatus( "OK", LIGHT_GREEN );
		bool error = false;
		ClassDesc* plug = LibClassDesc();
		if (plug == NULL) {
			printStatus( "ERROR", LIGHT_RED );
			printf( "Plug-in Exports Error\n" );
		} else {
			for (unsigned int x = 0; x < plugins.size(); x++) {
				if (plugins[x]->ClassID() == plug->ClassID()) {
					printStatus( "SKIPPING", YELLOW );
					printf( "Plug-in Already Loaded\n" );
					error = true;
					break;
				}
				if (plugins[x]->SuperClassID() == plug->SuperClassID()) {
					printStatus( "SKIPPING", YELLOW );
					printf( "Duplicate Plug-in\n" );
					error = true;
					break;
				}
			}
			if (error) {
#ifdef WIN32
				FreeLibrary(hMod);
#else
				dlclose(hMod);
#endif
				continue;
			}
			plugins.push_back( plug );
			libs.push_back( hMod );
		}
#ifdef WIN32 //Win32 while condition uses the _findnext function
	} while (_findnext( hFile, &c_file ) == 0);
	_findclose( hFile );
#else //Linux uses the readdir function
} while (( de = readdir( dir ) ) != NULL);
closedir(dir); //No other files in the directory, close it
#endif
}

PluginMgr::~PluginMgr(void)
{
//don't free the shared libraries in debug mode, so valgrind can resolve the stack trace
#ifndef _DEBUG
	for (unsigned int i = 0; i < libs.size(); i++) {
#ifdef WIN32
		FreeLibrary(libs[i]);
#else
		dlclose(libs[i]);
#endif
	}
#endif
}

bool PluginMgr::IsAvailable(SClass_ID plugintype)
{
	for (unsigned int i = 0; i < plugins.size(); i++) {
		if (plugins[i]->SuperClassID() == plugintype) {
			return true;
		}
	}
	return false;
}

void* PluginMgr::GetPlugin(SClass_ID plugintype)
{
	int plugs = plugins.size();
	while (plugs--) {
		if (plugins[plugs]->SuperClassID() == plugintype) {
			return ( plugins[plugs] )->Create();
		}
	}
	return NULL;
}

void PluginMgr::FreePlugin(void* ptr)
{
	( ( Plugin * ) ptr )->release();
}
