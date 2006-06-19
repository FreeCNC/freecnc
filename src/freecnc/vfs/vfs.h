#ifndef _VFS_VFS_H
#define _VFS_VFS_H

#include "../freecnc.h"

class VFile;

void VFS_PreInit(const char* binpath);
void VFS_Init(const char *binpath);
void VFS_Destroy();

void VFS_LoadGame(gametypes gt);

VFile *VFS_Open(const char *fname, const char* mode="rb");
void VFS_Close(VFile *file);

const char* VFS_getFirstExisting(const vector<const char*>& files);
const char* VFS_getFirstExisting(Uint32 count, ...);

#endif
