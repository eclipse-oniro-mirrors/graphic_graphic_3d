/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef API_3D_RENDER_IRENDER_DATA_STORE_DEFAULT_CAMERA_H
#define API_3D_RENDER_IRENDER_DATA_STORE_DEFAULT_CAMERA_H

#include <3d/render/render_data_defines_3d.h>
#include <base/containers/array_view.h>
#include <core/namespace.h>
#include <render/datastore/intf_render_data_store.h>

CORE3D_BEGIN_NAMESPACE()
/** @ingroup group_render_irenderdatastoredefaultcamera */
/** Interface to add cameras to rendering.
 * Not internally synchronized.
 */
class IRenderDataStoreDefaultCamera : public RENDER_NS::IRenderDataStore {
public:
    static constexpr BASE_NS::Uid UID { "9a13e890-2a33-4b45-beee-be39eaecce57" };

    ~IRenderDataStoreDefaultCamera() override = default;

    /** Add camera to scene.
     * @param camera A camera to be added.
     */
    virtual void AddCamera(const RenderCamera& camera) = 0;

    /** Get all cameras for particular scene id.
     * @return array view to all cameras.
     */
    virtual BASE_NS::array_view<const RenderCamera> GetCameras() const = 0;

    /** Get named camera.
     * @return render camera.
     */
    virtual RenderCamera GetCamera(const BASE_NS::string_view name) const = 0;

    /** Get camera by id.
     * @return render camera.
     */
    virtual RenderCamera GetCamera(const uint64_t id) const = 0;

    /** Get index of a named camera.
     * @return Index of camera in RenderCamera array. ~0u if not found.
     */
    virtual uint32_t GetCameraIndex(const BASE_NS::string_view name) const = 0;

    /** Get index of a camera id.
     * @return Index of camera in RenderCamera array. ~0u if not found.
     */
    virtual uint32_t GetCameraIndex(const uint64_t id) const = 0;

    /** Get camera count.
     * @return Count of cameras in RenderCamera array.
     */
    virtual uint32_t GetCameraCount() const = 0;

protected:
    IRenderDataStoreDefaultCamera() = default;
};
CORE3D_END_NAMESPACE()

#endif // API_3D_RENDER_IRENDER_DATA_STORE_DEFAULT_CAMERA_H
