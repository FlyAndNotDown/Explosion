//
// Created by johnk on 2024/10/31.
//

#include <taskflow/taskflow.hpp>

#include <cstddef>
#include <new>
#include <utility>

#include <Core/Thread.h>
#include <Runtime/ECS.h>

namespace Runtime {
    System::System(ECRegistry& inRegistry, const SystemSetupContext&)
        : registry(inRegistry)
    {
    }

    System::~System() = default;

    void System::Tick(float inDeltaTimeSeconds) {}
}

namespace Runtime::Internal {
    static bool IsGlobalCompClass(GCompClass inClass)
    {
        return inClass->GetMetaBoolOr(MetaPresets::globalComp, false);
    }

    CompRtti::CompRtti(CompClass inClass)
        : clazz(inClass)
        , copyConstructFrom(nullptr)
        , moveConstructFrom(nullptr)
        , destruct(nullptr)
    {
    }

    Mirror::Any CompRtti::CopyConstruct(ElemPtr inElem, const Mirror::Any& inOther) const
    {
        return clazz->GetConstructor(Mirror::IdPresets::copyCtor).InplaceNewDyn(inElem, { inOther.ConstRef() });
    }

    Mirror::Any CompRtti::MoveConstruct(ElemPtr inElem, const Mirror::Any& inOther) const
    {
        if (clazz->HasConstructor(Mirror::IdPresets::moveCtor)) {
            return clazz->GetConstructor(Mirror::IdPresets::moveCtor).InplaceNewDyn(inElem, { inOther });
        }
        return clazz->GetConstructor(Mirror::IdPresets::copyCtor).InplaceNewDyn(inElem, { inOther.ConstRef() });
    }

    void CompRtti::CopyConstructFrom(ElemPtr inDstElem, ElemPtr inSrcElem) const
    {
        if (copyConstructFrom != nullptr) {
            copyConstructFrom(inDstElem, inSrcElem);
        } else {
            CopyConstruct(inDstElem, Get(inSrcElem));
        }
    }

    void CompRtti::MoveConstructFrom(ElemPtr inDstElem, ElemPtr inSrcElem) const
    {
        if (moveConstructFrom != nullptr) {
            moveConstructFrom(inDstElem, inSrcElem);
        } else {
            MoveConstruct(inDstElem, Get(inSrcElem));
        }
    }

    void CompRtti::Destruct(ElemPtr inElem) const
    {
        if (destruct != nullptr) {
            destruct(inElem);
        } else {
            clazz->DestructDyn(clazz->InplaceGetObject(inElem));
        }
    }

    Mirror::Any CompRtti::Get(ElemPtr inElem) const
    {
        return clazz->InplaceGetObject(inElem);
    }

    size_t CompRtti::MemorySize() const
    {
        return clazz->SizeOf();
    }

    size_t CompRtti::MemoryAlignment() const
    {
        return clazz->AlignOf();
    }

    Archetype::Archetype(const std::vector<CompRtti>& inRttiVec)
        : id(0)
        , count(0)
        , capacity(0)
        , rttiVec(inRttiVec)
        , compMemory(rttiVec.size(), nullptr)
        , compStrides(rttiVec.size())
    {
        rttiMap.reserve(rttiVec.size());
        for (auto i = 0; i < rttiVec.size(); i++) {
            auto& rtti = rttiVec[i];
            const auto clazz = rtti.Class();
            rttiMap.emplace(clazz, i);
            compStrides[i] = rtti.MemorySize();

            id += clazz->GetTypeInfo()->id;
        }
    }

    Archetype::~Archetype()
    {
        DestroyElements();
        ReleaseMemory();
    }

    Archetype::Archetype(const Archetype& inOther)
        : id(inOther.id)
        , count(inOther.count)
        , capacity(inOther.capacity)
        , rttiVec(inOther.rttiVec)
        , rttiMap(inOther.rttiMap)
        , compMemory(rttiVec.size(), nullptr)
        , compStrides(inOther.compStrides)
        , elemMap(inOther.elemMap)
    {
        for (size_t compIndex = 0; compIndex < rttiVec.size(); compIndex++) {
            const auto& rtti = rttiVec[compIndex];
            if (capacity > 0) {
                compMemory[compIndex] = ::operator new(capacity * rtti.MemorySize(), std::align_val_t(rtti.MemoryAlignment()));
            }
            for (size_t elemIndex = 0; elemIndex < count; elemIndex++) {
                rtti.CopyConstructFrom(GetCompAt(elemIndex, compIndex), const_cast<void*>(inOther.GetCompAt(elemIndex, compIndex)));
            }
        }
    }

    Archetype::Archetype(Archetype&& inOther) noexcept
        : id(inOther.id)
        , count(std::exchange(inOther.count, 0))
        , capacity(std::exchange(inOther.capacity, 0))
        , rttiVec(std::move(inOther.rttiVec))
        , rttiMap(std::move(inOther.rttiMap))
        , compMemory(std::move(inOther.compMemory))
        , compStrides(std::move(inOther.compStrides))
        , elemMap(std::move(inOther.elemMap))
    {
    }

    Archetype& Archetype::operator=(const Archetype& inOther)
    {
        if (this != &inOther) {
            *this = Archetype(inOther);
        }
        return *this;
    }

    Archetype& Archetype::operator=(Archetype&& inOther) noexcept
    {
        if (this == &inOther) {
            return *this;
        }

        DestroyElements();
        ReleaseMemory();
        id = inOther.id;
        count = std::exchange(inOther.count, 0);
        capacity = std::exchange(inOther.capacity, 0);
        rttiVec = std::move(inOther.rttiVec);
        rttiMap = std::move(inOther.rttiMap);
        compMemory = std::move(inOther.compMemory);
        compStrides = std::move(inOther.compStrides);
        elemMap = std::move(inOther.elemMap);
        addTransitions.clear();
        removeTransitions.clear();
        return *this;
    }

    bool Archetype::Contains(CompClass inClazz) const
    {
        for (const auto& rtti : rttiVec) {
            if (rtti.Class() == inClazz) {
                return true;
            }
        }
        return false;
    }

    bool Archetype::ContainsAll(const std::vector<CompClass>& inClasses) const
    {
        for (const auto& clazz : inClasses) {
            if (!Contains(clazz)) {
                return false;
            }
        }
        return true;
    }

    bool Archetype::NotContainsAny(const std::vector<CompClass>& inClasses) const
    {
        for (const auto& clazz : inClasses) {
            if (Contains(clazz)) {
                return false;
            }
        }
        return true;
    }

