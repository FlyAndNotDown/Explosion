//
// Created by johnk on 2024/12/9.
//

#include <ECSTest.h>
#include <Test/Test.h>

#include <utility>

uint32_t LifetimeComp::instanceCount = 0;

LifetimeComp::LifetimeComp()
{
    instanceCount++;
}

LifetimeComp::LifetimeComp(std::string inValue)
    : value(std::move(inValue))
{
    instanceCount++;
}

LifetimeComp::LifetimeComp(const LifetimeComp& inOther)
    : value(inOther.value)
{
    instanceCount++;
}

LifetimeComp::LifetimeComp(LifetimeComp&& inOther) noexcept
    : value(std::move(inOther.value))
{
    instanceCount++;
}

LifetimeComp& LifetimeComp::operator=(const LifetimeComp& inOther)
{
    value = inOther.value;
    return *this;
}

LifetimeComp& LifetimeComp::operator=(LifetimeComp&& inOther) noexcept
{
    value = std::move(inOther.value);
    return *this;
}

LifetimeComp::~LifetimeComp()
{
    instanceCount--;
}

TEST(ECSTest, EntityTest)
{
    ECRegistry registry;
    const auto entity0 = registry.Create();
    const auto entity1 = registry.Create();
    ASSERT_EQ(registry.Count(), 2);
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
    ASSERT_NE(entity4, entity0);
    ASSERT_EQ(Runtime::Internal::EntityIndexOf(entity4), Runtime::Internal::EntityIndexOf(entity0));
    ASSERT_EQ(
        Runtime::Internal::EntityGenerationOf(entity4),
        Runtime::Internal::EntityGenerationOf(entity0) + 1);
    ASSERT_FALSE(registry.Valid(entity0));
    ASSERT_TRUE(registry.Valid(entity4));
    cur = { entity1, entity2, entity3, entity4 };

    registry.Each([&](const auto e) -> void {
        ASSERT_TRUE(cur.contains(e));
    });

    registry.Clear();
    ASSERT_EQ(registry.Count(), 0);
    ASSERT_FALSE(registry.Valid(entity1));
    ASSERT_FALSE(registry.Valid(entity4));
    const Entity entityAfterClear = registry.Create();
    ASSERT_EQ(Runtime::Internal::EntityIndexOf(entityAfterClear), 1);
    ASSERT_NE(entityAfterClear, entity1);
    ASSERT_NE(entityAfterClear, entity4);
    ASSERT_TRUE(registry.Valid(entityAfterClear));
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
    ASSERT_EQ(registry.CompCount(entity2), 2);
    std::unordered_set<CompClass> compClasses;
    registry.CompEach(entity2, [&](CompClass clazz) -> void {
        compClasses.emplace(clazz);
    });
    ASSERT_EQ(compClasses, (std::unordered_set<CompClass> {
        &Mirror::Class::Get<CompA>(),
        &Mirror::Class::Get<CompB>()
    }));

    const auto entity3 = registry.Create();
    registry.Emplace<CompA>(entity3, 5);
    registry.Emplace<CompB>(entity3, 6.0f);
    registry.Remove<CompA>(entity3);
    ASSERT_FALSE(constRegistry.Has<CompA>(entity3));
    ASSERT_TRUE(constRegistry.Has<CompB>(entity3));
    ASSERT_EQ(constRegistry.Get<CompB>(entity3).value, 6.0f);

    std::unordered_set expected = { entity1, entity2 };
    const auto view0 = registry.View<CompA>();
    ASSERT_EQ(view0.Count(), 2);
    view0.Each([&](Entity e, CompA& compA) -> void {
        ASSERT_TRUE(expected.contains(e));
    });

    expected = { entity1 };
    const auto view1 = registry.ConstView<CompA>(Exclude<CompB> {});
    ASSERT_EQ(view1.Count(), 1);
    view1.Each([&](Entity e, const CompA& compA) -> void {
        ASSERT_TRUE(expected.contains(e));
    });

    expected = { entity2, entity3 };
    const auto view2 = registry.View<CompB>();
    ASSERT_EQ(view2.Count(), 2);
    for (const auto& [entity, compB] : view2) {
        ASSERT_TRUE(expected.contains(entity));
    }

    expected = { entity3 };
    const auto view3 = registry.ConstView<CompB>(Exclude<CompA> {});
    ASSERT_EQ(view3.Count(), 1);
    for (const auto& [entity, compB] : view3) {
        ASSERT_TRUE(expected.contains(entity));
    }

    ASSERT_EQ(reinterpret_cast<uintptr_t>(registry.Find<CompA>(entity1)) % alignof(CompA), 0);
    ASSERT_EQ(reinterpret_cast<uintptr_t>(registry.Find<CompB>(entity2)) % alignof(CompB), 0);
}

