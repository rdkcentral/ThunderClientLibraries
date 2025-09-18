#include <gbm.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <cstring>
#include <string>
#include <sstream>

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm_fourcc.h>

#define EGL_EGLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>

/**
 * @class SimpleGBMTest
 * @brief A utility class for testing GBM buffer management and EGL integration.
 *
 * This class encapsulates the initialization and management of GBM (Generic Buffer Management)
 * devices and surfaces, as well as EGL context and surface creation for rendering. It provides
 * functionality to test buffer reuse, inspect buffer file descriptors, and retrieve buffer
 * information such as format, modifier, size, and stride.
 *
 * Internal buffer objects are managed via the BufferObject struct, which wraps a GBM buffer
 * object and its associated file descriptor, providing validity checks and cleanup logic.
 *
 * Key Features:
 * - Initialize a DRM device and create a GBM device/surface.
 * - Set up EGL display, context, and surface for rendering.
 * - Test buffer reuse across multiple iterations, tracking buffer validity and reuse status.
 * - Inspect and validate buffer file descriptors using DRM ioctl calls.
 * - Retrieve detailed buffer information for debugging and analysis.
 *
 * Usage:
 * 1. Call Initialize() with the DRM device path to set up GBM and EGL.
 * 2. Use TestBufferReuse() to perform buffer reuse tests.
 * 3. The destructor automatically cleans up all resources.
 *
 * Thread Safety: Not thread-safe.
 *
 * Dependencies:
 * - GBM (libgbm)
 * - EGL (libEGL)
 * - DRM (libdrm)
 * - POSIX (unistd, fcntl)
 */
class SimpleGBMTest {
private:
    struct BufferObject {
        int fd;
        gbm_bo* boPtr;

        explicit BufferObject(gbm_bo* bo)
            : fd((bo != nullptr) ? gbm_bo_get_fd(bo) : -1)
            , boPtr(bo)
        {
        }

        ~BufferObject()
        {
            // Only close if it is valid and not owned elsewhere
            if (fd >= 0) {
                close(fd);
            }
            boPtr = nullptr;
        }

        bool IsValid() const
        {
            return (fd >= 0) && (boPtr != nullptr);
        }

        static void Destroy(gbm_bo* bo, void* data)
        {
            if (!data)
                return;
            auto* buffer = static_cast<BufferObject*>(data);
            if (bo != buffer->boPtr) {
                std::cout << "Warning: destroying mismatched buffer!" << std::endl;
            }
            delete buffer;
        }

        bool CheckFd() const
        {
            return (fd >= 0) && (fcntl(fd, F_GETFL) != -1);
        }
    };

    bool InspectGbmFd(int dmaFd) const
    {
        drm_prime_handle args{};
        args.handle = 0;
        args.flags = 0; // just validity check
        args.fd = dmaFd;

        if (drmIoctl(_drmFd, DRM_IOCTL_PRIME_FD_TO_HANDLE, &args) < 0) {
            std::cerr << "InspectGbmFd: " << std::strerror(errno) << "\n";
            return false;
        }

        drm_gem_close close_args{};
        close_args.handle = args.handle;
        drmIoctl(_drmFd, DRM_IOCTL_GEM_CLOSE, &close_args);
        return true;
    }

    std::string GetInfo(int dmaFd, uint32_t width, uint32_t height, uint32_t stride, uint32_t format) const
    {
        gbm_import_fd_data data{ dmaFd, width, height, stride, format };

        gbm_bo* bo = gbm_bo_import(_gbmDevice, GBM_BO_IMPORT_FD, &data, 0);
        if (!bo)
            return "gbm_bo_import failed";

        uint32_t _format = gbm_bo_get_format(bo);
        uint64_t _modifier = gbm_bo_get_modifier(bo);
        uint32_t _width = gbm_bo_get_width(bo);
        uint32_t _height = gbm_bo_get_height(bo);
        uint32_t _stride = gbm_bo_get_stride(bo);

        const char* formatName = drmGetFormatName(_format);
        const char* modifierName = drmGetFormatModifierName(_modifier);

        std::ostringstream oss;
        oss << "Format: " << (formatName ? formatName : "UNKNOWN")
            << " (0x" << std::hex << _format << std::dec << ")"
            << ", Modifier: ";

        if (modifierName)
            oss << modifierName;
        else {
            std::ostringstream modStr;
            modStr << "0x" << std::hex << _modifier;
            oss << modStr.str();
        }

        oss << ", Size: " << _width << "x" << _height
            << ", Stride: " << _stride;

        gbm_bo_destroy(bo);
        return oss.str();
    }