    size_t Archetype::EmplaceElem(Entity inEntity)
    {
        AllocateNewElemBack();
        elemMap.emplace_back(inEntity);
        return count - 1;
    }

    size_t Archetype::EmplaceElem(Entity inEntity, Archetype& inSrcArchetype, size_t inSrcElemIndex)
    {
        const auto newElemIndex = EmplaceElem(inEntity);
        for (const auto& srcRtti : inSrcArchetype.rttiVec) {
            const auto* dstRtti = FindCompRtti(srcRtti.Class());
            if (dstRtti == nullptr) {
                continue;
            }
            const auto srcCompIndex = inSrcArchetype.GetCompIndex(srcRtti.Class());
            const auto dstCompIndex = GetCompIndex(srcRtti.Class());
            dstRtti->MoveConstructFrom(GetCompAt(newElemIndex, dstCompIndex), inSrcArchetype.GetCompAt(inSrcElemIndex, srcCompIndex));
        }
        return newElemIndex;
    }

    Mirror::Any Archetype::EmplaceComp(size_t inElemIndex, CompClass inCompClass, const Mirror::Any& inCompRef) // NOLINT
    {
        Assert(inElemIndex < count);
        const auto compIndex = GetCompIndex(inCompClass);
        return rttiVec[compIndex].MoveConstruct(GetCompAt(inElemIndex, compIndex), inCompRef);
    }

    Entity Archetype::EraseElem(size_t inElemIndex)
    {
        Assert(inElemIndex < count);
        const auto lastElemIndex = count - 1;
        Entity movedEntity = entityNull;

        if (inElemIndex == lastElemIndex) {
            for (size_t compIndex = 0; compIndex < rttiVec.size(); compIndex++) {
                rttiVec[compIndex].Destruct(GetCompAt(inElemIndex, compIndex));
            }
        } else {
            for (size_t compIndex = 0; compIndex < rttiVec.size(); compIndex++) {
                const auto& rtti = rttiVec[compIndex];
                ElemPtr elem = GetCompAt(inElemIndex, compIndex);
                ElemPtr lastElem = GetCompAt(lastElemIndex, compIndex);
                rtti.Destruct(elem);
                rtti.MoveConstructFrom(elem, lastElem);
                rtti.Destruct(lastElem);
            }

            const auto entityToLastElem = elemMap.at(lastElemIndex);
            elemMap[inElemIndex] = entityToLastElem;
            movedEntity = entityToLastElem;
        }

        elemMap.pop_back();
        count--;
        return movedEntity;
    }

    Mirror::Any Archetype::GetComp(size_t inElemIndex, CompClass inCompClass)
    {
        const auto compIndex = GetCompIndex(inCompClass);
        return rttiVec[compIndex].Get(GetCompAt(inElemIndex, compIndex));
    }

    Mirror::Any Archetype::GetComp(size_t inElemIndex, CompClass inCompClass) const
    {
        const auto compIndex = GetCompIndex(inCompClass);
        return rttiVec[compIndex].Get(const_cast<ElemPtr>(GetCompAt(inElemIndex, compIndex))).ConstRef();
    }

    size_t Archetype::GetCompIndex(CompClass inCompClass) const
    {
        const auto iter = rttiMap.find(inCompClass);
        Assert(iter != rttiMap.end());
        return iter->second;
    }

    size_t Archetype::Count() const
    {
        return count;
    }

    const std::vector<CompRtti>& Archetype::GetRttiVec() const
    {
        return rttiVec;
    }

    ArchetypeId Archetype::Id() const
    {
        return id;
    }

    std::vector<CompRtti> Archetype::NewRttiVecByAdd(const CompRtti& inRtti) const
    {
        auto result = rttiVec;
        result.emplace_back(inRtti);
        return result;
    }

    std::vector<CompRtti> Archetype::NewRttiVecByRemove(const CompRtti& inRtti) const
    {
        auto result = rttiVec;
        const auto iter = std::ranges::find_if(result, [&](const CompRtti& rtti) -> bool { return rtti.Class() == inRtti.Class(); });
        Assert(iter != result.end());
        result.erase(iter);
        return result;
    }

    Archetype* Archetype::FindAddTransition(CompClass inClass) const
    {
        const auto iter = std::ranges::find_if(addTransitions, [&](const auto& transition) -> bool { return transition.first == inClass; });
        return iter != addTransitions.end() ? iter->second : nullptr;
    }

    Archetype* Archetype::FindRemoveTransition(CompClass inClass) const
    {
        const auto iter = std::ranges::find_if(removeTransitions, [&](const auto& transition) -> bool { return transition.first == inClass; });
        return iter != removeTransitions.end() ? iter->second : nullptr;
    }

    void Archetype::CacheAddTransition(CompClass inClass, Archetype& inArchetype)
    {
        Assert(FindAddTransition(inClass) == nullptr);
        addTransitions.emplace_back(inClass, &inArchetype);
    }

    void Archetype::CacheRemoveTransition(CompClass inClass, Archetype& inArchetype)
    {
        Assert(FindRemoveTransition(inClass) == nullptr);
        removeTransitions.emplace_back(inClass, &inArchetype);
    }

    const CompRtti* Archetype::FindCompRtti(CompClass clazz) const
    {
        const auto iter = rttiMap.find(clazz);
        return iter != rttiMap.end() ? &rttiVec[iter->second] : nullptr;
    }

    size_t Archetype::Capacity() const
    {
        return capacity;
    }

    void Archetype::Reserve(float inRatio)
    {
        Assert(inRatio > 1.0f);
        const size_t newCapacity = static_cast<size_t>(std::ceil(static_cast<float>(std::max(Capacity(), static_cast<size_t>(1))) * inRatio));
        std::vector<ElemPtr> newCompMemory(rttiVec.size(), nullptr);

        for (size_t compIndex = 0; compIndex < rttiVec.size(); compIndex++) {
            const auto& rtti = rttiVec[compIndex];
            newCompMemory[compIndex] = ::operator new(newCapacity * rtti.MemorySize(), std::align_val_t(rtti.MemoryAlignment()));
            for (size_t elemIndex = 0; elemIndex < count; elemIndex++) {
                ElemPtr dstElem = CompAt(newCompMemory[compIndex], compIndex, elemIndex);
                ElemPtr srcElem = GetCompAt(elemIndex, compIndex);
                rtti.MoveConstructFrom(dstElem, srcElem);
                rtti.Destruct(srcElem);
            }
            if (compMemory[compIndex] != nullptr) {
                ::operator delete(compMemory[compIndex], std::align_val_t(rtti.MemoryAlignment()));
            }
        }
        compMemory = std::move(newCompMemory);
        capacity = newCapacity;
    }