TEST(ECSTest, TagStaticTest)
{
    static_assert(std::is_empty_v<TestTag>);
    static_assert(sizeof(TestTag) == 1 && alignof(TestTag) == 1);

    ECRegistry registry;
    const auto entity0 = registry.Create();
    const auto entity1 = registry.Create();
    registry.Emplace<CompA>(entity0, 7);
    registry.AddTag<TestTag>(entity0);
    registry.AddTag<TestTag>(entity1);
    registry.AddTag<OtherTestTag>(entity1);

    ASSERT_TRUE(registry.HasTag<TestTag>(entity0));
    ASSERT_TRUE(registry.HasTag<TestTag>(entity1));
    ASSERT_FALSE(registry.HasTag<OtherTestTag>(entity0));
    ASSERT_EQ(registry.Get<CompA>(entity0).value, 7);
    ASSERT_EQ(registry.CompCount(entity0), 1);
    ASSERT_EQ(registry.TagCount(entity0), 1);
    ASSERT_EQ(registry.TagCount(entity1), 2);

    std::unordered_set<TagClass> tags;
    registry.TagEach(entity1, [&](TagClass tag) -> void { tags.emplace(tag); });
    ASSERT_EQ(tags, (std::unordered_set<TagClass> {
        &Mirror::Class::Get<TestTag>(),
        &Mirror::Class::Get<OtherTestTag>()
    }));

    std::unordered_set expected = { entity0, entity1 };
    const auto tagView = registry.View(Tags<TestTag> {});
    ASSERT_EQ(tagView.Count(), 2);
    tagView.Each([&](Entity entity) -> void { ASSERT_TRUE(expected.contains(entity)); });

    const auto compAndTagView = registry.View<CompA>(Tags<TestTag> {});
    ASSERT_EQ(compAndTagView.Count(), 1);
    compAndTagView.Each([&](Entity entity, const CompA& comp) -> void {
        ASSERT_EQ(entity, entity0);
        ASSERT_EQ(comp.value, 7);
    });

    const auto excludedTagView = registry.View(Tags<TestTag> {}, Exclude<OtherTestTag> {});
    ASSERT_EQ(excludedTagView.Count(), 1);
    ASSERT_EQ(std::get<0>(*excludedTagView.begin()), entity0);

    registry.RemoveTag<TestTag>(entity0);
    ASSERT_FALSE(registry.HasTag<TestTag>(entity0));
    ASSERT_EQ(registry.Get<CompA>(entity0).value, 7);
    ASSERT_EQ(registry.TagCount(entity0), 0);

    ECRegistry dataRegistry;
    const auto dataEntity = dataRegistry.Create();
    dataRegistry.Emplace<TestTag>(dataEntity);
    ASSERT_TRUE(dataRegistry.Has<TestTag>(dataEntity));
    ASSERT_FALSE(dataRegistry.HasTag<TestTag>(dataEntity));
    ASSERT_EQ(dataRegistry.CompCount(dataEntity), 1);
    ASSERT_EQ(dataRegistry.TagCount(dataEntity), 0);
}

TEST(ECSTest, TagDynamicTest)
{
    const auto* tagClass = &Mirror::Class::Get<TestTag>();
    ECRegistry registry;
    const auto entity0 = registry.Create();
    const auto entity1 = registry.Create();
    registry.Emplace<CompA>(entity0, 3);
    registry.AddTagDyn(tagClass, entity0);
    registry.AddTagDyn(tagClass, entity1);

    ASSERT_TRUE(registry.HasTagDyn(tagClass, entity0));
    ASSERT_TRUE(registry.HasTagDyn(tagClass, entity1));

    const auto tagView = registry.RuntimeView(RuntimeFilter().IncludeTagDyn(tagClass));
    ASSERT_EQ(tagView.Count(), 2);

    int32_t sum = 0;
    const auto compAndTagView = registry.ConstRuntimeView(RuntimeFilter().Include<CompA>().IncludeTag<TestTag>());
    compAndTagView.Each([&](Entity, const CompA& comp) -> void { sum += comp.value; });
    ASSERT_EQ(sum, 3);

    registry.RemoveTagDyn(tagClass, entity0);
    ASSERT_FALSE(registry.HasTagDyn(tagClass, entity0));
    ASSERT_TRUE(registry.HasTagDyn(tagClass, entity1));
}

