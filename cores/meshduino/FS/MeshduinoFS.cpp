#include "MeshduinoFS.h"

// Do not use directly, instead use MeshduinoFS
std::shared_ptr<VFSImpl> meshduinoVFS = std::make_shared<VFSImpl>();
// std::shared_ptr<VFSImpl> meshduinoVFS(new VFSImpl());

FS MeshduinoFS = FS(meshduinoVFS);