    void Archetype::DestroyElements()
    {
        for (size_t compIndex = 0; compIndex < rttiVec.size(); compIndex++) {
            const auto& rtti = rttiVec[compIndex];
            for (size_t elemIndex = 0; elemIndex < count; elemIndex++) {
                rtti.Destruct(GetCompAt(elemIndex, compIndex));
            }
        }
        count = 0;
    }

    void Archetype::ReleaseMemory()
    {
        for (size_t compIndex = 0; compIndex < compMemory.size(); compIndex++) {
            if (compMemory[compIndex] != nullptr) {
                ::operator delete(compMemory[compIndex], std::align_val_t(rttiVec[compIndex].MemoryAlignment()));
                compMemory[compIndex] = nullptr;
            }
        }
        capacity = 0;
    }

    void Archetype::AllocateNewElemBack()
    {
        if (Count() == Capacity()) {
            Reserve();
        }
        count++;
    }

    EntityPool::EntityPool()
        : records(1)
    {
    }

    size_t EntityPool::Count() const
    {
        return allocated.size();
    }

    bool EntityPool::Valid(Entity inEntity) const
    {
        return inEntity < records.size() && records[inEntity].allocated;
    }

    Entity EntityPool::Allocate()
    {
        Entity result;
        if (!free.empty()) {
            result = free.back();
            free.pop_back();
        } else {
            result = static_cast<Entity>(records.size());
            records.emplace_back();
        }
        auto& record = records[result];
        record.allocated = true;
        record.archetype = 0;
        record.archetypePtr = nullptr;
        record.elemIndex = 0;
        record.allocatedIndex = allocated.size();
        allocated.emplace_back(result);
        return result;
    }

    void EntityPool::Allocate(Entity inEntity)
    {
        if (inEntity < records.size()) {
            Assert(!records[inEntity].allocated);
            const auto iter = std::ranges::find(free, inEntity);
            Assert(iter != free.end());
            *iter = free.back();
            free.pop_back();
        } else {
            for (Entity i = static_cast<Entity>(records.size()); i < inEntity; i++) {
                free.emplace_back(i);
            }
            records.resize(static_cast<size_t>(inEntity) + 1);
        }
        auto& record = records[inEntity];
        record.allocated = true;
        record.archetype = 0;
        record.archetypePtr = nullptr;
        record.elemIndex = 0;
        record.allocatedIndex = allocated.size();
        allocated.emplace_back(inEntity);
    }

    void EntityPool::Free(Entity inEntity)
    {
        Assert(Valid(inEntity));
        auto& record = records[inEntity];
        const auto lastEntity = allocated.back();
        allocated[record.allocatedIndex] = lastEntity;
        records[lastEntity].allocatedIndex = record.allocatedIndex;
        allocated.pop_back();
        record.allocated = false;
        record.archetype = 0;
        record.archetypePtr = nullptr;
        record.elemIndex = 0;
        free.emplace_back(inEntity);
    }

    void EntityPool::Clear()
    {
        records.clear();
        records.resize(1);
        free.clear();
        allocated.clear();
    }

    void EntityPool::Each(const EntityTraverseFunc& inFunc) const
    {
        for (const auto& entity : allocated) {
            inFunc(entity);
        }
    }

    void EntityPool::SetLocation(Entity inEntity, Archetype& inArchetype, size_t inElemIndex)
    {
        Assert(Valid(inEntity));
        records[inEntity].archetype = inArchetype.Id();
        records[inEntity].archetypePtr = &inArchetype;
        records[inEntity].elemIndex = inElemIndex;
    }

    void EntityPool::SetElemIndex(Entity inEntity, size_t inElemIndex)
    {
        Assert(Valid(inEntity));
        records[inEntity].elemIndex = inElemIndex;
    }

    void EntityPool::SetArchetypePtr(Entity inEntity, Archetype& inArchetype)
    {
        Assert(Valid(inEntity) && records[inEntity].archetype == inArchetype.Id());
        records[inEntity].archetypePtr = &inArchetype;
    }

    ArchetypeId EntityPool::GetArchetype(Entity inEntity) const
    {
        Assert(Valid(inEntity));
        return records[inEntity].archetype;
    }

    Archetype* EntityPool::GetArchetypePtr(Entity inEntity)
    {
        Assert(Valid(inEntity) && records[inEntity].archetypePtr != nullptr);
        return records[inEntity].archetypePtr;
    }

    const Archetype* EntityPool::GetArchetypePtr(Entity inEntity) const
    {
        Assert(Valid(inEntity) && records[inEntity].archetypePtr != nullptr);
        return records[inEntity].archetypePtr;
    }

    size_t EntityPool::GetElemIndex(Entity inEntity) const
    {
        Assert(Valid(inEntity));
        return records[inEntity].elemIndex;
    }

    EntityPool::ConstIter EntityPool::Begin() const
    {
        return allocated.begin();
    }

    EntityPool::ConstIter EntityPool::End() const
    {
        return allocated.end();
    }

    SystemFactory::SystemFactory(SystemClass inClass)
        : clazz(inClass)
    {
        BuildArgumentLists();
    }

    Common::UniquePtr<System> SystemFactory::Build(ECRegistry& inRegistry, const SystemSetupContext& inSetupContext) const
    {
        const Mirror::Any system = clazz->New(inRegistry, inSetupContext);
        const Mirror::Any systemRef = system.Deref();
        for (const auto& [name, argument] : arguments) {
            clazz->GetMemberVariable(name).SetDyn(systemRef, argument.ConstRef());
        }
        return system.As<System*>();
    }

    std::unordered_map<std::string, Mirror::Any> SystemFactory::GetArguments()
    {
        std::unordered_map<std::string, Mirror::Any> result;
        for (auto& [name, argument] : arguments) {
            result.emplace(name, argument.Ref());
        }
        return result;
    }

    const std::unordered_map<std::string, Mirror::Any>& SystemFactory::GetArguments() const
    {
        return arguments;
    }

    SystemClass SystemFactory::GetClass() const
    {
        return clazz;
    }

