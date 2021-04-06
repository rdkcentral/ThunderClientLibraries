#include "ModeSet.h"

#include <vector>
#include <list>
#include <string>
#include <cassert>
#include <limits>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

extern "C"
{
#include <drm/drm_fourcc.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>
}

static constexpr uint8_t DrmMaxDevices()
{
    // Just an arbitrary choice
    return 16;
}

static void GetNodes(uint32_t type, std::vector<std::string>& list)
{
    drmDevicePtr devices[DrmMaxDevices()];

    static_assert(sizeof(DrmMaxDevices()) <= sizeof(int));
    static_assert(std::numeric_limits<decltype(DrmMaxDevices())>::max() <= std::numeric_limits<int>::max());

    int device_count = drmGetDevices2(0 /* flags */, &devices[0], static_cast<int>(DrmMaxDevices()));

    if (device_count > 0)
    {
        for (int i = 0; i < device_count; i++)
        {
            switch (type)
            {
                case DRM_NODE_PRIMARY   :   // card<num>, always created, KMS, privileged
                case DRM_NODE_CONTROL   :   // ControlD<num>, currently unused
                case DRM_NODE_RENDER    :   // Solely for render clients, unprivileged
                                            {
                                                if ((1 << type) == (devices[i]->available_nodes & (1 << type)))
                                                {
                                                    list.push_back( std::string(devices[i]->nodes[type]) );
                                                }
                                                break;
                                            }
                case DRM_NODE_MAX       :
                default                 :   // Unknown (new) node type
                                        ;
            }
        }

        drmFreeDevices(&devices[0], device_count);
    }
}

static int FileDescriptor()
{
    int fd = -1;
    static std::vector<std::string> nodes;

    if (nodes.size() == 0) {
        GetNodes(DRM_NODE_PRIMARY, nodes);
    }

    std::vector<std::string>::iterator index(nodes.begin());

    while ((index != nodes.end()) && (fd == -1)) {
        // Select the first from the list
        if (index->empty() == false)
        {
            // The node might be priviliged and the call will fail.
            // Do not close fd with exec functions! No O_CLOEXEC!
            fd = open(index->c_str(), O_RDWR); 
            printf("Opening: %s [%d]\n", index->c_str(), fd);
        }
        index++;
    }

    return (fd);
}

static uint32_t GetConnectors(int fd, uint32_t type)
{
    uint32_t bitmask = 0;

    drmModeResPtr resources = drmModeGetResources(fd);

    if(nullptr != resources)
    {
        for (int i = 0; i < resources->count_connectors; i++)
        {
            drmModeConnectorPtr connector = drmModeGetConnector(fd, resources->connectors[i]);

            if(nullptr != connector)
            {
                if ((type == connector->connector_type) && (DRM_MODE_CONNECTED == connector->connection))
                {
                    bitmask = bitmask | (1 << i);
                }

                drmModeFreeConnector(connector);
            }
        }

        drmModeFreeResources(resources);
    }

    return bitmask;
}

static uint32_t GetCRTCS(int fd, bool valid)
{
    uint32_t bitmask = 0;

    drmModeResPtr resources = drmModeGetResources(fd);

    if(nullptr != resources)
    {
        for(int i = 0; i < resources->count_crtcs; i++)
        {
            drmModeCrtcPtr crtc = drmModeGetCrtc(fd, resources->crtcs[i]);

            if(nullptr != crtc)
            {
                bool currentSet = (crtc->mode_valid == 1);

                if(valid == currentSet)
                {
                    bitmask = bitmask | (1 << i);
                }
                drmModeFreeCrtc(crtc);
            }
        }

        drmModeFreeResources(resources);
    }

    return bitmask;
}