TEST(ECSTest, ViewArchetypeGrowthTest)
{
    ECRegistry registry;
    const auto staticView = registry.View<CompA>();
    const auto runtimeView = registry.RuntimeView(RuntimeFilter().Include<CompA>());

    const auto firstEntity = registry.Create();
    registry.Emplace<CompA>(firstEntity, 10);

    Entity lastEntity = entityNull;
    for (int32_t value = 1; value <= 64; value++) {
        lastEntity = registry.Create();
        registry.Emplace<CompA>(lastEntity, value);
    }

    int32_t staticSum = 0;
    staticView.Each([&](Entity, const CompA& comp) -> void { staticSum += comp.value; });
    ASSERT_EQ(staticView.Count(), 65);
    ASSERT_EQ(staticSum, 2090);

    int32_t runtimeSum = 0;
    runtimeView.Each([&](Entity, const CompA& comp) -> void { runtimeSum += comp.value; });
    ASSERT_EQ(runtimeView.Count(), 65);
    ASSERT_EQ(runtimeSum, 2090);

    registry.Emplace<CompB>(lastEntity, 1.0f);

    staticSum = 0;
    staticView.Each([&](Entity, const CompA& comp) -> void { staticSum += comp.value; });
    ASSERT_EQ(staticView.Count(), 65);
    ASSERT_EQ(staticSum, 2090);

    runtimeSum = 0;
    runtimeView.Each([&](Entity, const CompA& comp) -> void { runtimeSum += comp.value; });
    ASSERT_EQ(runtimeView.Count(), 65);
    ASSERT_EQ(runtimeSum, 2090);
}

TEST(ECSTest, ArchetypeLayoutIdentityTest)
{
    const auto compA = Runtime::Internal::CompRtti::Get<CompA>();
    const auto compB = Runtime::Internal::CompRtti::Get<CompB>();
    const auto* testTag = &Mirror::Class::Get<TestTag>();

    const Runtime::Internal::ArchetypeLayout layoutAB({ compA, compB }, {});
    const Runtime::Internal::ArchetypeLayout layoutBA({ compB, compA }, {});
    const Runtime::Internal::ArchetypeLayout layoutAWithTag({ compA }, Runtime::Internal::TagStorage({ testTag }));

    ASSERT_EQ(layoutAB, layoutBA);
    ASSERT_EQ(layoutAB.Hash(), layoutBA.Hash());
    ASSERT_NE(layoutAB, layoutAWithTag);

    std::unordered_map<Runtime::Internal::ArchetypeLayout, uint32_t, Runtime::Internal::ArchetypeLayoutHash> layouts;
    layouts.emplace(layoutAB, 1);
    layouts.emplace(layoutBA, 2);
    layouts.emplace(layoutAWithTag, 3);
    ASSERT_EQ(layouts.size(), 2);
    ASSERT_EQ(layouts.at(layoutAB), 1);
    ASSERT_EQ(layouts.at(layoutAWithTag), 3);
}