    void SystemFactory::BuildArgumentLists()
    {
        const auto& memberVariables = clazz->GetMemberVariables();
        arguments.reserve(memberVariables.size());
        for (const auto& [id, member] : memberVariables) {
            arguments.emplace(id.name, member.GetDyn(clazz->GetDefaultObject()));
        }
    }
} // namespace Runtime::Internal

namespace Runtime {
    ScopedUpdaterDyn::ScopedUpdaterDyn(ECRegistry& inRegistry, CompClass inClass, Entity inEntity, const Mirror::Any& inCompRef)
        : registry(inRegistry)
        , clazz(inClass)
        , entity(inEntity)
        , compRef(inCompRef)
    {
    }

    ScopedUpdaterDyn::~ScopedUpdaterDyn()
    {
        registry.NotifyUpdatedDyn(clazz, entity);
    }

    GScopedUpdaterDyn::GScopedUpdaterDyn(ECRegistry& inRegistry, GCompClass inClass, const Mirror::Any& inGlobalCompRef)
        : registry(inRegistry)
        , clazz(inClass)
        , globalCompRef(inGlobalCompRef)
    {
    }

    GScopedUpdaterDyn::~GScopedUpdaterDyn()
    {
        registry.GNotifyUpdatedDyn(clazz);
    }

    RuntimeFilter::RuntimeFilter() = default;

    RuntimeFilter& RuntimeFilter::IncludeDyn(CompClass inClass)
    {
        Assert(!includes.contains(inClass));
        includes.emplace(inClass);
        return *this;
    }

    RuntimeFilter& RuntimeFilter::ExcludeDyn(CompClass inClass)
    {
        Assert(!excludes.contains(inClass));
        excludes.emplace(inClass);
        return *this;
    }

    Observer::Observer(ECRegistry& inRegistry)
        : registry(inRegistry)
    {
    }

    Observer::~Observer()
    {
        UnbindAll();
    }

    Observer& Observer::ObConstructedDyn(CompClass inClass)
    {
        return OnEvent(registry.EventsDyn(inClass).onConstructed);
    }

    Observer& Observer::ObUpdatedDyn(CompClass inClass)
    {
        return OnEvent(registry.EventsDyn(inClass).onUpdated);
    }

    Observer& Observer::ObRemoved(CompClass inClass)
    {
        return OnEvent(registry.EventsDyn(inClass).onRemove);
    }

    size_t Observer::Count() const
    {
        return entities.size();
    }

    void Observer::Each(const EntityTraverseFunc& inFunc) const
    {
        for (const auto& entity : entities) {
            inFunc(entity);
        }
    }

    void Observer::EachThenClear(const EntityTraverseFunc& inFunc)
    {
        Each(inFunc);
        Clear();
    }

    void Observer::Clear()
    {
        entities.clear();
    }

    const std::vector<Entity>& Observer::All() const
    {
        return entities;
    }

    std::vector<Entity> Observer::Pop()
    {
        auto result = entities;
        Clear();
        return result;
    }

    void Observer::UnbindAll()
    {
        for (auto& deleter : receiverHandles | std::views::values) {
            deleter();
        }
        receiverHandles.clear();
    }

    Observer::ConstIter Observer::Begin() const
    {
        return entities.begin();
    }

    Observer::ConstIter Observer::End() const
    {
        return entities.end();
    }

    Observer::ConstIter Observer::begin() const
    {
        return Begin();
    }

    Observer::ConstIter Observer::end() const
    {
        return End();
    }

    void Observer::RecordEntity(ECRegistry& inRegistry, Entity inEntity)
    {
        entities.emplace_back(inEntity);
    }

    Observer& Observer::OnEvent(Common::Delegate<ECRegistry&, Entity>& inEvent)
    {
        const auto handle = inEvent.BindMember<&Observer::RecordEntity>(*this);
        receiverHandles.emplace_back(
            handle,
            [&inEvent, handle]() -> void {
                inEvent.Unbind(handle);
            });
        return *this;
    }

    EventsObserverDyn::EventsObserverDyn(ECRegistry& inRegistry, CompClass inClass)
        : constructedObserver(inRegistry.Observer())
        , updatedObserver(inRegistry.Observer())
        , removedObserver(inRegistry.Observer())
    {
        constructedObserver.ObConstructedDyn(inClass);
        updatedObserver.ObUpdatedDyn(inClass);
        removedObserver.ObRemoved(inClass);
    }

    EventsObserverDyn::~EventsObserverDyn() = default;

    size_t EventsObserverDyn::ConstructedCount() const
    {
        return constructedObserver.Count();
    }

    size_t EventsObserverDyn::UpdatedCount() const
    {
        return updatedObserver.Count();
    }

    size_t EventsObserverDyn::RemovedCount() const
    {
        return removedObserver.Count();
    }

    void EventsObserverDyn::EachConstructed(const EntityTraverseFunc& inFunc) const
    {
        constructedObserver.Each(inFunc);
    }

    void EventsObserverDyn::EachUpdated(const EntityTraverseFunc& inFunc) const
    {
        updatedObserver.Each(inFunc);
    }

    void EventsObserverDyn::EachRemoved(const EntityTraverseFunc& inFunc) const
    {
        removedObserver.Each(inFunc);
    }

    void EventsObserverDyn::ClearConstructed()
    {
        constructedObserver.Clear();
    }

    void EventsObserverDyn::ClearUpdated()
    {
        updatedObserver.Clear();
    }

    void EventsObserverDyn::ClearRemoved()
    {
        removedObserver.Clear();
    }

    void EventsObserverDyn::Clear()
    {
        ClearConstructed();
        ClearUpdated();
        ClearRemoved();
    }

    const Observer& EventsObserverDyn::Constructed() const
    {
        return constructedObserver;
    }

    const Observer& EventsObserverDyn::Updated() const
    {
        return updatedObserver;
    }

    const Observer& EventsObserverDyn::Removed() const
    {
        return removedObserver;
    }

    ECRegistry::ECRegistry()
    {
        archetypes.emplace(0, Internal::Archetype({}));
    }

    ECRegistry::~ECRegistry()
    {
        CheckEventsUnbound();
    }

    ECRegistry::ECRegistry(const ECRegistry& inOther)
        : entities(inOther.entities)
        , globalComps(inOther.globalComps)
        , archetypes(inOther.archetypes)
    {
        RebindEntityArchetypes();
    }