static bool FindProperDisplay(int fd, uint32_t& crtc, uint32_t& encoder, uint32_t& connector, uint32_t& fb)
{
    bool found = false;

    assert(fd != -1);

    // Only connected connectors are considered
    uint32_t connectorMask = GetConnectors(fd, DRM_MODE_CONNECTOR_HDMIA);

    // All CRTCs are considered for the given mode (valid / not valid)
    uint32_t crtcs = GetCRTCS(fd, true);

    drmModeResPtr resources = drmModeGetResources(fd);

    if(nullptr != resources)
    {
        int32_t connectorIndex = 0;

        while ((found == false) && (connectorIndex < resources->count_connectors))
        {
            if ((connectorMask & (1 << connectorIndex)) != 0)
            {
                drmModeConnectorPtr connectors = drmModeGetConnector(fd, resources->connectors[connectorIndex]);

                if(nullptr != connectors)
                {
                    int32_t encoderIndex = 0;

                    while ((found == false) && (encoderIndex < connectors->count_encoders))
                    {
                        drmModeEncoderPtr encoders = drmModeGetEncoder(fd, connectors->encoders[encoderIndex]);

                        if (nullptr != encoders)
                        {
                            uint32_t  matches = (encoders->possible_crtcs & crtcs);
                            uint32_t* pcrtc   = resources->crtcs;

                            while ((found == false) && (matches > 0))
                            {
                                if ((matches & 1) != 0)
                                {
                                    drmModeCrtcPtr modePtr = drmModeGetCrtc(fd, *pcrtc);

                                    if(nullptr != modePtr)
                                    {
                                        // A viable set found
                                        crtc = *pcrtc;
                                        encoder = encoders->encoder_id;
                                        connector = connectors->connector_id;
                                        fb = modePtr->buffer_id;

                                        drmModeFreeCrtc(modePtr);
                                        found = true;
                                    }
                                }
                                matches >>= 1;
                                pcrtc++;
                            }
                            drmModeFreeEncoder(encoders);
                        }
                        encoderIndex++;
                    }
                    drmModeFreeConnector(connectors);
                }
            }
            connectorIndex++;
        }

        drmModeFreeResources(resources);
    }

    return (found);
}

static bool CreateBuffer(int fd, const uint32_t connector, gbm_device*& device, uint32_t& modeIndex, uint32_t& id, struct gbm_bo*& buffer)
{
    assert(fd != -1);

    bool created = false;
    buffer = nullptr;
    modeIndex = 0;
    id = 0;
    device = gbm_create_device(fd);

    if (nullptr != device)
    {
        drmModeConnectorPtr pconnector = drmModeGetConnector(fd, connector);

        if (nullptr != pconnector)
        {
            bool found = false;
            uint32_t index = 0;
            uint64_t area = 0;

            while ( (found == false) && (index < static_cast<uint32_t>(pconnector->count_modes)) )
            {
                uint32_t type = pconnector->modes[index].type;

                // At least one preferred mode should be set by the driver, but dodgy EDID parsing might not provide it
                if (DRM_MODE_TYPE_PREFERRED == (DRM_MODE_TYPE_PREFERRED & type))
                {
                    modeIndex = index;

                    // Found a suitable mode; break the loop
                    found = true;
                }
                else if (DRM_MODE_TYPE_DRIVER == (DRM_MODE_TYPE_DRIVER & type))
                {
                    // Calculate screen area
                    uint64_t size = pconnector->modes[index].hdisplay * pconnector->modes[index].vdisplay;
                    if (area < size) {
                        area = size;

                        // Use another selection criterium
                        // Select highest clock and vertical refresh rate

                        if ( (pconnector->modes[index].clock > pconnector->modes[modeIndex].clock) ||
                             ( (pconnector->modes[index].clock == pconnector->modes[modeIndex].clock) && 
                               (pconnector->modes[index].vrefresh > pconnector->modes[modeIndex].vrefresh) ) )
                        {
                            modeIndex = index;
                        }
                    }
                }
                index++;
            }

            // A large enough initial buffer for scan out
            struct gbm_bo* bo = gbm_bo_create(
                                  device, 
                                  pconnector->modes[modeIndex].hdisplay,
                                  pconnector->modes[modeIndex].vdisplay,
                                  ModeSet::SupportedBufferType(),
                                  GBM_BO_USE_SCANOUT /* presented on a screen */ | GBM_BO_USE_RENDERING /* used for rendering */);

            drmModeFreeConnector(pconnector);

            if(nullptr != bo)
            {
                // Associate a frame buffer with this bo
                int32_t fb_fd = gbm_device_get_fd(device);

                uint32_t format = gbm_bo_get_format(bo);

                assert (format == DRM_FORMAT_XRGB8888 || format == DRM_FORMAT_ARGB8888);

                uint32_t bpp = gbm_bo_get_bpp(bo);

                int32_t ret = drmModeAddFB(
                                fb_fd, 
                                gbm_bo_get_width(bo), 
                                gbm_bo_get_height(bo), 
                                format != DRM_FORMAT_ARGB8888 ? bpp - 8 : bpp,
                                bpp,
                                gbm_bo_get_stride(bo), 
                                gbm_bo_get_handle(bo).u32, &id);

                if(0 == ret)
                {
                    buffer = bo;
                    created = true;
                }
            }
        }
    }

   return created;
}

