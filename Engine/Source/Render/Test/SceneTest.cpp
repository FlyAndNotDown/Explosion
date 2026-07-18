//
// Created by johnk on 2026/7/17.
//

#include <utility>

#include <Test/Test.h>
#include <Core/Thread.h>
#include <Render/Scene.h>

using namespace Render;

TEST(SceneTest, StoresConcreteProxyTypesIndependently)
{
    Core::ScopedThreadTag threadTag(Core::ThreadTag::render);
    Scene scene;
    constexpr Scene::EntityId entity = 1;

    DirectionalLightSceneProxy directionalLight;
    directionalLight.intensity = 1.0f;
    PointLightSceneProxy pointLight;
    pointLight.intensity = 2.0f;
    SpotLightSceneProxy spotLight;
    spotLight.intensity = 3.0f;
    StaticPrimitiveSceneProxy staticPrimitive;

    scene.Add<DirectionalLightSceneProxy>(entity, std::move(directionalLight));
    scene.Add<PointLightSceneProxy>(entity, std::move(pointLight));
    scene.Add<SpotLightSceneProxy>(entity, std::move(spotLight));
    scene.Add<StaticPrimitiveSceneProxy>(entity, std::move(staticPrimitive));

    EXPECT_EQ(scene.All<DirectionalLightSceneProxy>().size(), 1);
    EXPECT_EQ(scene.All<PointLightSceneProxy>().size(), 1);
    EXPECT_EQ(scene.All<SpotLightSceneProxy>().size(), 1);
    EXPECT_EQ(scene.All<StaticPrimitiveSceneProxy>().size(), 1);
    EXPECT_EQ(scene.Get<DirectionalLightSceneProxy>(entity).intensity, 1.0f);
    EXPECT_EQ(scene.Get<PointLightSceneProxy>(entity).intensity, 2.0f);
    EXPECT_EQ(scene.Get<SpotLightSceneProxy>(entity).intensity, 3.0f);
}