    ECRegistry::ECRegistry(ECRegistry&& inOther) noexcept
        : entities(std::move(inOther.entities))
        , globalComps(std::move(inOther.globalComps))
        , archetypes(std::move(inOther.archetypes))
    {
        RebindEntityArchetypes();
    }

    ECRegistry& ECRegistry::operator=(const ECRegistry& inOther)
    {
        if (this == &inOther) {
            return *this;
        }
        entities = inOther.entities;
        globalComps = inOther.globalComps;
        archetypes = inOther.archetypes;
        RebindEntityArchetypes();
        return *this;
    }

    ECRegistry& ECRegistry::operator=(ECRegistry&& inOther) noexcept
    {
        if (this == &inOther) {
            return *this;
        }
        entities = std::move(inOther.entities);
        globalComps = std::move(inOther.globalComps);
        archetypes = std::move(inOther.archetypes);
        RebindEntityArchetypes();
        return *this;
    }

    Entity ECRegistry::Create()
    {
        const Entity result = entities.Allocate();
        Internal::Archetype& archetype = archetypes.at(0);
        const auto elemIndex = archetype.EmplaceElem(result);
        entities.SetLocation(result, archetype, elemIndex);
        return result;
    }

    void ECRegistry::Create(Entity inEntity)
    {
        entities.Allocate(inEntity);
        Internal::Archetype& archetype = archetypes.at(0);
        const auto elemIndex = archetype.EmplaceElem(inEntity);
        entities.SetLocation(inEntity, archetype, elemIndex);
    }

    void ECRegistry::Destroy(Entity inEntity)
    {
        Assert(Valid(inEntity));
        Internal::Archetype& archetype = *entities.GetArchetypePtr(inEntity);
        const auto elemIndex = entities.GetElemIndex(inEntity);
        for (const auto& compRtti : archetype.GetRttiVec()) {
            NotifyRemoveDyn(compRtti.Class(), inEntity);
        }
        EraseArchetypeElem(archetype, elemIndex);
        entities.Free(inEntity);
    }

    bool ECRegistry::Valid(Entity inEntity) const
    {
        return entities.Valid(inEntity);
    }

    size_t ECRegistry::Count() const
    {
        return entities.Count();
    }

    void ECRegistry::Clear()
    {
        entities.Clear();
        globalComps.clear();
        archetypes.clear();
        archetypes.emplace(0, Internal::Archetype({}));
    }

    void ECRegistry::Each(const EntityTraverseFunc& inFunc) const
    {
        entities.Each(inFunc);
    }

    ECRegistry::ConstIter ECRegistry::Begin() const
    {
        return entities.Begin();
    }

    ECRegistry::ConstIter ECRegistry::End() const
    {
        return entities.End();
    }

    ECRegistry::ConstIter ECRegistry::begin() const
    {
        return Begin();
    }

    ECRegistry::ConstIter ECRegistry::end() const
    {
        return End();
    }

    void ECRegistry::CompEach(Entity inEntity, const CompTraverseFunc& inFunc) const
    {
        const Internal::Archetype& archetype = *entities.GetArchetypePtr(inEntity);
        for (const auto& compRtti : archetype.GetRttiVec()) {
            inFunc(compRtti.Class());
        }
    }

    size_t ECRegistry::CompCount(Entity inEntity) const
    {
        const Internal::Archetype& archetype = *entities.GetArchetypePtr(inEntity);
        return archetype.GetRttiVec().size();
    }

    Runtime::RuntimeView ECRegistry::RuntimeView(const RuntimeFilter& inFilter)
    {
        return Runtime::RuntimeView { *this, inFilter };
    }

    Runtime::ConstRuntimeView ECRegistry::RuntimeView(const RuntimeFilter& inFilter) const
    {
        return Runtime::ConstRuntimeView { *this, inFilter };
    }

    Runtime::ConstRuntimeView ECRegistry::ConstRuntimeView(const RuntimeFilter& inFilter) const
    {
        return Runtime::ConstRuntimeView { *this, inFilter };
    }

    Runtime::EventsObserverDyn ECRegistry::EventsObserverDyn(CompClass inClass)
    {
        return Runtime::EventsObserverDyn { *this, inClass };
    }

    void ECRegistry::NotifyUpdatedDyn(CompClass inClass, Entity inEntity)
    {
        const auto iter = compEvents.find(inClass);
        if (iter == compEvents.end()) {
            return;
        }
        iter->second.onUpdated.Broadcast(*this, inEntity);
    }

    void ECRegistry::NotifyConstructedDyn(CompClass inClass, Entity inEntity)
    {
        const auto iter = compEvents.find(inClass);
        if (iter == compEvents.end()) {
            return;
        }
        iter->second.onConstructed.Broadcast(*this, inEntity);
    }

    void ECRegistry::NotifyRemoveDyn(CompClass inClass, Entity inEntity)
    {
        const auto iter = compEvents.find(inClass);
        if (iter == compEvents.end()) {
            return;
        }
        iter->second.onRemove.Broadcast(*this, inEntity);
    }

    Runtime::Observer ECRegistry::Observer()
    {
        return Runtime::Observer { *this };
    }

    void ECRegistry::Save(ECArchive& outArchive) const
    {
        outArchive = {};
        outArchive.entities.reserve(Count());
        Each([&](Entity entity) -> void {
            if (Has<TransientTag>(entity)) {
                return;
            }
            outArchive.entities.emplace(entity, EntityArchive {});
            auto& comps = outArchive.entities.at(entity).comps;
            comps.reserve(CompCount(entity));

            CompEach(entity, [&](CompClass clazz) -> void {
                if (clazz->IsTransient()) {
                    return;
                }
                comps.emplace(clazz, std::vector<uint8_t> {});
                Common::MemorySerializeStream stream(comps.at(clazz));
                GetDyn(clazz, entity).Serialize(stream);
            });
        });

        auto& gComps = outArchive.globalComps;
        gComps.reserve(GCompCount());
        GCompEach([&](GCompClass clazz) -> void {
            if (clazz->IsTransient()) {
                return;
            }
            gComps.emplace(clazz, std::vector<uint8_t> {});
            Common::MemorySerializeStream stream(gComps.at(clazz));
            GGetDyn(clazz).Serialize(stream);
        });
    }