TEST(ECSTest, ComponentLifetimeTest)
{
    const auto baselineInstanceCount = LifetimeComp::instanceCount;
    const std::string firstValue = "first component value that does not use the small string optimization";
    const std::string secondValue = "second component value that does not use the small string optimization";
    {
        ECRegistry registry;
        const auto entity0 = registry.Create();
        const auto entity1 = registry.Create();
        registry.Emplace<LifetimeComp>(entity0, std::string(firstValue));
        registry.Emplace<LifetimeComp>(entity1, std::string(secondValue));
        ASSERT_EQ(LifetimeComp::instanceCount, baselineInstanceCount + 2);

        registry.Emplace<CompA>(entity0, 1);
        ASSERT_EQ(LifetimeComp::instanceCount, baselineInstanceCount + 2);
        ASSERT_EQ(registry.Get<LifetimeComp>(entity0).value, firstValue);
        ASSERT_EQ(registry.Get<LifetimeComp>(entity1).value, secondValue);

        registry.Remove<CompA>(entity0);
        ASSERT_EQ(LifetimeComp::instanceCount, baselineInstanceCount + 2);
        registry.Destroy(entity1);
        ASSERT_EQ(LifetimeComp::instanceCount, baselineInstanceCount + 1);
        ASSERT_EQ(registry.Get<LifetimeComp>(entity0).value, firstValue);

        registry.Clear();
        ASSERT_EQ(LifetimeComp::instanceCount, baselineInstanceCount);
    }
    ASSERT_EQ(LifetimeComp::instanceCount, baselineInstanceCount);
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

    std::unordered_set expected = { entity0, entity1 };
    const auto view0 = registry.ConstRuntimeView(
        RuntimeFilter()
            .IncludeDyn(compAClass));
    ASSERT_EQ(view0.Count(), 2);
    view0.Each([&](Entity e, const CompA& compA) -> void {
        ASSERT_TRUE(expected.contains(e));
    });

    const auto entity2 = registry.Create();
    registry.EmplaceDyn(compAClass, entity2, Mirror::ForwardAsArgList(3));
    registry.EmplaceDyn(compBClass, entity2, Mirror::ForwardAsArgList(4.0f));
    const auto entity3 = registry.Create();
    registry.EmplaceDyn(compBClass, entity3, Mirror::ForwardAsArgList(5.0f));

    const auto view1 = registry.RuntimeView(
        RuntimeFilter()
            .Include<CompA>());
    expected = { entity0, entity1, entity2 };
    ASSERT_EQ(view1.Count(), 3);
    for (const auto& entity : view1) {
        ASSERT_TRUE(expected.contains(entity));
    }

    const auto view2 = registry.ConstRuntimeView(
        RuntimeFilter()
            .Include<CompA>()
            .Exclude<CompB>());
    expected = { entity0, entity1 };
    ASSERT_EQ(view2.Count(), 2);
    view2.Each([&](Entity e, const CompA& compA) -> void {
        ASSERT_TRUE(expected.contains(e));
    });

    const auto view3 = registry.RuntimeView(
        RuntimeFilter()
            .Include<CompB>());
    expected = { entity2, entity3 };
    ASSERT_EQ(view3.Count(), 2);
    view3.Each([&](Entity e, const CompB& compB) -> void {
        ASSERT_TRUE(expected.contains(e));
    });

    const auto view4 = registry.ConstRuntimeView(
        RuntimeFilter()
            .Include<CompB>()
            .Exclude<CompA>());
    expected = { entity3 };
    ASSERT_EQ(view4.Count(), 1);
    view4.Each([&](Entity e, const CompB& compB) -> void {
        ASSERT_TRUE(expected.contains(e));
    });

    const auto view5 = constRegistry.ConstRuntimeView(
        RuntimeFilter()
            .Include<CompA>()
            .Include<CompB>());
    expected = { entity2 };
    ASSERT_EQ(view5.Count(), 1);
    view5.Each([&](Entity e, const CompA& compA, const CompB& compB) -> void {
        ASSERT_TRUE(expected.contains(e));
    });

    registry.RemoveDyn(compAClass, entity0);
    ASSERT_FALSE(registry.HasDyn(compAClass, entity0));
    ASSERT_TRUE(registry.FindDyn(compAClass, entity0).Empty());
}

TEST(ECSTest, GlobalComponentStaticTest)
{
    ECRegistry registry;
    registry.GEmplace<GCompA>(1);
    ASSERT_TRUE(registry.GHas<GCompA>());
    ASSERT_EQ(registry.GFind<GCompA>()->value, 1);
    ASSERT_EQ(registry.GGet<GCompA>().value, 1);

    const ECRegistry& constRegistry = registry;
    ASSERT_TRUE(constRegistry.GHas<GCompA>());
    ASSERT_EQ(constRegistry.GFind<GCompA>()->value, 1);
    ASSERT_EQ(constRegistry.GGet<GCompA>().value, 1);

    registry.GEmplace<GCompB>(2.0f);
    ASSERT_TRUE(registry.GHas<GCompA>());
    ASSERT_TRUE(registry.GHas<GCompB>());
    ASSERT_EQ(registry.GGet<GCompB>().value, 2.0f);

    registry.GRemove<GCompB>();
    ASSERT_TRUE(registry.GHas<GCompA>());
    ASSERT_FALSE(registry.GHas<GCompB>());
}

TEST(ECSTest, GlobalComponentDynamicTest)
{
    GCompClass gCompAClass = &Mirror::Class::Get<GCompA>();
    GCompClass gCompBClass = &Mirror::Class::Get<GCompB>();

    ECRegistry registry;
    registry.GEmplaceDyn(gCompAClass, Mirror::ForwardAsArgList(1));
    ASSERT_TRUE(registry.GHasDyn(gCompAClass));
    ASSERT_EQ(registry.GFindDyn(gCompAClass).As<GCompA&>().value, 1);
    ASSERT_EQ(registry.GGetDyn(gCompAClass).As<GCompA&>().value, 1);

    const ECRegistry& constRegistry = registry;
    ASSERT_TRUE(constRegistry.GHasDyn(gCompAClass));
    ASSERT_EQ(constRegistry.GFindDyn(gCompAClass).As<const GCompA&>().value, 1);
    ASSERT_EQ(constRegistry.GGetDyn(gCompAClass).As<const GCompA&>().value, 1);

    registry.GEmplaceDyn(gCompBClass, Mirror::ForwardAsArgList(2.0f));
    ASSERT_TRUE(registry.GHasDyn(gCompAClass));
    ASSERT_TRUE(registry.GHasDyn(gCompBClass));
    ASSERT_EQ(registry.GGetDyn(gCompBClass).As<const GCompB&>().value, 2.0f);

    registry.GRemoveDyn(gCompBClass);
    ASSERT_TRUE(registry.GHasDyn(gCompAClass));
    ASSERT_FALSE(registry.GHasDyn(gCompBClass));
}

