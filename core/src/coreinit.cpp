#include "coreinit.h"

#include "appsettings.h"
#include "completion.h"
#include "encodingservice.h"
#include "syntaxrepository.h"

namespace AppleCat::Core {

void coreInit()
{
    AppSettings::init();
    EncodingService::instance().init();
    SyntaxRepository::instance().init();
    CompletionEngine::instance()->init();
}

const char *coreVersion()
{
#ifdef APPLECAT_VERSION
    return APPLECAT_VERSION;
#else
    return "4.0.0";
#endif
}

} // namespace AppleCat::Core
