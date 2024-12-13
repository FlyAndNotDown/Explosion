//
// Created by johnk on 2024/12/9.
//

#include <ECSTest.h>

TEST(ECSTest, EntityTest)
{
    ECRegistry registry;
    const auto entity0 = registry.Create();
    const auto entity1 = registry.Create();
    ASSERT_EQ(registry.Size(), 2);
    ASSERT_TRUE(registry.Valid(entity0));
    ASSERT_TRUE(registry.Valid(entity1));
    ASSERT_FALSE(registry.Valid(99));

    std::unordered_set cur = { entity0, entity1 };
    for (const auto e : registry) {
        ASSERT_TRUE(cur.contains(e));
    }

    const auto entity2 = registry.Create();
    const auto entity3 = registry.Create();
    registry.Destroy(entity0);
    const auto entity4 = registry.Create();
    cur = { entity1, entity2, entity3, entity4 };

    registry.Each([&](const auto e) -> void {
        ASSERT_TRUE(cur.contains(e));
    });
}

TEST(ECSTest, ComponentStaticTest)
{
    ECRegistry registry;
    const auto entity0 = registry.Create();
    const auto entity1 = registry.Create();

    registry.Emplace<CompA>(entity0, 1);
    registry.Emplace<CompA>(entity1, 2);
    ASSERT_TRUE(registry.Has<CompA>(entity0));
    ASSERT_TRUE(registry.Has<CompA>(entity1));
    ASSERT_EQ(registry.Find<CompA>(entity0)->value, 1);
    ASSERT_EQ(registry.Find<CompA>(entity1)->value, 2);
    ASSERT_EQ(registry.Get<CompA>(entity0).value, 1);
    ASSERT_EQ(registry.Get<CompA>(entity1).value, 2);

    const ECRegistry& constRegistry = registry;
    ASSERT_TRUE(constRegistry.Has<CompA>(entity0));

    ASSERT_EQ(constRegistry.Find<CompA>(entity0)->value, 1);
    ASSERT_EQ(constRegistry.Find<CompA>(entity1)->value, 2);
    ASSERT_EQ(constRegistry.Get<CompA>(entity0).value, 1);
    ASSERT_EQ(constRegistry.Get<CompA>(entity1).value, 2);

    registry.Remove<CompA>(entity0);
    ASSERT_FALSE(constRegistry.Has<CompA>(entity0));

    const auto entity2 = registry.Create();
    registry.Emplace<CompA>(entity2, 3);
    registry.Emplace<CompB>(entity2, 4.0f);
    ASSERT_TRUE(constRegistry.Has<CompA>(entity2));
    ASSERT_TRUE(constRegistry.Has<CompB>(entity2));
    ASSERT_EQ(constRegistry.Get<CompA>(entity2).value, 3);
    ASSERT_EQ(constRegistry.Get<CompB>(entity2).value, 4.0f);

    const auto entity3 = registry.Create();
    registry.Emplace<CompA>(entity3, 5);
    registry.Emplace<CompB>(entity3, 6.0f);
    registry.Remove<CompA>(entity3);
    ASSERT_FALSE(constRegistry.Has<CompA>(entity3));
    ASSERT_TRUE(constRegistry.Has<CompB>(entity3));
    ASSERT_EQ(constRegistry.Get<CompB>(entity3).value, 6.0f);

    std::unordered_set expected = { entity1, entity2 };
    const auto view0 = registry.View<CompA>();
    view0.Each([&](Entity e) -> void {
        ASSERT_TRUE(expected.contains(e));
    });

    expected = { entity1 };
    const auto view1 = registry.View<CompA>(Exclude<CompB> {});
    view1.Each([&](Entity e) -> void {
        ASSERT_TRUE(expected.contains(e));
    });

    expected = { entity2, entity3 };
    const auto view2 = registry.View<CompB>();
    view2.Each([&](Entity e) -> void {
        ASSERT_TRUE(expected.contains(e));
    });

    expected = { entity3 };
    const auto view3 = registry.View<CompB>(Exclude<CompA> {});
    view3.Each([&](Entity e) -> void {
        ASSERT_TRUE(expected.contains(e));
    });
}

TEST(ECSTest, ComponentDynamicTest)
{
    CompClass compAClass = &Mirror::Class::Get<CompA>();
    CompClass compBClass = &Mirror::Class::Get<CompB>();

    ECRegistry registry;
    const ECRegistry& constRegistry = registry;
    const auto entity0 = registry.Create();
    const auto entity1 = registry.Create();
    registry.EmplaceDyn(compAClass, entity0, Mirror::ForwardAsArgList(1));
    registry.EmplaceDyn(compAClass, entity1, Mirror::ForwardAsArgList(2));
    ASSERT_TRUE(registry.HasDyn(compAClass, entity0));
    ASSERT_TRUE(constRegistry.HasDyn(compAClass, entity1));
    ASSERT_EQ(registry.GetDyn(compAClass, entity0).As<CompA&>().value, 1);
    ASSERT_EQ(constRegistry.GetDyn(compAClass, entity1).As<const CompA&>().value, 2);
    ASSERT_EQ(registry.FindDyn(compAClass, entity0).As<CompA&>().value, 1);
    ASSERT_EQ(constRegistry.FindDyn(compAClass, entity1).As<const CompA&>().value, 2);

    // TODO
}

TEST(ECSTest, GlobalComponentStaticTest)
{
    // TODO
}

TEST(ECSTest, GlobalComponentDynamicTest)
{
    // TODO
}

TEST(ECSTest, ComponentEventStaticTest)
{
    // TODO
}

TEST(ECSTest, ComponentEventDynamicTest)
{
    // TODO
}

TEST(ECSTest, ObserverTest)
{
    // TODO
}