TEST(ECSTest, ComponentEventStaticTest)
{
    EventCounts count;
    ECRegistry registry;
    const auto constructedCallback = registry.Events<CompA>().onConstructed.BindLambda([&](ECRegistry&, Entity) -> void { count.onConstructed++; });
    const auto updatedCallback = registry.Events<CompA>().onUpdated.BindLambda([&](ECRegistry&, Entity) -> void { count.onUpdated++; });
    const auto removeCallback = registry.Events<CompA>().onRemove.BindLambda([&](ECRegistry&, Entity) -> void { count.onRemove++; });

    const auto entity0 = registry.Create();
    const auto entity1 = registry.Create();
    registry.Emplace<CompA>(entity0, 1);
    ASSERT_EQ(count, EventCounts(1, 0, 0));
    registry.Emplace<CompA>(entity1, 2);
    ASSERT_EQ(count, EventCounts(2, 0, 0));

    {
        const auto updater = registry.Update<CompA>(entity0);
        updater->value = 3;
    }
    ASSERT_EQ(count, EventCounts(2, 1, 0));
    registry.Update<CompA>(entity0, [](CompA& compA) -> void {
        compA.value = 4;
    });
    ASSERT_EQ(count, EventCounts(2, 2, 0));
    registry.Get<CompA>(entity1).value = 5;
    registry.NotifyUpdated<CompA>(entity1);
    ASSERT_EQ(count, EventCounts(2, 3, 0));

    registry.Remove<CompA>(entity0);
    ASSERT_EQ(count, EventCounts(2, 3, 1));
    registry.Remove<CompA>(entity1);
    ASSERT_EQ(count, EventCounts(2, 3, 2));

    const auto entity2 = registry.Create();
    registry.Emplace<CompA>(entity2, 6);
    registry.Destroy(entity2);
    ASSERT_EQ(count, EventCounts(3, 3, 3));

    registry.Events<CompA>().onConstructed.Unbind(constructedCallback);
    registry.Events<CompA>().onUpdated.Unbind(updatedCallback);
    registry.Events<CompA>().onRemove.Unbind(removeCallback);
}

TEST(ECSTest, ComponentEventDynamicTest)
{
    CompClass compAClass = &Mirror::Class::Get<CompA>();
    EventCounts count;
    ECRegistry registry;
    const auto constructedCallback = registry.EventsDyn(compAClass).onConstructed.BindLambda([&](ECRegistry&, Entity) -> void { count.onConstructed++; });
    const auto updatedCallback = registry.EventsDyn(compAClass).onUpdated.BindLambda([&](ECRegistry&, Entity) -> void { count.onUpdated++; });
    const auto removeCallback = registry.EventsDyn(compAClass).onRemove.BindLambda([&](ECRegistry&, Entity) -> void { count.onRemove++; });

    const auto entity0 = registry.Create();
    const auto entity1 = registry.Create();
    registry.EmplaceDyn(compAClass, entity0, Mirror::ForwardAsArgList(1));
    ASSERT_EQ(count, EventCounts(1, 0, 0));
    registry.EmplaceDyn(compAClass, entity1, Mirror::ForwardAsArgList(2));
    ASSERT_EQ(count, EventCounts(2, 0, 0));

    {
        const auto updater = registry.UpdateDyn(compAClass, entity0);
        updater.As<CompA&>().value = 3;
    }
    ASSERT_EQ(count, EventCounts(2, 1, 0));
    registry.UpdateDyn(compAClass, entity0, [](const Mirror::Any& ref) -> void {
        ref.As<CompA&>().value = 4;
    });
    ASSERT_EQ(count, EventCounts(2, 2, 0));
    registry.GetDyn(compAClass, entity1).As<CompA&>().value = 5;
    registry.NotifyUpdatedDyn(compAClass, entity1);
    ASSERT_EQ(count, EventCounts(2, 3, 0));

    registry.RemoveDyn(compAClass, entity0);
    ASSERT_EQ(count, EventCounts(2, 3, 1));
    registry.RemoveDyn(compAClass, entity1);
    ASSERT_EQ(count, EventCounts(2, 3, 2));

    registry.EventsDyn(compAClass).onConstructed.Unbind(constructedCallback);
    registry.EventsDyn(compAClass).onUpdated.Unbind(updatedCallback);
    registry.EventsDyn(compAClass).onRemove.Unbind(removeCallback);
}

