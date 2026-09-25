// One-call bootstrap for all core services (call from main after
// QCoreApplication exists).

#pragma once

namespace AppleCat::Core {

void coreInit();
const char *coreVersion();

}
