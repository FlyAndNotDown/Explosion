//
// Created by johnk on 2024/10/14.
//

#include <utility>

#include <Runtime/Component/Transform.h>

namespace Runtime::Internal {
    static bool IsAncestor(ECRegistry& inRegistry, Entity inAncestor, Entity inEntity)
    {
        for (Entity current = inEntity; current != entityNull; current = inRegistry.Get<Hierarchy>(current).parent) {
            if (current == inAncestor) {
                return true;
            }
        }
        return false;
    }
}

namespace Runtime {
    WorldTransform::WorldTransform() = default;

    WorldTransform::WorldTransform(Common::FTransform inLocalToWorld)
        : localToWorld(std::move(inLocalToWorld))
    {
    }

    LocalTransform::LocalTransform() = default;

    LocalTransform::LocalTransform(Common::FTransform inLocalToParent)
        : localToParent(std::move(inLocalToParent))
    {
    }

    Hierarchy::Hierarchy()
        : parent(entityNull)
        , firstChild(entityNull)
        , prevBro(entityNull)
        , nextBro(entityNull)
    {
    }

    bool HierarchyOps::HasParent(ECRegistry& inRegistry, Entity inTarget)
    {
        const auto& hierarchy = inRegistry.Get<Hierarchy>(inTarget);
        return hierarchy.parent != entityNull;
    }

    bool HierarchyOps::HasBro(ECRegistry& inRegistry, Entity inTarget)
    {
        const auto& hierarchy = inRegistry.Get<Hierarchy>(inTarget);
        return hierarchy.prevBro != entityNull || hierarchy.nextBro != entityNull;
    }

    bool HierarchyOps::HasChildren(ECRegistry& inRegistry, Entity inTarget)
    {
        const auto& hierarchy = inRegistry.Get<Hierarchy>(inTarget);
        return hierarchy.firstChild != entityNull;
    }

    void HierarchyOps::AttachToParent(ECRegistry& inRegistry, Entity inChild, Entity inParent)
    {
        Assert(inRegistry.Valid(inChild) && inRegistry.Valid(inParent));
        Assert(inChild != inParent);
        Assert(inRegistry.Has<Hierarchy>(inChild) && inRegistry.Has<Hierarchy>(inParent));
        Assert(!HasParent(inRegistry, inChild) && !HasBro(inRegistry, inChild));
        Assert(!Internal::IsAncestor(inRegistry, inChild, inParent));

        auto& childHierarchy = inRegistry.Get<Hierarchy>(inChild);
        auto& parentHierarchy = inRegistry.Get<Hierarchy>(inParent);
        const Entity oldFirstChild = parentHierarchy.firstChild;
        childHierarchy.parent = inParent;
        childHierarchy.nextBro = oldFirstChild;
        if (oldFirstChild != entityNull) {
            auto& oldFirstChildHierarchy = inRegistry.Get<Hierarchy>(oldFirstChild);
            oldFirstChildHierarchy.prevBro = inChild;
            inRegistry.NotifyUpdated<Hierarchy>(oldFirstChild);
        }
        parentHierarchy.firstChild = inChild;
        inRegistry.NotifyUpdated<Hierarchy>(inChild);
        inRegistry.NotifyUpdated<Hierarchy>(inParent);
    }

    void HierarchyOps::DetachFromParent(ECRegistry& inRegistry, Entity inChild)
    {
        Assert(HasParent(inRegistry, inChild));
        auto& childHierarchy = inRegistry.Get<Hierarchy>(inChild);
        const Entity parent = childHierarchy.parent;
        auto& parentHierarchy = inRegistry.Get<Hierarchy>(parent);

        if (parentHierarchy.firstChild == inChild) {
            Assert(childHierarchy.prevBro == entityNull);
            parentHierarchy.firstChild = childHierarchy.nextBro;
            if (childHierarchy.nextBro != entityNull) {
                const Entity nextBro = childHierarchy.nextBro;
                auto& nextBroHierarchy = inRegistry.Get<Hierarchy>(nextBro);
                nextBroHierarchy.prevBro = entityNull;
                inRegistry.NotifyUpdated<Hierarchy>(nextBro);
            }
            childHierarchy.nextBro = entityNull;
        } else {
            Assert(childHierarchy.prevBro != entityNull);
            const auto prevBro = childHierarchy.prevBro;
            const auto nextBro = childHierarchy.nextBro;
            auto& prevBroHierarchy = inRegistry.Get<Hierarchy>(prevBro);
            Assert(prevBroHierarchy.nextBro == inChild);
            prevBroHierarchy.nextBro = nextBro;
            if (nextBro != entityNull) {
                auto& nextBroHierarchy = inRegistry.Get<Hierarchy>(nextBro);
                Assert(nextBroHierarchy.prevBro == inChild);
                nextBroHierarchy.prevBro = prevBro;
                inRegistry.NotifyUpdated<Hierarchy>(nextBro);
            }
            childHierarchy.prevBro = entityNull;
            childHierarchy.nextBro = entityNull;
            inRegistry.NotifyUpdated<Hierarchy>(prevBro);
        }
        childHierarchy.parent = entityNull;
        inRegistry.NotifyUpdated<Hierarchy>(inChild);
        inRegistry.NotifyUpdated<Hierarchy>(parent);
    }

    void HierarchyOps::Remove(ECRegistry& inRegistry, Entity inTarget)
    {
        Assert(inRegistry.Valid(inTarget) && inRegistry.Has<Hierarchy>(inTarget));
        while (HasChildren(inRegistry, inTarget)) {
            DetachFromParent(inRegistry, inRegistry.Get<Hierarchy>(inTarget).firstChild);
        }
        if (HasParent(inRegistry, inTarget)) {
            DetachFromParent(inRegistry, inTarget);
        }
        inRegistry.Remove<Hierarchy>(inTarget);
    }

    void HierarchyOps::Destroy(ECRegistry& inRegistry, Entity inTarget)
    {
        Assert(inRegistry.Valid(inTarget));
        if (inRegistry.Has<Hierarchy>(inTarget)) {
            Remove(inRegistry, inTarget);
        }
        inRegistry.Destroy(inTarget);
    }

    void HierarchyOps::TraverseChildren(ECRegistry& inRegistry, Entity inParent, const TraverseFunc& inFunc)
    {
        const auto& parentHierarchy = inRegistry.Get<Hierarchy>(inParent);
        for (auto child = parentHierarchy.firstChild; child != entityNull;) {
            const auto& childHierarchy = inRegistry.Get<Hierarchy>(child);
            inFunc(child, inParent);
            child = childHierarchy.nextBro;
        }
    }

    void HierarchyOps::TraverseChildrenRecursively(ECRegistry& inRegistry, Entity inParent, const TraverseFunc& inFunc) // NOLINT
    {
        const auto& parentHierarchy = inRegistry.Get<Hierarchy>(inParent);
        for (auto child = parentHierarchy.firstChild; child != entityNull;) {
            const auto& childHierarchy = inRegistry.Get<Hierarchy>(child);
            inFunc(child, inParent);
            TraverseChildrenRecursively(inRegistry, child, inFunc);
            child = childHierarchy.nextBro;
        }
    }
}