TEST(ECSTest, TagEventTest)
{
    EventCounts count;
    ECRegistry registry;
    const auto constructedCallback = registry.Events<TestTag>().onConstructed.BindLambda([&](ECRegistry&, Entity) -> void { count.onConstructed++; });
    const auto removeCallback = registry.Events<TestTag>().onRemove.BindLambda([&](ECRegistry&, Entity) -> void { count.onRemove++; });

    const auto entity0 = registry.Create();
    const auto entity1 = registry.Create();
    registry.AddTag<TestTag>(entity0);
    registry.AddTag<TestTag>(entity1);
    registry.RemoveTag<TestTag>(entity0);
    registry.Destroy(entity1);

    ASSERT_EQ(count, EventCounts(2, 0, 2));
    registry.Events<TestTag>().onConstructed.Unbind(constructedCallback);
    registry.Events<TestTag>().onRemove.Unbind(removeCallback);
}

TEST(ECSTest, GlobalComponentEventStaticTest)
{
    EventCounts count;
    ECRegistry registry;
    const auto constructedCallback = registry.GEvents<GCompA>().onConstructed.BindLambda([&](ECRegistry&) -> void { count.onConstructed++; });
    const auto updatedCallback = registry.GEvents<GCompA>().onUpdated.BindLambda([&](ECRegistry&) -> void { count.onUpdated++; });
    const auto removeCallback = registry.GEvents<GCompA>().onRemove.BindLambda([&](ECRegistry&) -> void { count.onRemove++; });

    registry.GEmplace<GCompA>(1);
    ASSERT_EQ(count, EventCounts(1, 0, 0));

    {
        const auto updater = registry.GUpdate<GCompA>();
        updater->value = 3;
    }
    ASSERT_EQ(count, EventCounts(1, 1, 0));
    registry.GUpdate<GCompA>([](GCompA& gCompA) -> void {
        gCompA.value = 4;
    });
    ASSERT_EQ(count, EventCounts(1, 2, 0));
    registry.GGet<GCompA>().value = 5;
    registry.GNotifyUpdated<GCompA>();
    ASSERT_EQ(count, EventCounts(1, 3, 0));

    registry.GRemove<GCompA>();
    ASSERT_EQ(count, EventCounts(1, 3, 1));

    registry.GEvents<GCompA>().onConstructed.Unbind(constructedCallback);
    registry.GEvents<GCompA>().onUpdated.Unbind(updatedCallback);
    registry.GEvents<GCompA>().onRemove.Unbind(removeCallback);
}

TEST(ECSTest, GlobalComponentEventDynamicTest)
{
    GCompClass gCompAClass = &Mirror::Class::Get<GCompA>();
    EventCounts count;
    ECRegistry registry;
    const auto constructedCallback = registry.GEventsDyn(gCompAClass).onConstructed.BindLambda([&](ECRegistry&) -> void { count.onConstructed++; });
    const auto updatedCallback = registry.GEventsDyn(gCompAClass).onUpdated.BindLambda([&](ECRegistry&) -> void { count.onUpdated++; });
    const auto removeCallback = registry.GEventsDyn(gCompAClass).onRemove.BindLambda([&](ECRegistry&) -> void { count.onRemove++; });

    registry.GEmplaceDyn(gCompAClass, Mirror::ForwardAsArgList(1));
    ASSERT_EQ(count, EventCounts(1, 0, 0));

    {
        const auto updater = registry.GUpdateDyn(gCompAClass);
        updater.As<GCompA&>().value = 3;
    }
    ASSERT_EQ(count, EventCounts(1, 1, 0));
    registry.GUpdateDyn(gCompAClass, [](const Mirror::Any& ref) -> void {
        ref.As<GCompA&>().value = 4;
    });
    ASSERT_EQ(count, EventCounts(1, 2, 0));
    registry.GGetDyn(gCompAClass).As<GCompA&>().value = 5;
    registry.GNotifyUpdatedDyn(gCompAClass);
    ASSERT_EQ(count, EventCounts(1, 3, 0));

    registry.GRemove<GCompA>();
    ASSERT_EQ(count, EventCounts(1, 3, 1));

    registry.GEventsDyn(gCompAClass).onConstructed.Unbind(constructedCallback);
    registry.GEventsDyn(gCompAClass).onUpdated.Unbind(updatedCallback);
    registry.GEventsDyn(gCompAClass).onRemove.Unbind(removeCallback);
}

