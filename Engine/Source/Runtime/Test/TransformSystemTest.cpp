#include <unordered_set>

#include <Test/Test.h>
#include <Runtime/Component/Transform.h>
#include <Runtime/System/Transform.h>

TEST(TransformSystemTest, PublishesResolvedWorldTransformChanges)
{
    Runtime::ECRegistry registry;
    const Runtime::Entity root = registry.Create();
    Common::FTransform rootTransform;
    rootTransform.translation = Common::FVec3(1.0f, 0.0f, 0.0f);
    registry.Emplace<Runtime::WorldTransform>(root, rootTransform);
    registry.Emplace<Runtime::Hierarchy>(root);

    const Runtime::Entity child = registry.Create();
    Common::FTransform childWorldTransform;
    childWorldTransform.translation = Common::FVec3(3.0f, 0.0f, 0.0f);
    registry.Emplace<Runtime::WorldTransform>(child, childWorldTransform);
    Common::FTransform childLocalTransform;
    childLocalTransform.translation = Common::FVec3(2.0f, 0.0f, 0.0f);
    registry.Emplace<Runtime::LocalTransform>(child, childLocalTransform);
    registry.Emplace<Runtime::Hierarchy>(child);
    Runtime::HierarchyOps::AttachToParent(registry, child, root);

    Runtime::SystemSetupContext setupContext;
    Runtime::TransformSystem transformSystem(registry, setupContext);
    auto resolvedWorldTransforms = registry.Observer();
    resolvedWorldTransforms.ObUpdated<Runtime::WorldTransform>();

    registry.Update<Runtime::WorldTransform>(root, [](Runtime::WorldTransform& transform) -> void {
        transform.localToWorld.translation.x = 5.0f;
    });
    transformSystem.Tick(1.0f / 60.0f);

    EXPECT_FLOAT_EQ(registry.Get<Runtime::WorldTransform>(child).localToWorld.translation.x, 7.0f);
    const std::unordered_set<Runtime::Entity> firstChangedEntities(resolvedWorldTransforms.begin(), resolvedWorldTransforms.end());
    EXPECT_EQ(firstChangedEntities, (std::unordered_set<Runtime::Entity> { root, child }));
    resolvedWorldTransforms.Clear();

    registry.Update<Runtime::LocalTransform>(child, [](Runtime::LocalTransform& transform) -> void {
        transform.localToParent.translation.x = 4.0f;
    });
    transformSystem.Tick(1.0f / 60.0f);

    EXPECT_FLOAT_EQ(registry.Get<Runtime::WorldTransform>(child).localToWorld.translation.x, 9.0f);
    EXPECT_EQ(resolvedWorldTransforms.All(), (std::vector<Runtime::Entity> { child }));
}
