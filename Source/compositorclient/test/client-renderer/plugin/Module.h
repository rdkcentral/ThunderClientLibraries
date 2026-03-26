#pragma once

#ifndef MODULE_NAME
#define MODULE_NAME Plugin_CompositionClientRender
#endif

#include <core/core.h>
#include <messaging/messaging.h>
#include <plugins/plugins.h>

#if defined(__WINDOWS__)
#if defined(COMPOSITORCLIENT_EXPORTS)
#undef EXTERNAL
#define EXTERNAL EXTERNAL_EXPORT
#else
#pragma comment(lib, "compositorclient.lib")
#endif
#endif