TEST(ECSTest, ObserverTest)
{
    ECRegistry registry;
    auto observer = registry.Observer();
    observer
        .ObConstructed<CompA>()
        .ObUpdatedDyn(&Mirror::Class::Get<CompB>());

    const auto entity0 = registry.Create();
    ASSERT_EQ(observer.Count(), 0);

    registry.Emplace<CompA>(entity0, 1);
    ASSERT_EQ(observer.Count(), 1);
    std::unordered_set expected = { entity0 };
    observer.Each([&](Entity e) -> void {
        ASSERT_TRUE(expected.contains(e));
    });

    const auto entity1 = registry.Create();
    registry.Emplace<CompB>(entity1, 2.0f);
    ASSERT_EQ(observer.Count(), 1);
    registry.Update<CompB>(entity1, [](CompB& compB) -> void {
        compB.value = 3.0f;
    });
    ASSERT_EQ(observer.Count(), 2);
    expected = { entity0, entity1 };
    for (const auto e : observer) {
        ASSERT_TRUE(expected.contains(e));
    }

    const auto popped = observer.Pop();
    ASSERT_EQ(observer.Count(), 0);
    ASSERT_EQ(std::unordered_set<Entity>(popped.begin(), popped.end()), expected);

    registry.Update<CompB>(entity1, [](CompB& compB) -> void {
        compB.value = 4.0f;
    });
    size_t traversedCount = 0;
    observer.EachThenClear([&](Entity e) -> void {
        ASSERT_EQ(e, entity1);
        traversedCount++;
    });
    ASSERT_EQ(traversedCount, 1);
    ASSERT_EQ(observer.Count(), 0);
}

TEST(ECSTest, EventsObserverStaticTest)
{
    ECRegistry registry;
    auto observer = registry.EventsObserver<CompA>();
    const auto entity = registry.Create();

    registry.Emplace<CompA>(entity, 1);
    ASSERT_EQ(observer.ConstructedCount(), 1);
    ASSERT_EQ(observer.UpdatedCount(), 0);
    ASSERT_EQ(observer.RemovedCount(), 0);

    registry.Update<CompA>(entity, [](CompA& compA) -> void {
        compA.value = 2;
    });
    ASSERT_EQ(observer.ConstructedCount(), 1);
    ASSERT_EQ(observer.UpdatedCount(), 1);
    ASSERT_EQ(observer.RemovedCount(), 0);

    registry.Remove<CompA>(entity);
    ASSERT_EQ(observer.Constructed().All(), std::vector<Entity> { entity });
    ASSERT_EQ(observer.Updated().All(), std::vector<Entity> { entity });
    ASSERT_EQ(observer.Removed().All(), std::vector<Entity> { entity });

    observer.ClearUpdated();
    ASSERT_EQ(observer.ConstructedCount(), 1);
    ASSERT_EQ(observer.UpdatedCount(), 0);
    ASSERT_EQ(observer.RemovedCount(), 1);
    observer.Clear();
    ASSERT_EQ(observer.ConstructedCount(), 0);
    ASSERT_EQ(observer.RemovedCount(), 0);
}

TEST(ECSTest, EventsObserverDynamicTest)
{
    const auto* compClass = &Mirror::Class::Get<CompA>();
    ECRegistry registry;
    auto observer = registry.EventsObserverDyn(compClass);
    const auto entity = registry.Create();

    registry.EmplaceDyn(compClass, entity, Mirror::ForwardAsArgList(1));
    registry.NotifyUpdatedDyn(compClass, entity);
    registry.Destroy(entity);

    ASSERT_EQ(observer.ConstructedCount(), 1);
    ASSERT_EQ(observer.UpdatedCount(), 1);
    ASSERT_EQ(observer.RemovedCount(), 1);
    ASSERT_EQ(observer.Constructed().All(), std::vector<Entity> { entity });
    ASSERT_EQ(observer.Updated().All(), std::vector<Entity> { entity });
    ASSERT_EQ(observer.Removed().All(), std::vector<Entity> { entity });
}