    void ECRegistry::Load(const ECArchive& inArchive)
    {
        Clear();

        for (const auto& [entity, entityArchive] : inArchive.entities) {
            Create(entity);
            for (const auto& [compClass, compData] : entityArchive.comps) {
                if (compClass->IsTransient()) {
                    continue;
                }
                Assert(compClass->HasDefaultConstructor());
                Mirror::Any compRef = EmplaceDyn(compClass, entity, {});
                Common::MemoryDeserializeStream stream(compData);
                compRef.Deserialize(stream);
            }
        }

        for (const auto& [gCompClass, gCompData] : inArchive.globalComps) {
            if (gCompClass->IsTransient()) {
                continue;
            }
            Assert(gCompClass->HasDefaultConstructor());
            Mirror::Any gCompRef = GEmplaceDyn(gCompClass, {});
            Common::MemoryDeserializeStream stream(gCompData);
            gCompRef.Deserialize(stream);
        }
    }

    void ECRegistry::CheckEventsUnbound() const
    {
        for (const auto& events : compEvents | std::views::values) {
            Assert(events.onConstructed.Count() == 0);
            Assert(events.onUpdated.Count() == 0);
            Assert(events.onRemove.Count() == 0);
        }

        for (const auto& events : globalCompEvents | std::views::values) {
            Assert(events.onConstructed.Count() == 0);
            Assert(events.onUpdated.Count() == 0);
            Assert(events.onRemove.Count() == 0);
        }
    }

    void ECRegistry::EraseArchetypeElem(Internal::Archetype& inArchetype, size_t inElemIndex)
    {
        const Entity movedEntity = inArchetype.EraseElem(inElemIndex);
        if (movedEntity != entityNull) {
            entities.SetElemIndex(movedEntity, inElemIndex);
        }
    }

    void ECRegistry::RebindEntityArchetypes()
    {
        for (auto iter = entities.Begin(); iter != entities.End(); ++iter) {
            entities.SetArchetypePtr(*iter, archetypes.at(entities.GetArchetype(*iter)));
        }
    }

    Mirror::Any ECRegistry::EmplaceDyn(CompClass inClass, Entity inEntity, const Mirror::ArgumentList& inArgs)
    {
        Internal::Archetype& archetype = MoveEntityForAdd(Internal::CompRtti(inClass), inEntity);
        Mirror::Any tempObj = inClass->ConstructDyn(inArgs);
        Mirror::Any compRef = archetype.EmplaceComp(entities.GetElemIndex(inEntity), inClass, tempObj.Ref());
        NotifyConstructedDyn(inClass, inEntity);
        return compRef;
    }

    void ECRegistry::RemoveDyn(CompClass inClass, Entity inEntity)
    {
        MoveEntityForRemove(inClass, inEntity);
    }

    Internal::Archetype& ECRegistry::MoveEntityForAdd(const Internal::CompRtti& inRtti, Entity inEntity)
    {
        Assert(Valid(inEntity));
        const CompClass inClass = inRtti.Class();
        const auto elemIndex = entities.GetElemIndex(inEntity);
        Internal::Archetype& archetype = *entities.GetArchetypePtr(inEntity);
        Internal::Archetype* newArchetype = archetype.FindAddTransition(inClass);

        if (newArchetype == nullptr) {
            const Internal::ArchetypeId newArchetypeId = archetype.Id() + inClass->GetTypeInfo()->id;
            if (!archetypes.contains(newArchetypeId)) {
                archetypes.emplace(newArchetypeId, Internal::Archetype(archetype.NewRttiVecByAdd(inRtti)));
            }
            newArchetype = &archetypes.at(newArchetypeId);
            archetype.CacheAddTransition(inClass, *newArchetype);
            if (newArchetype->FindRemoveTransition(inClass) == nullptr) {
                newArchetype->CacheRemoveTransition(inClass, archetype);
            }
        }

        const auto newElemIndex = newArchetype->EmplaceElem(inEntity, archetype, elemIndex);
        entities.SetLocation(inEntity, *newArchetype, newElemIndex);
        EraseArchetypeElem(archetype, elemIndex);
        return *newArchetype;
    }

    void ECRegistry::MoveEntityForRemove(CompClass inClass, Entity inEntity)
    {
        Assert(Valid(inEntity) && HasDyn(inClass, inEntity));
        const auto elemIndex = entities.GetElemIndex(inEntity);
        Internal::Archetype& archetype = *entities.GetArchetypePtr(inEntity);
        Internal::Archetype* newArchetype = archetype.FindRemoveTransition(inClass);

        if (newArchetype == nullptr) {
            const Internal::ArchetypeId newArchetypeId = archetype.Id() - inClass->GetTypeInfo()->id;
            if (!archetypes.contains(newArchetypeId)) {
                archetypes.emplace(newArchetypeId, Internal::Archetype(archetype.NewRttiVecByRemove(Internal::CompRtti(inClass))));
            }
            newArchetype = &archetypes.at(newArchetypeId);
            archetype.CacheRemoveTransition(inClass, *newArchetype);
            if (newArchetype->FindAddTransition(inClass) == nullptr) {
                newArchetype->CacheAddTransition(inClass, archetype);
            }
        }

        NotifyRemoveDyn(inClass, inEntity);
        const auto newElemIndex = newArchetype->EmplaceElem(inEntity, archetype, elemIndex);
        entities.SetLocation(inEntity, *newArchetype, newElemIndex);
        EraseArchetypeElem(archetype, elemIndex);
    }

    void ECRegistry::UpdateDyn(CompClass inClass, Entity inEntity, const DynUpdateFunc& inFunc)
    {
        Assert(Valid(inEntity) && HasDyn(inClass, inEntity));
        inFunc(GetDyn(inClass, inEntity));
        NotifyUpdatedDyn(inClass, inEntity);
    }

    ScopedUpdaterDyn ECRegistry::UpdateDyn(CompClass inClass, Entity inEntity)
    {
        Assert(Valid(inEntity) && HasDyn(inClass, inEntity));
        return { *this, inClass, inEntity, GetDyn(inClass, inEntity) };
    }

    bool ECRegistry::HasDyn(CompClass inClass, Entity inEntity) const
    {
        Assert(Valid(inEntity));
        return entities.GetArchetypePtr(inEntity)->Contains(inClass);
    }

    Mirror::Any ECRegistry::FindDyn(CompClass inClass, Entity inEntity)
    {
        return HasDyn(inClass, inEntity) ? GetDyn(inClass, inEntity) : Mirror::Any();
    }

    Mirror::Any ECRegistry::FindDyn(CompClass inClass, Entity inEntity) const
    {
        return HasDyn(inClass, inEntity) ? GetDyn(inClass, inEntity) : Mirror::Any();
    }

