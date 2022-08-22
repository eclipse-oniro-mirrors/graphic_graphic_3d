/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "3d_widget_adapter_test.h"

#include "3d_widget_adapter_log.h"
#include "graphics_manager.h"

namespace OHOS::Render3D {
void WidgetAdapter3DTest::SetUpTestCase()
{
}

void WidgetAdapter3DTest::TearDownTestCase()
{
}

void WidgetAdapter3DTest::SetUp()
{
}

void WidgetAdapter3DTest::TearDown()
{
}

void EngineTest::CreateEcs(uint32_t key)
{
    (void)(key);
}

void EngineTest::LoadCustGeometry(std::vector<OHOS::Ace::RefPtr<SVGeometry>> &shapes)
{
    (void)(shapes);
}

void EngineTest::SetUpCustomRenderTarget(const TextureInfo &info)
{
    (void)(info);
}

void EngineTest::SetUpCameraViewPort(uint32_t width, uint32_t height)
{
    (void)(width);
    (void)(height);
}

void EngineTest::SetUpCameraTransform(float position[], float rotationAngle, float rotationAxis[])
{
    (void)(position);
    (void)(rotationAngle);
    (void)(rotationAxis);
}

void EngineTest::SetUpCameraViewProjection(float zNear, float zFar, float fovDegrees)
{
    (void)(zNear);
    (void)(zFar);
    (void)(fovDegrees);
}

void EngineTest::SetLightProperties(int lightType, float color[], float intensity, bool shadow, float position[],
    float rotationAngle, float rotationAxis[])
{
    (void)(lightType);
    (void)(color);
    (void)(intensity);
    (void)(shadow);
    (void)(position);
    (void)(rotationAngle);
    (void)(rotationAxis);
}

void EngineTest::LoadSceneModel(std::string modelPath)
{
    (void)(modelPath);
}

void EngineTest::LoadBackgroundModel(std::string modelPath, SceneViewerBackgroundType type)
{
    (void)(type);
    (void)(modelPath);
}

void EngineTest::OnTouchEvent(const SceneViewerTouchEvent& event)
{
    (void)(event);
}

void EngineTest::Tick(const uint64_t aTotalTime, const uint64_t aDeltaTime)
{
    (void)(aDeltaTime);
    (void)(aTotalTime);
}

void EngineTest::AddGeometries(const std::vector<OHOS::Ace::RefPtr<SVGeometry>>& shapes)
{
    (void)(shapes);
}

void EngineTest::UpdateGLTFAnimations(const std::vector<OHOS::Ace::RefPtr<GLTFAnimation>>& animations)
{
    (void)(animations);
}

bool EngineTest::LoadEngineLib()
{
    return true;
}

bool EngineTest::InitEngine(EGLContext eglContext, const PlatformData& data)
{
    (void)(eglContext);
    (void)(data);
    return true;
}

bool EngineTest::IsAnimating()
{
    return true;
}

namespace {
/**
 * @tc.name: SetUpSceneViewer1
 * @tc.desc: Verify SceneViewer setup
 * @tc.type: FUNC
 * @tc.require: SR000GUGO2
 */
HWTEST_F(WidgetAdapter3DTest, SetUpSceneViewer1, testing::ext::TestSize.Level1)
{
    SceneViewerAdapter adapter(0U);

    auto engine = std::make_unique<EngineTest>();
    adapter.SetEngine(std::move(engine));

    TextureInfo texture {};
    std::string gltf {};
    std::string background {};
    SceneViewerBackgroundType type = SceneViewerBackgroundType::CUBE_MAP;
    adapter.SetUpSceneViewer(texture, gltf, background, type);
}

/**
 * @tc.name: SetUpCameraViewProjection1
 * @tc.desc: Verify SceneViewer setup projection
 * @tc.type: FUNC
 * @tc.require: SR000GUGO2
 */
HWTEST_F(WidgetAdapter3DTest, SetUpCameraViewProjection1, testing::ext::TestSize.Level1)
{
    SceneViewerAdapter adapter(0U);
    float near = 0.0f;
    float far = 0.0f;
    float degree = 0.0f;
    adapter.SetUpCameraViewProjection(near, far, degree);
}

/**
 * @tc.name: CreateLight1
 * @tc.desc: Verify SceneViewer Create light
 * @tc.type: FUNC
 * @tc.require: SR000GUGO2
 */
HWTEST_F(WidgetAdapter3DTest, CreateLight1, testing::ext::TestSize.Level1)
{
    SceneViewerAdapter adapter(0U);
    adapter.CreateLight();
}

/**
 * @tc.name: SetUpCustomRenderTarget1
 * @tc.desc: Verify SceneViewer Create RenderTarget
 * @tc.type: FUNC
 * @tc.require: SR000GUGO2
 */
HWTEST_F(WidgetAdapter3DTest, SetUpCustomRenderTarget1, testing::ext::TestSize.Level1)
{
    SceneViewerAdapter adapter(0U);
    TextureInfo texture;
    adapter.SetUpCustomRenderTarget(texture);
}

/**
 * @tc.name: UnLoadModel1
 * @tc.desc: Verify SceneViewer unload model
 * @tc.type: FUNC
 * @tc.require: SR000GUGO2
 */
HWTEST_F(WidgetAdapter3DTest, UnLoadModel1, testing::ext::TestSize.Level1)
{
    SceneViewerAdapter adapter(0U);
    adapter.UnLoadModel();
}

/**
 * @tc.name: OnTouchEvent1
 * @tc.desc: Verify SceneViewer handle touch event
 * @tc.type: FUNC
 * @tc.require: SR000GUGO2
 */
HWTEST_F(WidgetAdapter3DTest, OnTouchEvent1, testing::ext::TestSize.Level1)
{
    SceneViewerAdapter adapter(0U);
    int32_t pointerId = 0;
    SceneViewerTouchEvent event(pointerId);
    adapter.OnTouchEvent(event);
}

/**
 * @tc.name: DrawFrame1
 * @tc.desc: Verify SceneViewer render frame
 * @tc.type: FUNC
 * @tc.require: SR000GUI0P
 */
HWTEST_F(WidgetAdapter3DTest, DrawFrame1, testing::ext::TestSize.Level1)
{
    SceneViewerAdapter adapter(0U);
    adapter.DrawFrame();
}

/**
 * @tc.name: DrawFrame1
 * @tc.desc: Verify SceneViewer tick frame
 * @tc.type: FUNC
 * @tc.require: SR000GUI0P
 */
HWTEST_F(WidgetAdapter3DTest, Tick1, testing::ext::TestSize.Level1)
{
    SceneViewerAdapter adapter(0U);
    uint64_t total = 1U;
    uint64_t delta = 1U;
    adapter.Tick(total, delta);
}

/**
 * @tc.name: AddGeometries1
 * @tc.desc: Verify SceneViewer add geometries
 * @tc.type: FUNC
 * @tc.require: SR000GUGO2
 */
HWTEST_F(WidgetAdapter3DTest, AddGeometries1, testing::ext::TestSize.Level1)
{
    SceneViewerAdapter adapter(0U);
    adapter.AddGeometries({});
}

/**
 * @tc.name: UpdateGLTFAnimations1
 * @tc.desc: Verify update gltf animation
 * @tc.type: FUNC
 * @tc.require: SR000GUGO2
 */
HWTEST_F(WidgetAdapter3DTest, UpdateGLTFAnimations1, testing::ext::TestSize.Level1)
{
    SceneViewerAdapter adapter(0U);
    adapter.UpdateGLTFAnimations( {} );
}

/**
 * @tc.name: SetEngine1
 * @tc.desc: Verify set null engine
 * @tc.type: FUNC
 * @tc.require: SR000GUGO2
 */
HWTEST_F(WidgetAdapter3DTest, SetEngine1, testing::ext::TestSize.Level1)
{
    SceneViewerAdapter adapter(0U);
    adapter.SetEngine(nullptr);
}

/**
 * @tc.name: SetEngine2
 * @tc.desc: Verify set valid engine
 * @tc.type: FUNC
 * @tc.require: SR000GUGO2
 */
HWTEST_F(WidgetAdapter3DTest, SetEngine2, testing::ext::TestSize.Level1)
{
    SceneViewerAdapter adapter(0U);
    auto engine = std::make_unique<EngineTest>();
    adapter.SetEngine(std::move(engine));
}

/**
 * @tc.name: IsAnimating1
 * @tc.desc: Verify get animation state
 * @tc.type: FUNC
 * @tc.require: SR000GUGO2
 */
HWTEST_F(WidgetAdapter3DTest, IsAnimating1, testing::ext::TestSize.Level1)
{
    SceneViewerAdapter adapter(0U);
    auto animating = adapter.IsAnimating();
    ASSERT_EQ(animating, false);
}

/**
 * @tc.name: IsAnimating2
 * @tc.desc: Verify get animation state
 * @tc.type: FUNC
 * @tc.require: SR000GUGO2
 */
HWTEST_F(WidgetAdapter3DTest, IsAnimating2, testing::ext::TestSize.Level1)
{
    SceneViewerAdapter adapter(0U);
    auto engine = std::make_unique<EngineTest>();
    adapter.SetEngine(std::move(engine));

    auto animating = adapter.IsAnimating();
    ASSERT_EQ(animating, true);
}
} // namespace
} // namespace OHOS::Render3D