TEST(ECSTest, ECRegistryCopyTest)
{
    ECRegistry registry0;
    const auto entity0 = registry0.Create();
    const auto entity1 = registry0.Create();
    registry0.Emplace<CompA>(entity0, 1);
    registry0.Emplace<CompB>(entity1, 2.0f);
    registry0.AddTag<TestTag>(entity0);

    ECRegistry registry1 = registry0;
    ASSERT_EQ(registry1.Get<CompA>(entity0).value, 1);
    ASSERT_EQ(registry1.Get<CompB>(entity1).value, 2.0f);
    ASSERT_TRUE(registry1.HasTag<TestTag>(entity0));

    registry1.Remove<CompA>(entity0);
    registry1.Emplace<CompB>(entity0, 3.0f);
    ASSERT_FALSE(registry1.Has<CompA>(entity0));
    ASSERT_EQ(registry1.Get<CompB>(entity0).value, 3.0f);
    ASSERT_EQ(registry0.Get<CompA>(entity0).value, 1);
}

TEST(ECSTest, ECRegistryValueSemanticsTest)
{
    const auto baselineInstanceCount = LifetimeComp::instanceCount;
    const std::string originalValue = "original component value that does not use the small string optimization";
    {
        ECRegistry registry0;
        const auto entity = registry0.Create();
        registry0.Emplace<LifetimeComp>(entity, std::string(originalValue));
        ASSERT_EQ(LifetimeComp::instanceCount, baselineInstanceCount + 1);

        {
            ECRegistry registry1 = registry0;
            ASSERT_EQ(LifetimeComp::instanceCount, baselineInstanceCount + 2);
            registry1.Get<LifetimeComp>(entity).value = "copied registry component with independent string storage";
            ASSERT_EQ(registry0.Get<LifetimeComp>(entity).value, originalValue);

            ECRegistry registry2;
            const auto replacedEntity = registry2.Create();
            registry2.Emplace<LifetimeComp>(replacedEntity, std::string("component replaced by copy assignment"));
            ASSERT_EQ(LifetimeComp::instanceCount, baselineInstanceCount + 3);
            registry2 = registry0;
            ASSERT_EQ(LifetimeComp::instanceCount, baselineInstanceCount + 3);
            ASSERT_EQ(registry2.Get<LifetimeComp>(entity).value, registry0.Get<LifetimeComp>(entity).value);
            registry2.Emplace<CompA>(entity, 1);

            ECRegistry registry3 = std::move(registry2);
            ASSERT_EQ(LifetimeComp::instanceCount, baselineInstanceCount + 3);
            ASSERT_EQ(registry3.Get<LifetimeComp>(entity).value, registry0.Get<LifetimeComp>(entity).value);
            registry3.Remove<CompA>(entity);
            ASSERT_FALSE(registry3.Has<CompA>(entity));
        }
        ASSERT_EQ(LifetimeComp::instanceCount, baselineInstanceCount + 1);
    }
    ASSERT_EQ(LifetimeComp::instanceCount, baselineInstanceCount);
}

TEST(ECSTest, ECSRegistrySaveLoadTest)
{
    ECArchive archive;
    Entity emptyEntity;
    {
        ECRegistry registry;
        const auto entity0 = registry.Create();
        const auto entity1 = registry.Create();
        const Entity discardedEntity = registry.Create();
        registry.Destroy(discardedEntity);
        emptyEntity = registry.Create();
        ASSERT_NE(emptyEntity, discardedEntity);
        registry.Emplace<CompA>(entity0, 1);
        registry.Emplace<CompB>(entity1, 2.0f);
        registry.AddTag<TestTag>(entity0);
        registry.GEmplace<GCompA>(1);
        registry.GEmplace<GCompB>(2.0f);
        const auto transientEntity = registry.Create();
        registry.Emplace<CompA>(transientEntity, 3);
        registry.AddTag<TransientTag>(transientEntity);

        registry.Save(archive);
    }

    {
        ECRegistry registry;
        registry.Load(archive);

        ASSERT_EQ(registry.Count(), 3);
        ASSERT_EQ(registry.Get<CompA>(1u).value, 1);
        ASSERT_TRUE(registry.HasTag<TestTag>(1u));
        ASSERT_EQ(registry.TagCount(1u), 1);
        ASSERT_EQ(registry.Get<CompB>(2u).value, 2.0f);
        ASSERT_TRUE(registry.Valid(emptyEntity));
        ASSERT_EQ(registry.CompCount(emptyEntity), 0);
        ASSERT_EQ(registry.GGet<GCompA>().value, 1);
        ASSERT_EQ(registry.GGet<GCompB>().value, 2.0f);
        ASSERT_EQ(registry.GCompCount(), 2);
    }
}