    Mirror::Any ECRegistry::GetDyn(CompClass inClass, Entity inEntity)
    {
        Assert(Valid(inEntity) && HasDyn(inClass, inEntity));
        Mirror::Any compRef = entities.GetArchetypePtr(inEntity)->GetComp(entities.GetElemIndex(inEntity), inClass);
        return compRef;
    }

    Mirror::Any ECRegistry::GetDyn(CompClass inClass, Entity inEntity) const
    {
        Assert(Valid(inEntity) && HasDyn(inClass, inEntity));
        Mirror::Any compRef = entities.GetArchetypePtr(inEntity)->GetComp(entities.GetElemIndex(inEntity), inClass);
        return compRef.ConstRef();
    }

    ECRegistry::CompEvents& ECRegistry::EventsDyn(CompClass inClass)
    {
        return compEvents[inClass];
    }

    void ECRegistry::GNotifyUpdatedDyn(GCompClass inClass)
    {
        const auto iter = globalCompEvents.find(inClass);
        if (iter == globalCompEvents.end()) {
            return;
        }
        iter->second.onUpdated.Broadcast(*this);
    }

    void ECRegistry::GNotifyConstructedDyn(GCompClass inClass)
    {
        const auto iter = globalCompEvents.find(inClass);
        if (iter == globalCompEvents.end()) {
            return;
        }
        iter->second.onConstructed.Broadcast(*this);
    }

    void ECRegistry::GNotifyRemoveDyn(CompClass inClass)
    {
        const auto iter = globalCompEvents.find(inClass);
        if (iter == globalCompEvents.end()) {
            return;
        }
        iter->second.onRemove.Broadcast(*this);
    }

    Mirror::Any ECRegistry::GEmplaceDyn(GCompClass inClass, const Mirror::ArgumentList& inArgs)
    {
        Assert(Internal::IsGlobalCompClass(inClass));
        Assert(!GHasDyn(inClass));
        globalComps.emplace(inClass, inClass->ConstructDyn(inArgs));
        GNotifyConstructedDyn(inClass);
        return GGetDyn(inClass);
    }

    void ECRegistry::GRemoveDyn(GCompClass inClass)
    {
        Assert(Internal::IsGlobalCompClass(inClass));
        Assert(GHasDyn(inClass));
        GNotifyRemoveDyn(inClass);
        globalComps.erase(inClass);
    }

    void ECRegistry::GUpdateDyn(GCompClass inClass, const DynUpdateFunc& inFunc)
    {
        Assert(Internal::IsGlobalCompClass(inClass));
        Assert(GHasDyn(inClass));
        inFunc(GGetDyn(inClass));
        GNotifyUpdatedDyn(inClass);
    }

    GScopedUpdaterDyn ECRegistry::GUpdateDyn(GCompClass inClass)
    {
        Assert(Internal::IsGlobalCompClass(inClass));
        Assert(GHasDyn(inClass));
        return { *this, inClass, GGetDyn(inClass) };
    }

    bool ECRegistry::GHasDyn(GCompClass inClass) const
    {
        Assert(Internal::IsGlobalCompClass(inClass));
        return globalComps.contains(inClass);
    }

    Mirror::Any ECRegistry::GFindDyn(GCompClass inClass)
    {
        Assert(Internal::IsGlobalCompClass(inClass));
        return GHasDyn(inClass) ? GGetDyn(inClass) : Mirror::Any();
    }

    Mirror::Any ECRegistry::GFindDyn(GCompClass inClass) const
    {
        Assert(Internal::IsGlobalCompClass(inClass));
        return GHasDyn(inClass) ? GGetDyn(inClass) : Mirror::Any();
    }

    Mirror::Any ECRegistry::GGetDyn(GCompClass inClass)
    {
        Assert(Internal::IsGlobalCompClass(inClass));
        Assert(GHasDyn(inClass));
        return globalComps.at(inClass).Ref();
    }

    Mirror::Any ECRegistry::GGetDyn(GCompClass inClass) const
    {
        Assert(Internal::IsGlobalCompClass(inClass));
        Assert(GHasDyn(inClass));
        return globalComps.at(inClass).ConstRef();
    }

    ECRegistry::GCompEvents& ECRegistry::GEventsDyn(GCompClass inClass)
    {
        Assert(Internal::IsGlobalCompClass(inClass));
        return globalCompEvents[inClass];
    }

    void ECRegistry::GCompEach(const GCompTraverseFunc& inFunc) const
    {
        for (const auto& clazz : globalComps | std::views::keys) {
            inFunc(clazz);
        }
    }

    size_t ECRegistry::GCompCount() const
    {
        return globalComps.size();
    }

    SystemGroup::SystemGroup(std::string inName, SystemExecuteStrategy inStrategy)
        : name(std::move(inName))
        , strategy(inStrategy)
    {
    }

    Internal::SystemFactory& SystemGroup::EmplaceSystemDyn(SystemClass inClass)
    {
        Assert(!HasSystemDyn(inClass));
        return systems.emplace_back(inClass);
    }

    void SystemGroup::RemoveSystemDyn(SystemClass inClass)
    {
        const auto iter = FindSystem(inClass);
        Assert(iter != systems.end());
        systems.erase(iter);
    }

    bool SystemGroup::HasSystemDyn(SystemClass inClass) const
    {
        const auto iter = FindSystem(inClass);
        return iter != systems.end();
    }

    Internal::SystemFactory& SystemGroup::GetSystemDyn(SystemClass inClass)
    {
        const auto iter = FindSystem(inClass);
        Assert(iter != systems.end());
        return *iter;
    }

    const Internal::SystemFactory& SystemGroup::GetSystemDyn(SystemClass inClass) const
    {
        const auto iter = FindSystem(inClass);
        Assert(iter != systems.end());
        return *iter;
    }

    Internal::SystemFactory& SystemGroup::MoveSystemToDyn(SystemClass inSrcClass, SystemClass inDstClass)
    {
        const auto srcIter = FindSystem(inSrcClass);
        Assert(srcIter != systems.end());
        const auto tempGroup = std::move(*srcIter);
        systems.erase(srcIter);

        const auto dstIter = FindSystem(inDstClass);
        Assert(dstIter != systems.end());
        return *systems.emplace(dstIter, std::move(tempGroup));
    }

    const std::vector<Internal::SystemFactory>& SystemGroup::GetSystems()
    {
        return systems;
    }

