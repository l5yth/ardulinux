#pragma once

#include "vfs_api.h"

extern fs::FS MeshduinoFS;

extern std::shared_ptr<VFSImpl> meshduinoVFS; // Do not use directly, instead use MeshduinoFS