    int _drmFd{ -1 };
    gbm_device* _gbmDevice{ nullptr };
    gbm_surface* _surface{ nullptr };
    EGLDisplay _eglDisplay{ EGL_NO_DISPLAY };
    EGLContext _eglContext{ EGL_NO_CONTEXT };
    EGLSurface _eglSurface{ EGL_NO_SURFACE };
    std::vector<BufferObject*> _buffers;

public:
    ~SimpleGBMTest() { Cleanup(); }

    bool Initialize(const std::string& device)
    {
        _drmFd = open(device.c_str(), O_RDWR | O_CLOEXEC);
        if (_drmFd < 0) {
            std::cerr << "Failed to open " << device << ": " << std::strerror(errno) << "\n";
            return false;
        }
        std::cout << "Opened DRM device: " << device << std::endl;

        _gbmDevice = gbm_create_device(_drmFd);
        if (!_gbmDevice) {
            std::cerr << "Failed to create GBM device\n";
            return false;
        }

        const char* backend = gbm_device_get_backend_name(_gbmDevice);
        std::cout << "GBM backend: " << (backend ? backend : "unknown") << std::endl;

        // Default: Mesa-style flags (Intel/AMD/others)
        uint32_t flags = GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING;

        // NVIDIA’s GBM implementation requires *no flags*
        if (backend && strcasecmp(backend, "nvidia") == 0) {
            flags = 0;
            std::cout << "Using NVIDIA GBM backend → creating surface with no flags" << std::endl;
        } else {
            std::cout << "Using Mesa/other GBM backend → creating surface with SCANOUT | RENDERING" << std::endl;
        }

        _surface = gbm_surface_create(_gbmDevice, 640, 480, GBM_FORMAT_XRGB8888, flags);
        if (!_surface) {
            std::cerr << "Failed to create GBM surface\n";
            return false;
        }
        std::cout << "Created GBM surface.\n";

        return InitEGL();
    }

