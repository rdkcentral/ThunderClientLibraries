#ifndef MODULE_NAME
#define MODULE_NAME LocalTraceTest
#endif

#include <core/core.h>
#include <localtracer/localtracer.h>
#include <messaging/messaging.h>

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

using namespace WPEFramework;

constexpr char module[] = "LocalTraceTest";

int main(int /*argc*/, const char* argv[])
{
    Messaging::LocalTracer& tracer = Messaging::LocalTracer::Open();

    const char* executableName(Core::FileNameOnly(argv[0]));
    {
        Messaging::ConsolePrinter printer(true);

        tracer.Callback(&printer);
        tracer.EnableMessage(module, "Error", true);
        tracer.EnableMessage(module, "Information", true);

        TRACE(Trace::Information, ("%s - build: %s", executableName, __TIMESTAMP__));

        sleep(5);

        TRACE(Trace::Error, ("Exiting.... "));
    }

    tracer.Close();

    WPEFramework::Core::Singleton::Dispose();

    return 0;
}