    const std::vector<Internal::SystemFactory>& SystemGroup::GetSystems() const
    {
        return systems;
    }

    const std::string& SystemGroup::GetName() const
    {
        return name;
    }

    SystemExecuteStrategy SystemGroup::GetStrategy() const
    {
        return strategy;
    }

    std::vector<Internal::SystemFactory>::iterator SystemGroup::FindSystem(SystemClass inClass)
    {
        return std::ranges::find_if(systems, [inClass](const Internal::SystemFactory& inFactory) -> bool {
            return inFactory.GetClass() == inClass;
        });
    }

    std::vector<Internal::SystemFactory>::const_iterator SystemGroup::FindSystem(SystemClass inClass) const
    {
        return std::ranges::find_if(systems, [inClass](const Internal::SystemFactory& inFactory) -> bool {
            return inFactory.GetClass() == inClass;
        });
    }

    SystemGraph::SystemGraph() = default;

    SystemGroup& SystemGraph::AddGroup(const std::string& inName, SystemExecuteStrategy inStrategy)
    {
        Assert(!HasGroup(inName));
        return systemGroups.emplace_back(inName, inStrategy);
    }

    void SystemGraph::RemoveGroup(const std::string& inName)
    {
        const auto iter = FindGroup(inName);
        Assert(iter != systemGroups.end());
        systemGroups.erase(iter);
    }

    bool SystemGraph::HasGroup(const std::string& inName) const
    {
        const auto iter = FindGroup(inName);
        return iter != systemGroups.end();
    }

    SystemGroup& SystemGraph::GetGroup(const std::string& inName)
    {
        const auto iter = FindGroup(inName);
        Assert(iter != systemGroups.end());
        return *iter;
    }

    const SystemGroup& SystemGraph::GetGroup(const std::string& inName) const
    {
        const auto iter = FindGroup(inName);
        Assert(iter != systemGroups.end());
        return *iter;
    }

    const std::vector<SystemGroup>& SystemGraph::GetGroups() const
    {
        return systemGroups;
    }

    SystemGroup& SystemGraph::MoveGroupTo(const std::string& inSrcName, const std::string& inDstName)
    {
        const auto srcIter = FindGroup(inSrcName);
        Assert(srcIter != systemGroups.end());
        const auto tempGroup = std::move(*srcIter);
        systemGroups.erase(srcIter);

        const auto dstIter = FindGroup(inDstName);
        Assert(dstIter != systemGroups.end());
        return *systemGroups.emplace(dstIter, std::move(tempGroup));
    }

    std::vector<SystemGroup>::iterator SystemGraph::FindGroup(const std::string& inName)
    {
        return std::ranges::find_if(systemGroups, [&](const SystemGroup& group) -> bool {
            return group.GetName() == inName;
        });
    }

    std::vector<SystemGroup>::const_iterator SystemGraph::FindGroup(const std::string& inName) const
    {
        return std::ranges::find_if(systemGroups, [&](const SystemGroup& group) -> bool {
            return group.GetName() == inName;
        });
    }

    SystemPipeline::SystemPipeline(const SystemGraph& inGraph)
    {
        const auto& systemGroups = inGraph.GetGroups();
        systemGraph.reserve(systemGroups.size());

        for (const auto& group : systemGroups) {
            auto& [systemContexts, strategy] = systemGraph.emplace_back();
            const auto& factories = group.GetSystems();

            strategy = group.GetStrategy();
            systemContexts.reserve(factories.size());
            for (const auto& factory : group.GetSystems()) {
                systemContexts.emplace_back(factory, nullptr);
            }
        }
    }

    void SystemPipeline::ParallelPerformAction(const ActionFunc& inActionFunc)
    {
        tf::Taskflow taskFlow;
        auto lastBarrier = taskFlow.emplace([]() -> void { Core::ScopedThreadTag threadTag(Core::ThreadTag::gameWorker); });

        for (auto& groupContext : systemGraph) {
            if (groupContext.strategy == SystemExecuteStrategy::sequential) {
                tf::Task lastTask = lastBarrier;
                for (auto& systemContext : groupContext.systems) {
                    auto task = taskFlow.emplace([&]() -> void {
                        Core::ScopedThreadTag threadTag(Core::ThreadTag::gameWorker);
                        inActionFunc(systemContext);
                    });
                    task.succeed(lastTask);
                    lastTask = task;
                }
                lastBarrier = lastTask;
            } else if (groupContext.strategy == SystemExecuteStrategy::concurrent) {
                std::vector<tf::Task> tasks;
                tasks.reserve(groupContext.systems.size());

                for (auto& systemContext : groupContext.systems) {
                    tasks.emplace_back(taskFlow.emplace([&]() -> void {
                        Core::ScopedThreadTag threadTag(Core::ThreadTag::gameWorker);
                        inActionFunc(systemContext);
                    }));
                    tasks.back().succeed(lastBarrier);
                }

                auto barrier = taskFlow.emplace([]() -> void { Core::ScopedThreadTag threadTag(Core::ThreadTag::gameWorker); });
                for (const auto& task : tasks) {
                    barrier.succeed(task);
                }
                lastBarrier = barrier;
            } else {
                QuickFail();
            }
        }

        tf::Executor executor;
        executor
            .run(taskFlow)
            .wait();
    }

    SystemSetupContext::SystemSetupContext()
        : playType(PlayType::max)
        , client(nullptr)
    {
    }

    SystemGraphExecutor::SystemGraphExecutor(ECRegistry& inEcRegistry, const SystemGraph& inSystemGraph, const SystemSetupContext& inSetupContext)
        : ecRegistry(inEcRegistry)
        , systemGraph(inSystemGraph)
        , pipeline(inSystemGraph)
    {
        pipeline.ParallelPerformAction([&](SystemPipeline::SystemContext& context) -> void {
            context.instance = context.factory.Build(inEcRegistry, inSetupContext);
        });
    }

    SystemGraphExecutor::~SystemGraphExecutor()
    {
        pipeline.ParallelPerformAction([](SystemPipeline::SystemContext& context) -> void {
            context.instance = nullptr;
        });
        ecRegistry.CheckEventsUnbound();
    }

    void SystemGraphExecutor::Tick(float inDeltaTimeSeconds)
    {
        pipeline.ParallelPerformAction([&](const SystemPipeline::SystemContext& context) -> void {
            context.instance->Tick(inDeltaTimeSeconds);
        });
    }
} // namespace Runtime