    void TestBufferReuse(int iterations)
    {
        std::cout << "\n=== Testing Buffer Reuse ===\n";

        for (int i = 0; i < iterations; ++i) {
            std::cout << "\n--- Iteration " << (i + 1) << " ---\n";

            eglSwapBuffers(_eglDisplay, _eglSurface);

            gbm_bo* bo = gbm_surface_lock_front_buffer(_surface);
            if (!bo) {
                std::cerr << "Failed to lock front buffer!\n";
                break;
            }

            BufferObject* object = static_cast<BufferObject*>(gbm_bo_get_user_data(bo));
            bool isReused = true;

            if (!object) {
                object = new BufferObject(bo);
                gbm_bo_set_user_data(bo, object, &BufferObject::Destroy);
                isReused = false;
            }

            std::cout << "Buffer: " << bo
                      << " (" << (bo == object->boPtr ? "EQUAL PTR" : "DIFFERENT PTR") << ")"
                      << " (" << (isReused ? "REUSED" : "NEW") << ")"
                      << " (" << (object->CheckFd() ? "VALID FD" : "INVALID FD") << ")"
                      << " (" << (InspectGbmFd(object->fd) ? "VALID BUFFER" : "INVALID BUFFER") << ")"
                      << " - " << GetInfo(object->fd, gbm_bo_get_width(bo), gbm_bo_get_height(bo), gbm_bo_get_stride(bo), gbm_bo_get_format(bo))
                      << "\n";

            gbm_surface_release_buffer(_surface, bo);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

private:
    bool InitEGL()
    {
        _eglDisplay = eglGetPlatformDisplay(EGL_PLATFORM_GBM_KHR, _gbmDevice, nullptr);
        if (_eglDisplay == EGL_NO_DISPLAY) {
            std::cerr << "Failed to get EGL display\n";
            return false;
        }

        if (!eglInitialize(_eglDisplay, nullptr, nullptr)) {
            std::cerr << "Failed to initialize EGL\n";
            return false;
        }

        const EGLint baseAttribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_NONE
        };

        EGLint numConfigs = 0;
        if (!eglChooseConfig(_eglDisplay, baseAttribs, nullptr, 0, &numConfigs) || numConfigs == 0) {
            std::cerr << "eglChooseConfig returned no configs\n";
            return false;
        }

        std::vector<EGLConfig> configs(numConfigs);
        if (!eglChooseConfig(_eglDisplay, baseAttribs, configs.data(), numConfigs, &numConfigs)) {
            std::cerr << "Failed to get EGL configs\n";
            return false;
        }

        EGLConfig config = nullptr;
        for (int i = 0; i < numConfigs; i++) {
            EGLint visualId = 0, alphaSize = 0, red = 0, green = 0, blue = 0;
            eglGetConfigAttrib(_eglDisplay, configs[i], EGL_NATIVE_VISUAL_ID, &visualId);
            eglGetConfigAttrib(_eglDisplay, configs[i], EGL_ALPHA_SIZE, &alphaSize);
            eglGetConfigAttrib(_eglDisplay, configs[i], EGL_RED_SIZE, &red);
            eglGetConfigAttrib(_eglDisplay, configs[i], EGL_GREEN_SIZE, &green);
            eglGetConfigAttrib(_eglDisplay, configs[i], EGL_BLUE_SIZE, &blue);

            if (visualId == GBM_FORMAT_XRGB8888 && alphaSize == 0) {
                config = configs[i];
                std::cout << "Selected EGL config: "
                          << "R" << red << " G" << green << " B" << blue
                          << " A" << alphaSize
                          << " visualId=0x" << std::hex << visualId << std::dec
                          << std::endl;
                break;
            }
        }

        if (!config) {
            std::cerr << "No matching EGLConfig found for GBM_FORMAT_XRGB8888\n";
            return false;
        }

        const EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
        _eglContext = eglCreateContext(_eglDisplay, config, EGL_NO_CONTEXT, contextAttribs);
        if (_eglContext == EGL_NO_CONTEXT) {
            EGLint error = eglGetError();
            std::cerr << "Failed to create EGL context: 0x"
                      << std::hex << error << std::dec << std::endl;
            return false;
        }

        _eglSurface = eglCreateWindowSurface(_eglDisplay, config,
            (EGLNativeWindowType)_surface, nullptr);
        if (_eglSurface == EGL_NO_SURFACE) {
            EGLint error = eglGetError();
            std::cerr << "eglCreateWindowSurface failed with 0x"
                      << std::hex << error << std::dec << std::endl;
            return false;
        }

        if (!eglMakeCurrent(_eglDisplay, _eglSurface, _eglSurface, _eglContext)) {
            std::cerr << "Failed to make EGL context current\n";
            return false;
        }

        std::cout << "EGL initialized successfully\n";
        return true;
    }

    void Cleanup()
    {
        if (_eglDisplay != EGL_NO_DISPLAY) {
            eglMakeCurrent(_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            if (_eglSurface != EGL_NO_SURFACE)
                eglDestroySurface(_eglDisplay, _eglSurface);
            if (_eglContext != EGL_NO_CONTEXT)
                eglDestroyContext(_eglDisplay, _eglContext);
            eglTerminate(_eglDisplay);
        }
        if (_surface)
            gbm_surface_destroy(_surface);
        if (_gbmDevice)
            gbm_device_destroy(_gbmDevice);
        if (_drmFd >= 0)
            close(_drmFd);
    }
};

int main(int argc, char* argv[])
{
    std::string device = "/dev/dri/card0";
    int iterations = 10;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-d" && i + 1 < argc)
            device = argv[++i];
        else if (arg == "-i" && i + 1 < argc)
            iterations = std::atoi(argv[++i]);
        else if (arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [-d device] [-i iterations]\n";
            return 0;
        }
    }

    std::cout << "GBM Buffer Reuse Test\nDevice: " << device << "\nIterations: " << iterations << "\n";

    SimpleGBMTest test;
    if (!test.Initialize(device)) {
        std::cerr << "Initialization failed!\n";
        return 1;
    }

    test.TestBufferReuse(iterations);
    return 0;
}
