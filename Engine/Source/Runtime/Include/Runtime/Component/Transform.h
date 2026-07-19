//
// Created by johnk on 2024/10/14.
//

#pragma once

#include <Common/Math/Adapters.h>
#include <Runtime/Meta.h>
#include <Runtime/ECS.h>
#include <Runtime/Api.h>

namespace Runtime {
    struct RUNTIME_API EClass(comp) WorldTransform final {
        EClassBody(WorldTransform)

        WorldTransform();
        explicit WorldTransform(Common::FTransform inLocalToWorld);

        EProperty() Common::FTransform localToWorld;
    };

    // must be used with Hierarchy and WorldTransform
    struct RUNTIME_API EClass(comp) LocalTransform final {
        EClassBody(LocalTransform)

        LocalTransform();
        explicit LocalTransform(Common::FTransform inLocalToParent);

        EProperty() Common::FTransform localToParent;
    };

    struct RUNTIME_API EClass(comp) Hierarchy final {
        EClassBody(Hierarchy)

        Hierarchy();

        EProperty() Entity parent;
        EProperty() Entity firstChild;
        EProperty() Entity prevBro;
        EProperty() Entity nextBro;
    };

    class RUNTIME_API HierarchyOps {
    public:
        using TraverseFunc = std::function<void(Entity, Entity)>;

        static bool HasParent(ECRegistry& inRegistry, Entity inTarget);
        static bool HasBro(ECRegistry& inRegistry, Entity inTarget);
        static bool HasChildren(ECRegistry& inRegistry, Entity inTarget);
        static void AttachToParent(ECRegistry& inRegistry, Entity inChild, Entity inParent);
        static void DetachFromParent(ECRegistry& inRegistry, Entity inChild);
        static void Remove(ECRegistry& inRegistry, Entity inTarget);
        static void Destroy(ECRegistry& inRegistry, Entity inTarget);
        static void TraverseChildren(ECRegistry& inRegistry, Entity inParent, const TraverseFunc& inFunc);
        static void TraverseChildrenRecursively(ECRegistry& inRegistry, Entity inParent, const TraverseFunc& inFunc);
    };
}