ModeSet::ModeSet()
    : _crtc(0)
    , _encoder(0)
    , _connector(0)
    , _mode(0)
    , _buffer(nullptr)
{
    if (drmAvailable() > 0) {
        _fd = FileDescriptor();

        if(_fd >= 0) {
            bool enabled = false;
            if ( (FindProperDisplay(_fd, _crtc, _encoder, _connector, _fb) == true) && 
                 /* TODO: Changes the original fb which might not be what is intended */
                 (CreateBuffer(_fd, _connector, _device, _mode, _fb, _buffer) == true) && 
                 (drmSetMaster(_fd) == 0) ) {

                drmModeConnectorPtr pconnector = drmModeGetConnector(_fd, _connector);

                if(pconnector != nullptr) {
                    /* At least one mode has to be set */
                    enabled = (0 == drmModeSetCrtc(_fd, _crtc, _fb, 0, 0, &_connector, 1, &(pconnector->modes[_mode])));

                    drmModeFreeConnector(pconnector);
                }
            }
            if (enabled == false) {
                Destruct();
            }
        }
    }
}

ModeSet::~ModeSet()
{
    Destruct();
}

void ModeSet::Destruct()
{
    // Destroy the initial buffer if one exists
    if (nullptr != _buffer)
    {
        gbm_bo_destroy(_buffer);
        _buffer = nullptr;
    }

    if (nullptr != _device)
    {
        gbm_device_destroy(_device);
        _device = nullptr;
    }

    if(_fd >= 0) {
        close(_fd);
    }

    _crtc = 0;
    _encoder = 0;
    _connector = 0;
}

uint32_t ModeSet::Width() const
{
    // Derived from modinfo if CreateBuffer was called prior to this
    uint32_t width = 0;

    if (nullptr != _buffer)
    {
        width = gbm_bo_get_width(_buffer);
    }

    return width;
}

uint32_t ModeSet::Height() const
{
    // Derived from modinfo if CreateBuffer was called prior to this
    uint32_t height = 0;

    if (nullptr != _buffer)
    {
        height = gbm_bo_get_height(_buffer);
    }

    return height;
}

static void event_vblank_handler (int fd, unsigned int sequence, unsigned int tv_sec, unsigned int tv_usec, void *user_data) {
    static_cast<ModeSet::ICallback*>(user_data)->VBlank(sequence, tv_sec, tv_usec);
}

static void event_page_flip_handler (int fd, unsigned int sequence, unsigned int tv_sec, unsigned int tv_usec, void *user_data) {
    static_cast<ModeSet::ICallback*>(user_data)->PageFlip(sequence, tv_sec, tv_usec);
}

int ModeSet::Handle() {
    drmEventContext event;
    event.version = 2;
    event.vblank_handler = event_vblank_handler;
    event.page_flip_handler = event_page_flip_handler;
    return (drmHandleEvent(_fd, &event));
}

int ModeSet::PageFlip(ICallback* callback, const uint32_t id) {
    return (drmModePageFlip(_fd, _crtc, id, DRM_MODE_PAGE_FLIP_EVENT, callback));
}

int ModeSet::VBlank(ICallback* callback) {
    return (-1);
}

// These created resources are automatically destroyed if gbm_device is destroyed
struct gbm_surface* ModeSet::CreateRenderTarget(const uint32_t width, const uint32_t height, uint32_t& id)
{
    assert(nullptr != _device);

    struct gbm_surface* result = gbm_surface_create(_device, width, height, SupportedBufferType(), GBM_BO_USE_SCANOUT /* presented on a screen */ | GBM_BO_USE_RENDERING /* used for rendering */);

    if (result != nullptr) {
        // struct gbm_bo* bo = gbm_surface_lock_front_buffer (result);

        // Associate a frame buffer with this bo
        uint32_t format = gbm_bo_get_format(_buffer);
        uint32_t bpp = gbm_bo_get_bpp(_buffer);

        assert (format == DRM_FORMAT_XRGB8888 || format == DRM_FORMAT_ARGB8888);

        int32_t ret = drmModeAddFB(
                                _fd, 
                                gbm_bo_get_width(_buffer), 
                                gbm_bo_get_height(_buffer), 
                                format != DRM_FORMAT_ARGB8888 ? bpp - BPP() + ColorDepth() : bpp,
                                bpp,
                                gbm_bo_get_stride(_buffer), 
                                gbm_bo_get_handle(_buffer).u32, &id);

        if (ret == 0) {
            printf ("Created a render target and added...[%p]\n", result);
        }
        else {
            printf ("Created a render target but failed to add it: [%d]...[%p]\n", errno, result);
        }
    }

    return result;
}

void ModeSet::DestroyRenderTarget(struct gbm_surface* surface)
{
    if (nullptr != surface)
    {
        gbm_surface_destroy(surface);
    }
}

