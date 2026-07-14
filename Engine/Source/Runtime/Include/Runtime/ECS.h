//
// Created by johnk on 2024/10/31.
//

#pragma once

#include <array>
#include <memory>
#include <tuple>
#include <unordered_set>
#include <unordered_map>
#include <vector>

#include <Common/Delegate.h>
#include <Common/Utility.h>
#include <Common/Memory.h>
#include <Mirror/Mirror.h>
#include <Runtime/Meta.h>
#include <Runtime/Api.h>

namespace Runtime {
    using Entity = uint32_t;
    static constexpr Entity entityNull = 0;

    using CompClass = const Mirror::Class*;
    using GCompClass = const Mirror::Class*;
    using SystemClass = const Mirror::Class*;

    class ECRegistry;
    class Client;
    struct SystemSetupContext;

    template <typename T>
    concept ECRegistryOrConst = std::is_same_v<std::remove_const_t<T>, ECRegistry>;

    class RUNTIME_API EClass() System {
    public:
        EPolyBaseClassBody(System)

        explicit System(ECRegistry& inRegistry, const SystemSetupContext&);
        virtual ~System();
        virtual void Tick(float inDeltaTimeSeconds);

    protected:
        ECRegistry& registry;
    };
}

namespace Runtime::Internal {
    using ArchetypeId = Mirror::TypeId;
    using ElemPtr = void*;

    template <typename T> const Mirror::Class* GetClass();
    template <typename T> struct MemberFuncPtrTraits;

    class RUNTIME_API CompRtti {
    public:
        explicit CompRtti(CompClass inClass);
        template <typename C> static CompRtti Create();
        Mirror::Any CopyConstruct(ElemPtr inElem, const Mirror::Any& inOther) const;
        Mirror::Any MoveConstruct(ElemPtr inElem, const Mirror::Any& inOther) const;
        void CopyConstructFrom(ElemPtr inDstElem, ElemPtr inSrcElem) const;
        void MoveConstructFrom(ElemPtr inDstElem, ElemPtr inSrcElem) const;
        void Destruct(ElemPtr inElem) const;
        Mirror::Any Get(ElemPtr inElem) const;
        CompClass Class() const;
        size_t MemorySize() const;
        size_t MemoryAlignment() const;

    private:
        using ConstructFromFunc = void(*)(ElemPtr, ElemPtr);
        using DestructFunc = void(*)(ElemPtr);

        CompClass clazz;
        ConstructFromFunc copyConstructFrom;
        ConstructFromFunc moveConstructFrom;
        DestructFunc destruct;
    };

    class RUNTIME_API Archetype {
    public:
        explicit Archetype(const std::vector<CompRtti>& inRttiVec);
        ~Archetype();

        Archetype(const Archetype& inOther);
        Archetype(Archetype&& inOther) noexcept;
        Archetype& operator=(const Archetype& inOther);
        Archetype& operator=(Archetype&& inOther) noexcept;

        bool Contains(CompClass inClazz) const;
        bool ContainsAll(const std::vector<CompClass>& inClasses) const;
        bool NotContainsAny(const std::vector<CompClass>& inClasses) const;
        size_t EmplaceElem(Entity inEntity);
        size_t EmplaceElem(Entity inEntity, Archetype& inSrcArchetype, size_t inSrcElemIndex);
        Mirror::Any EmplaceComp(size_t inElemIndex, CompClass inCompClass, const Mirror::Any& inCompRef);
        template <typename C, typename... Args> C& EmplaceComp(size_t inElemIndex, Args&&... inArgs);
        Entity EraseElem(size_t inElemIndex);
        Mirror::Any GetComp(size_t inElemIndex, CompClass inCompClass);
        Mirror::Any GetComp(size_t inElemIndex, CompClass inCompClass) const;
        template <typename C> C& GetComp(size_t inElemIndex);
        template <typename C> const C& GetComp(size_t inElemIndex) const;
        ElemPtr GetCompAt(size_t inElemIndex, size_t inCompIndex);
        const void* GetCompAt(size_t inElemIndex, size_t inCompIndex) const;
        ElemPtr GetCompColumn(size_t inCompIndex);
        const void* GetCompColumn(size_t inCompIndex) const;
        size_t GetCompIndex(CompClass inCompClass) const;
        template <typename C> size_t GetCompIndex() const;
        Entity EntityAt(size_t inElemIndex) const;
        size_t Count() const;
        auto All() const;
        const std::vector<CompRtti>& GetRttiVec() const;
        ArchetypeId Id() const;
        std::vector<CompRtti> NewRttiVecByAdd(const CompRtti& inRtti) const;
        std::vector<CompRtti> NewRttiVecByRemove(const CompRtti& inRtti) const;
        Archetype* FindAddTransition(CompClass inClass) const;
        Archetype* FindRemoveTransition(CompClass inClass) const;
        void CacheAddTransition(CompClass inClass, Archetype& inArchetype);
        void CacheRemoveTransition(CompClass inClass, Archetype& inArchetype);

    private:
        using CompRttiIndex = size_t;
        const CompRtti* FindCompRtti(CompClass clazz) const;
        size_t Capacity() const;
        void Reserve(float inRatio = 1.5f);
        void DestroyElements();
        void ReleaseMemory();
        void AllocateNewElemBack();
        ElemPtr CompAt(ElemPtr inMemory, size_t inCompIndex, size_t inElemIndex) const;

        ArchetypeId id;
        size_t count;
        size_t capacity;
        std::vector<CompRtti> rttiVec;
        std::unordered_map<CompClass, CompRttiIndex> rttiMap;
        std::vector<ElemPtr> compMemory;
        std::vector<size_t> compStrides;
        std::vector<Entity> elemMap;
        std::vector<std::pair<CompClass, Archetype*>> addTransitions;
        std::vector<std::pair<CompClass, Archetype*>> removeTransitions;
    };

    class RUNTIME_API EntityPool {
    public:
        using EntityTraverseFunc = std::function<void(Entity)>;
        using ConstIter = std::vector<Entity>::const_iterator;

        EntityPool();

        size_t Count() const;
        bool Valid(Entity inEntity) const;
        Entity Allocate();
        void Allocate(Entity inEntity);
        void Free(Entity inEntity);
        void Clear();
        void Each(const EntityTraverseFunc& inFunc) const;
        void SetLocation(Entity inEntity, Archetype& inArchetype, size_t inElemIndex);
        void SetElemIndex(Entity inEntity, size_t inElemIndex);
        void SetArchetypePtr(Entity inEntity, Archetype& inArchetype);
        ArchetypeId GetArchetype(Entity inEntity) const;
        Archetype* GetArchetypePtr(Entity inEntity);
        const Archetype* GetArchetypePtr(Entity inEntity) const;
        size_t GetElemIndex(Entity inEntity) const;
        ConstIter Begin() const;
        ConstIter End() const;

    private:
        struct EntityRecord {
            bool allocated = false;
            ArchetypeId archetype = 0;
            Archetype* archetypePtr = nullptr;
            size_t elemIndex = 0;
            size_t allocatedIndex = 0;
        };

        std::vector<EntityRecord> records;
        std::vector<Entity> free;
        std::vector<Entity> allocated;
    };

    class SystemFactory {
    public:
        explicit SystemFactory(SystemClass inClass);
        Common::UniquePtr<System> Build(ECRegistry& inRegistry, const SystemSetupContext& inSetupContext) const;
        std::unordered_map<std::string, Mirror::Any> GetArguments();
        const std::unordered_map<std::string, Mirror::Any>& GetArguments() const;
        SystemClass GetClass() const;

    private:
        void BuildArgumentLists();

        SystemClass clazz;
        std::unordered_map<std::string, Mirror::Any> arguments;
    };
}

namespace Runtime {
    template <typename C>
    class ScopedUpdater {
    public:
        ScopedUpdater(ECRegistry& inRegistry, Entity inEntity, C& inCompRef);
        ~ScopedUpdater();

        NonCopyable(ScopedUpdater)
        NonMovable(ScopedUpdater)

        C* operator->() const;

    private:
        ECRegistry& registry;
        Entity entity;
        C& compRef;
    };

    template <typename G>
    class GScopedUpdater {
    public:
        GScopedUpdater(ECRegistry& inRegistry, G& inGlobalCompRef);
        ~GScopedUpdater();

        NonCopyable(GScopedUpdater)
        NonMovable(GScopedUpdater)

        G* operator->() const;

    private:
        ECRegistry& registry;
        G& globalCompRef;
    };

    class RUNTIME_API ScopedUpdaterDyn {
    public:
        ScopedUpdaterDyn(ECRegistry& inRegistry, CompClass inClass, Entity inEntity, const Mirror::Any& inCompRef);
        ~ScopedUpdaterDyn();

        NonCopyable(ScopedUpdaterDyn)
        NonMovable(ScopedUpdaterDyn)

        template <typename T> T As() const;

    private:
        ECRegistry& registry;
        CompClass clazz;
        Entity entity;
        Mirror::Any compRef;
    };

    class RUNTIME_API GScopedUpdaterDyn {
    public:
        GScopedUpdaterDyn(ECRegistry& inRegistry, GCompClass inClass, const Mirror::Any& inGlobalCompRef);
        ~GScopedUpdaterDyn();

        NonCopyable(GScopedUpdaterDyn)
        NonMovable(GScopedUpdaterDyn)

        template <typename T> T As() const;

    private:
        ECRegistry& registry;
        GCompClass clazz;
        Mirror::Any globalCompRef;
    };

    template <typename... T>
    struct Contains {};

    template <typename... T>
    struct Exclude {};

    template <typename... T>
    class BasicView;

    template <ECRegistryOrConst R, typename... C, typename... E>
    class BasicView<R, Exclude<E...>, C...> {
    public:
        using ResultVector = std::vector<std::tuple<Entity, C&...>>;
        using ConstIter = typename ResultVector::const_iterator;

        explicit BasicView(R& inRegistry);
        NonCopyable(BasicView)
        NonMovable(BasicView)

        template <typename F> void Each(F&& inFunc) const;
        const ResultVector& All() const;
        size_t Count() const;
        ConstIter Begin() const;
        ConstIter End() const;
        ConstIter begin() const;
        ConstIter end() const;

    private:
        using CompColumnPtr = std::conditional_t<std::is_const_v<R>, const void*, void*>;
        using CompColumns = std::array<CompColumnPtr, sizeof...(C)>;

        struct QueryEntry {
            Internal::ArchetypeId archetype;
            std::array<size_t, sizeof...(C)> compIndices;
        };

        template <typename A, size_t... I> CompColumns ResolveCompColumns(A& inArchetype, const QueryEntry& inEntry, std::index_sequence<I...>) const;
        template <size_t... I> auto MakeResult(Internal::Archetype& inArchetype, size_t inElemIndex, const CompColumns& inCompColumns, std::index_sequence<I...>) const;
        template <size_t... I> auto MakeResult(const Internal::Archetype& inArchetype, size_t inElemIndex, const CompColumns& inCompColumns, std::index_sequence<I...>) const;
        void Materialize() const;
        void Evaluate(R& inRegistry);

        R* registry;
        std::vector<QueryEntry> query;
        mutable ResultVector result;
        mutable bool materialized;
    };

    template <typename R, typename E, typename... C> using View = BasicView<R, E, C...>;
    template <typename R, typename E, typename... C> using ConstView = BasicView<const R, E, const C...>;

    class RUNTIME_API RuntimeFilter {
    public:
        RuntimeFilter();
        template <typename C> RuntimeFilter& Include();
        template <typename C> RuntimeFilter& Exclude();
        RuntimeFilter& IncludeDyn(CompClass inClass);
        RuntimeFilter& ExcludeDyn(CompClass inClass);

    private:
        template <ECRegistryOrConst R> friend class BasicRuntimeView;

        std::unordered_set<CompClass> includes;
        std::unordered_set<CompClass> excludes;
    };

    template <ECRegistryOrConst R>
    class BasicRuntimeView {
    public:
        using ResultEntitiesVector = std::vector<Entity>;
        using ConstIter = typename ResultEntitiesVector::const_iterator;

        explicit BasicRuntimeView(R& inRegistry, const RuntimeFilter& inFilter);
        NonCopyable(BasicRuntimeView)
        NonMovable(BasicRuntimeView)

        template <typename F> void Each(F&& inFunc) const;
        size_t Count() const;
        ConstIter Begin() const;
        ConstIter End() const;
        ConstIter begin() const;
        ConstIter end() const;

    private:
        using CompColumnPtr = std::conditional_t<std::is_const_v<R>, const void*, void*>;

        struct QueryEntry {
            Internal::ArchetypeId archetype;
            std::vector<size_t> compIndices;
        };

        template <typename ArgTuple, size_t... I> auto BuildCompSlots(std::index_sequence<I...>) const;
        template <typename F, typename ArgTuple, typename A, size_t... I> void InvokeTraverseFuncInternal(F&& inFunc, A& inArchetype, size_t inElemIndex, const std::vector<CompColumnPtr>& inCompColumns, const std::array<size_t, sizeof...(I)>& inCompSlots, std::index_sequence<I...>) const;
        template <typename C> decltype(auto) GetCompRef(size_t inElemIndex, const std::vector<CompColumnPtr>& inCompColumns, size_t inCompSlot) const;
        void MaterializeEntities() const;
        void Evaluate(R& inRegistry, const RuntimeFilter& inFilter);

        R* registry;
        std::vector<CompClass> includes;
        std::unordered_map<CompClass, size_t> slotMap;
        std::vector<QueryEntry> query;
        mutable ResultEntitiesVector resultEntities;
        mutable bool materialized;
    };

    using RuntimeView = BasicRuntimeView<ECRegistry>;
    using ConstRuntimeView = BasicRuntimeView<const ECRegistry>;

    class RUNTIME_API Observer {
    public:
        using ConstIter = std::vector<Entity>::const_iterator;
        using EntityTraverseFunc = Internal::EntityPool::EntityTraverseFunc;
        using ReceiverDeleter = std::function<void()>;

        explicit Observer(ECRegistry& inRegistry);
        ~Observer();
        NonCopyable(Observer)
        NonMovable(Observer)

        template <typename C> Observer& ObConstructed();
        template <typename C> Observer& ObUpdated();
        template <typename C> Observer& ObRemoved();
        Observer& ObConstructedDyn(CompClass inClass);
        Observer& ObUpdatedDyn(CompClass inClass);
        Observer& ObRemoved(CompClass inClass);
        size_t Count() const;
        void Each(const EntityTraverseFunc& inFunc) const;
        void EachThenClear(const EntityTraverseFunc& inFunc);
        void Clear();
        const std::vector<Entity>& All() const;
        std::vector<Entity> Pop();
        void UnbindAll();
        ConstIter Begin() const;
        ConstIter End() const;
        ConstIter begin() const;
        ConstIter end() const;

    private:
        Observer& OnEvent(Common::Delegate<ECRegistry&, Entity>& inEvent);
        void RecordEntity(ECRegistry& inRegistry, Entity inEntity);

        ECRegistry& registry;
        std::vector<std::pair<Common::CallbackHandle, ReceiverDeleter>> receiverHandles;
        std::vector<Entity> entities;
    };

    template <typename C>
    class EventsObserver {
    public:
        using EntityTraverseFunc = Observer::EntityTraverseFunc;

        explicit EventsObserver(ECRegistry& inRegistry);
        ~EventsObserver();
        NonCopyable(EventsObserver)
        NonMovable(EventsObserver)

        size_t ConstructedCount() const;
        size_t UpdatedCount() const;
        size_t RemovedCount() const;
        void EachConstructed(const EntityTraverseFunc& inFunc) const;
        void EachUpdated(const EntityTraverseFunc& inFunc) const;
        void EachRemoved(const EntityTraverseFunc& inFunc) const;
        void ClearConstructed();
        void ClearUpdated();
        void ClearRemoved();
        void Clear();
        auto& Constructed();
        auto& Updated();
        auto& Removed();
        const Observer& Constructed() const;
        const Observer& Updated() const;
        const Observer& Removed() const;

    private:
        Observer constructedObserver;
        Observer updatedObserver;
        Observer removedObserver;
    };

    class RUNTIME_API EventsObserverDyn {
    public:
        using EntityTraverseFunc = Observer::EntityTraverseFunc;

        explicit EventsObserverDyn(ECRegistry& inRegistry, CompClass inClass);
        ~EventsObserverDyn();
        NonCopyable(EventsObserverDyn)
        NonMovable(EventsObserverDyn)

        size_t ConstructedCount() const;
        size_t UpdatedCount() const;
        size_t RemovedCount() const;
        void EachConstructed(const EntityTraverseFunc& inFunc) const;
        void EachUpdated(const EntityTraverseFunc& inFunc) const;
        void EachRemoved(const EntityTraverseFunc& inFunc) const;
        void ClearConstructed();
        void ClearUpdated();
        void ClearRemoved();
        void Clear();
        const Observer& Constructed() const;
        const Observer& Updated() const;
        const Observer& Removed() const;

    private:
        Observer constructedObserver;
        Observer updatedObserver;
        Observer removedObserver;
    };

    // entities carrying this component are skipped entirely by ECRegistry::Save, use it to mark runtime-created
    // entities (e.g. players spawned by systems) that must not be serialized into a level
    struct RUNTIME_API EClass(transient) TransientTag {
        EClassBody(TransientTag)
    };

    struct RUNTIME_API EClass() EntityArchive {
        EClassBody(EntityArchive)

        EProperty() std::unordered_map<CompClass, std::vector<uint8_t>> comps;
    };

    struct RUNTIME_API EClass() ECArchive {
        EClassBody(ECArchive)

        EProperty() std::unordered_map<Entity, EntityArchive> entities;
        EProperty() std::unordered_map<GCompClass, std::vector<uint8_t>> globalComps;
    };

    class RUNTIME_API ECRegistry {
    public:
        using EntityTraverseFunc = Internal::EntityPool::EntityTraverseFunc;
        using CompTraverseFunc = std::function<void(CompClass)>;
        using GCompTraverseFunc = std::function<void(GCompClass)>;
        using DynUpdateFunc = std::function<void(const Mirror::Any&)>;
        using ConstIter = Internal::EntityPool::ConstIter;
        using CompEvent = Common::Delegate<ECRegistry&, Entity>;
        using GCompEvent = Common::Delegate<ECRegistry&>;

        struct CompEvents {
            CompEvent onConstructed;
            CompEvent onUpdated;
            CompEvent onRemove;
        };

        struct GCompEvents {
            GCompEvent onConstructed;
            GCompEvent onUpdated;
            GCompEvent onRemove;
        };

        ECRegistry();
        ~ECRegistry();

        ECRegistry(const ECRegistry& inOther);
        ECRegistry(ECRegistry&& inOther) noexcept;
        ECRegistry& operator=(const ECRegistry& inOther);
        ECRegistry& operator=(ECRegistry&& inOther) noexcept;

        // entity
        Entity Create();
        void Create(Entity inEntity);
        void Destroy(Entity inEntity);
        bool Valid(Entity inEntity) const;
        size_t Count() const;
        void Clear();
        void Each(const EntityTraverseFunc& inFunc) const;
        ConstIter Begin() const;
        ConstIter End() const;
        ConstIter begin() const;
        ConstIter end() const;
        void CompEach(Entity inEntity, const CompTraverseFunc& inFunc) const;
        size_t CompCount(Entity inEntity) const;

        // component static
        template <typename C, typename... Args> C& Emplace(Entity inEntity, Args&&... inArgs);
        template <typename C> void Remove(Entity inEntity);
        template <typename C> void NotifyUpdated(Entity inEntity);
        template <typename C, typename F> void Update(Entity inEntity, F&& inFunc);
        template <typename C> ScopedUpdater<C> Update(Entity inEntity);
        template <typename C> bool Has(Entity inEntity) const;
        template <typename C> C* Find(Entity inEntity);
        template <typename C> const C* Find(Entity inEntity) const;
        template <typename C> C& Get(Entity inEntity);
        template <typename C> const C& Get(Entity inEntity) const;
        template <typename... C, typename... E> Runtime::View<ECRegistry, Exclude<E...>, C...> View(Exclude<E...> = {});
        template <typename... C, typename... E> Runtime::ConstView<ECRegistry, Exclude<E...>, C...> View(Exclude<E...> = {}) const;
        template <typename... C, typename... E> Runtime::ConstView<ECRegistry, Exclude<E...>, C...> ConstView(Exclude<E...> = {}) const;
        template <typename C> CompEvents& Events();
        template <typename C> EventsObserver<C> EventsObserver();

        // component dynamic
        Mirror::Any EmplaceDyn(CompClass inClass, Entity inEntity, const Mirror::ArgumentList& inArgs);
        void RemoveDyn(CompClass inClass, Entity inEntity);
        void NotifyUpdatedDyn(CompClass inClass, Entity inEntity);
        void UpdateDyn(CompClass inClass, Entity inEntity, const DynUpdateFunc& inFunc);
        ScopedUpdaterDyn UpdateDyn(CompClass inClass, Entity inEntity);
        bool HasDyn(CompClass inClass, Entity inEntity) const;
        Mirror::Any FindDyn(CompClass inClass, Entity inEntity);
        Mirror::Any FindDyn(CompClass inClass, Entity inEntity) const;
        Mirror::Any GetDyn(CompClass inClass, Entity inEntity);
        Mirror::Any GetDyn(CompClass inClass, Entity inEntity) const;
        CompEvents& EventsDyn(CompClass inClass);
        Runtime::RuntimeView RuntimeView(const RuntimeFilter& inFilter);
        Runtime::ConstRuntimeView RuntimeView(const RuntimeFilter& inFilter) const;
        Runtime::ConstRuntimeView ConstRuntimeView(const RuntimeFilter& inFilter) const;
        Runtime::EventsObserverDyn EventsObserverDyn(CompClass inClass);

        // global component static
        template <typename G, typename... Args> G& GEmplace(Args&&... inArgs);
        template <typename G> void GRemove();
        template <typename G> void GNotifyUpdated();
        template <typename G, typename F> void GUpdate(F&& inFunc);
        template <typename G> GScopedUpdater<G> GUpdate();
        template <typename G> bool GHas() const;
        template <typename G> G* GFind();
        template <typename G> const G* GFind() const;
        template <typename G> G& GGet();
        template <typename G> const G& GGet() const;
        template <typename G> GCompEvents& GEvents();

        // global component dynamic
        Mirror::Any GEmplaceDyn(GCompClass inClass, const Mirror::ArgumentList& inArgs);
        void GRemoveDyn(GCompClass inClass);
        void GNotifyUpdatedDyn(GCompClass inClass);
        void GUpdateDyn(GCompClass inClass, const DynUpdateFunc& inFunc);
        GScopedUpdaterDyn GUpdateDyn(GCompClass inClass);
        bool GHasDyn(GCompClass inClass) const;
        Mirror::Any GFindDyn(GCompClass inClass);
        Mirror::Any GFindDyn(GCompClass inClass) const;
        Mirror::Any GGetDyn(GCompClass inClass);
        Mirror::Any GGetDyn(GCompClass inClass) const;
        GCompEvents& GEventsDyn(GCompClass inClass);
        void GCompEach(const GCompTraverseFunc& inFunc) const;
        size_t GCompCount() const;

        // comp observer
        Runtime::Observer Observer();

        // serialization
        void Save(ECArchive& outArchive) const;
        void Load(const ECArchive& inArchive);

        // utils
        void CheckEventsUnbound() const;

    private:
        template <typename... T> friend class BasicView;
        template <ECRegistryOrConst R> friend class BasicRuntimeView;

        void NotifyConstructedDyn(CompClass inClass, Entity inEntity);
        void NotifyRemoveDyn(CompClass inClass, Entity inEntity);
        void GNotifyConstructedDyn(GCompClass inClass);
        void GNotifyRemoveDyn(CompClass inClass);
        Internal::Archetype& MoveEntityForAdd(const Internal::CompRtti& inRtti, Entity inEntity);
        void MoveEntityForRemove(CompClass inClass, Entity inEntity);
        void EraseArchetypeElem(Internal::Archetype& inArchetype, size_t inElemIndex);
        void RebindEntityArchetypes();

        Internal::EntityPool entities;
        std::unordered_map<GCompClass, Mirror::Any> globalComps;
        std::unordered_map<Internal::ArchetypeId, Internal::Archetype> archetypes;
        // transients, not copy or move
        std::unordered_map<CompClass, CompEvents> compEvents;
        std::unordered_map<GCompClass, GCompEvents> globalCompEvents;
    };

    enum class SystemExecuteStrategy : uint8_t {
        sequential,
        concurrent,
        max
    };

    class RUNTIME_API SystemGroup {
    public:
        explicit SystemGroup(std::string inName, SystemExecuteStrategy inStrategy);

        template <typename S> Internal::SystemFactory& EmplaceSystem();
        template <typename S> void RemoveSystem();
        template <typename S> bool HasSystem() const;
        template <typename S> Internal::SystemFactory& GetSystem();
        template <typename S> const Internal::SystemFactory& GetSystem() const;
        template <typename SrcSys, typename DstSys> const Internal::SystemFactory& MoveSystemTo();

        Internal::SystemFactory& EmplaceSystemDyn(SystemClass inClass);
        void RemoveSystemDyn(SystemClass inClass);
        bool HasSystemDyn(SystemClass inClass) const;
        Internal::SystemFactory& GetSystemDyn(SystemClass inClass);
        const Internal::SystemFactory& GetSystemDyn(SystemClass inClass) const;
        Internal::SystemFactory& MoveSystemToDyn(SystemClass inSrcClass, SystemClass inDstClass);

        const std::vector<Internal::SystemFactory>& GetSystems();
        const std::vector<Internal::SystemFactory>& GetSystems() const;
        const std::string& GetName() const;
        SystemExecuteStrategy GetStrategy() const;

    private:
        std::vector<Internal::SystemFactory>::iterator FindSystem(SystemClass inClass);
        std::vector<Internal::SystemFactory>::const_iterator FindSystem(SystemClass inClass) const;

        std::string name;
        SystemExecuteStrategy strategy;
        std::vector<Internal::SystemFactory> systems;
    };

    class RUNTIME_API SystemGraph {
    public:
        SystemGraph();

        SystemGroup& AddGroup(const std::string& inName, SystemExecuteStrategy inStrategy);
        void RemoveGroup(const std::string& inName);
        bool HasGroup(const std::string& inName) const;
        SystemGroup& GetGroup(const std::string& inName);
        const SystemGroup& GetGroup(const std::string& inName) const;
        const std::vector<SystemGroup>& GetGroups() const;
        SystemGroup& MoveGroupTo(const std::string& inSrcName, const std::string& inDstName);

    private:
        std::vector<SystemGroup>::iterator FindGroup(const std::string& inName);
        std::vector<SystemGroup>::const_iterator FindGroup(const std::string& inName) const;

        std::vector<SystemGroup> systemGroups;
    };

    class SystemPipeline {
    public:
        explicit SystemPipeline(const SystemGraph& inGraph);

    private:
        struct SystemContext {
            const Internal::SystemFactory& factory;
            Common::UniquePtr<System> instance;
        };

        struct SystemGroupContext {
            std::vector<SystemContext> systems;
            SystemExecuteStrategy strategy;
        };

        friend class SystemGraphExecutor;
        using ActionFunc = std::function<void(SystemContext&)>;

        void ParallelPerformAction(const ActionFunc& inActionFunc);

        std::vector<SystemGroupContext> systemGraph;
    };

    enum class PlayType : uint8_t {
        editor,
        game,
        max
    };

    struct SystemSetupContext {
        SystemSetupContext();

        PlayType playType;
        Client* client;
    };

    class SystemGraphExecutor {
    public:
        explicit SystemGraphExecutor(ECRegistry& inEcRegistry, const SystemGraph& inSystemGraph, const SystemSetupContext& inSetupContext);
        ~SystemGraphExecutor();

        NonCopyable(SystemGraphExecutor)
        NonMovable(SystemGraphExecutor)

        void Tick(float inDeltaTimeSeconds);

    private:
        ECRegistry& ecRegistry;
        SystemGraph systemGraph;
        SystemPipeline pipeline;
    };
}

namespace Runtime::Internal {
    template <typename T>
    const Mirror::Class* GetClass()
    {
        return &Mirror::Class::Get<T>();
    }

    inline CompClass CompRtti::Class() const
    {
        return clazz;
    }

    template <typename C>
    CompRtti CompRtti::Create()
    {
        CompRtti result(GetClass<C>());
        result.copyConstructFrom = [](ElemPtr dst, ElemPtr src) -> void {
            std::construct_at(static_cast<C*>(dst), *static_cast<const C*>(src));
        };
        result.moveConstructFrom = [](ElemPtr dst, ElemPtr src) -> void {
            if constexpr (std::is_move_constructible_v<C>) {
                std::construct_at(static_cast<C*>(dst), std::move(*static_cast<C*>(src)));
            } else {
                std::construct_at(static_cast<C*>(dst), *static_cast<const C*>(src));
            }
        };
        result.destruct = [](ElemPtr elem) -> void {
            std::destroy_at(static_cast<C*>(elem));
        };
        return result;
    }

    template <typename Class, typename Ret, typename... Args>
    struct MemberFuncPtrTraits<Ret(Class::*)(Args...)> {
        static constexpr auto ArgSize = sizeof...(Args);
        using ClassType = Class;
        using ArgsTupleType = std::tuple<Args...>;
    };

    template <typename Class, typename Ret, typename... Args>
    struct MemberFuncPtrTraits<Ret(Class::*)(Args...) const> {
        static constexpr auto ArgSize = sizeof...(Args);
        using ClassType = const Class;
        using ArgsTupleType = std::tuple<Args...>;
    };

    inline auto Archetype::All() const
    {
        return std::views::all(elemMap);
    }

    inline ElemPtr Archetype::GetCompAt(size_t inElemIndex, size_t inCompIndex)
    {
        return CompAt(compMemory[inCompIndex], inCompIndex, inElemIndex);
    }

    inline const void* Archetype::GetCompAt(size_t inElemIndex, size_t inCompIndex) const
    {
        return CompAt(compMemory[inCompIndex], inCompIndex, inElemIndex);
    }

    inline ElemPtr Archetype::GetCompColumn(size_t inCompIndex)
    {
        Assert(inCompIndex < compMemory.size());
        return compMemory[inCompIndex];
    }

    inline const void* Archetype::GetCompColumn(size_t inCompIndex) const
    {
        Assert(inCompIndex < compMemory.size());
        return compMemory[inCompIndex];
    }

    inline ElemPtr Archetype::CompAt(ElemPtr inMemory, size_t inCompIndex, size_t inElemIndex) const
    {
        return static_cast<uint8_t*>(inMemory) + (inElemIndex * compStrides[inCompIndex]);
    }

    inline Entity Archetype::EntityAt(size_t inElemIndex) const
    {
        Assert(inElemIndex < elemMap.size());
        return elemMap[inElemIndex];
    }

    template <typename C>
    C& Archetype::GetComp(size_t inElemIndex)
    {
        Assert(inElemIndex < count);
        return *static_cast<C*>(GetCompAt(inElemIndex, GetCompIndex<C>()));
    }

    template <typename C>
    const C& Archetype::GetComp(size_t inElemIndex) const
    {
        Assert(inElemIndex < count);
        return *static_cast<const C*>(GetCompAt(inElemIndex, GetCompIndex<C>()));
    }

    template <typename C, typename... Args>
    C& Archetype::EmplaceComp(size_t inElemIndex, Args&&... inArgs)
    {
        Assert(inElemIndex < count);
        auto* comp = static_cast<C*>(GetCompAt(inElemIndex, GetCompIndex<C>()));
        return *std::construct_at(comp, std::forward<Args>(inArgs)...);
    }

    template <typename C>
    size_t Archetype::GetCompIndex() const
    {
        const CompClass clazz = GetClass<C>();
        for (size_t i = 0; i < rttiVec.size(); i++) {
            if (rttiVec[i].Class() == clazz) {
                return i;
            }
        }
        Assert(false);
        return 0;
    }
} // namespace Runtime::Internal

namespace Runtime {
    template <typename C>
    ScopedUpdater<C>::ScopedUpdater(ECRegistry& inRegistry, Entity inEntity, C& inCompRef)
        : registry(inRegistry)
        , entity(inEntity)
        , compRef(inCompRef)
    {
    }

    template <typename C>
    ScopedUpdater<C>::~ScopedUpdater()
    {
        registry.NotifyUpdated<C>(entity);
    }

    template <typename C>
    C* ScopedUpdater<C>::operator->() const
    {
        return &compRef;
    }

    template <typename G>
    GScopedUpdater<G>::GScopedUpdater(ECRegistry& inRegistry, G& inGlobalCompRef)
        : registry(inRegistry)
        , globalCompRef(inGlobalCompRef)
    {
    }

    template <typename G>
    GScopedUpdater<G>::~GScopedUpdater()
    {
        registry.GNotifyUpdated<G>();
    }

    template <typename G>
    G* GScopedUpdater<G>::operator->() const
    {
        return &globalCompRef;
    }

    template <typename T>
    T ScopedUpdaterDyn::As() const
    {
        return compRef.As<T>();
    }

    template <typename T>
    T GScopedUpdaterDyn::As() const
    {
        return globalCompRef.As<T>();
    }

    template <ECRegistryOrConst R, typename ... C, typename ... E>
    BasicView<R, Exclude<E...>, C...>::BasicView(R& inRegistry)
        : registry(&inRegistry)
        , materialized(false)
    {
        Evaluate(inRegistry);
    }

    template <ECRegistryOrConst R, typename... C, typename... E>
    template <typename F>
    void BasicView<R, Exclude<E...>, C...>::Each(F&& inFunc) const
    {
        using Traits = Internal::MemberFuncPtrTraits<decltype(&F::operator())>;

        for (const auto& entry : query) {
            auto& archetype = registry->archetypes.at(entry.archetype);
            const auto count = archetype.Count();
            if constexpr (Traits::ArgSize == 1) {
                for (size_t i = 0; i < count; i++) {
                    inFunc(archetype.EntityAt(i));
                }
            } else {
                const CompColumns compColumns = ResolveCompColumns(archetype, entry, std::index_sequence_for<C...> {});
                for (size_t i = 0; i < count; i++) {
                    std::apply(inFunc, MakeResult(archetype, i, compColumns, std::index_sequence_for<C...> {}));
                }
            }
        }
    }

    template <ECRegistryOrConst R, typename... C, typename... E>
    const typename BasicView<R, Exclude<E...>, C...>::ResultVector& BasicView<R, Exclude<E...>, C...>::All() const
    {
        Materialize();
        return result;
    }

    template <ECRegistryOrConst R, typename ... C, typename ... E>
    size_t BasicView<R, Exclude<E...>, C...>::Count() const
    {
        size_t resultCount = 0;
        for (const auto& entry : query) {
            resultCount += registry->archetypes.at(entry.archetype).Count();
        }
        return resultCount;
    }

    template <ECRegistryOrConst R, typename ... C, typename ... E>
    typename BasicView<R, Exclude<E...>, C...>::ConstIter BasicView<R, Exclude<E...>, C...>::Begin() const
    {
        Materialize();
        return result.begin();
    }

    template <ECRegistryOrConst R, typename ... C, typename ... E>
    typename BasicView<R, Exclude<E...>, C...>::ConstIter BasicView<R, Exclude<E...>, C...>::End() const
    {
        Materialize();
        return result.end();
    }

    template <ECRegistryOrConst R, typename ... C, typename ... E>
    typename BasicView<R, Exclude<E...>, C...>::ConstIter BasicView<R, Exclude<E...>, C...>::begin() const
    {
        return Begin();
    }

    template <ECRegistryOrConst R, typename ... C, typename ... E>
    typename BasicView<R, Exclude<E...>, C...>::ConstIter BasicView<R, Exclude<E...>, C...>::end() const
    {
        return End();
    }

    template <ECRegistryOrConst R, typename... C, typename... E>
    template <typename A, size_t... I>
    typename BasicView<R, Exclude<E...>, C...>::CompColumns BasicView<R, Exclude<E...>, C...>::ResolveCompColumns(A& inArchetype, const QueryEntry& inEntry, std::index_sequence<I...>) const
    {
        return { inArchetype.GetCompColumn(inEntry.compIndices[I])... };
    }

    template <ECRegistryOrConst R, typename... C, typename... E>
    template <size_t... I>
    auto BasicView<R, Exclude<E...>, C...>::MakeResult(Internal::Archetype& inArchetype, size_t inElemIndex, const CompColumns& inCompColumns, std::index_sequence<I...>) const
    {
        return typename BasicView<R, Exclude<E...>, C...>::ResultVector::value_type(
            inArchetype.EntityAt(inElemIndex),
            static_cast<std::conditional_t<std::is_const_v<C>, const std::remove_const_t<C>, std::remove_const_t<C>>*>(inCompColumns[I])[inElemIndex]...);
    }

    template <ECRegistryOrConst R, typename... C, typename... E>
    template <size_t... I>
    auto BasicView<R, Exclude<E...>, C...>::MakeResult(const Internal::Archetype& inArchetype, size_t inElemIndex, const CompColumns& inCompColumns, std::index_sequence<I...>) const
    {
        return typename BasicView<R, Exclude<E...>, C...>::ResultVector::value_type(
            inArchetype.EntityAt(inElemIndex),
            static_cast<const std::remove_const_t<C>*>(inCompColumns[I])[inElemIndex]...);
    }

    template <ECRegistryOrConst R, typename... C, typename... E>
    void BasicView<R, Exclude<E...>, C...>::Materialize() const
    {
        if (materialized) {
            return;
        }

        result.reserve(Count());
        for (const auto& entry : query) {
            auto& archetype = registry->archetypes.at(entry.archetype);
            const auto count = archetype.Count();
            const CompColumns compColumns = ResolveCompColumns(archetype, entry, std::index_sequence_for<C...> {});
            for (size_t i = 0; i < count; i++) {
                result.emplace_back(MakeResult(archetype, i, compColumns, std::index_sequence_for<C...> {}));
            }
        }
        materialized = true;
    }

    template <ECRegistryOrConst R, typename... C, typename... E>
    void BasicView<R, Exclude<E...>, C...>::Evaluate(R& inRegistry)
    {
        std::vector<CompClass> includeCompIds;
        includeCompIds.reserve(sizeof...(C));
        (void) std::initializer_list<int> { ([&]() -> void {
            includeCompIds.emplace_back(Internal::GetClass<std::decay_t<C>>());
        }(), 0)... };

        std::vector<CompClass> excludeCompIds;
        excludeCompIds.reserve(sizeof...(E));
        (void) std::initializer_list<int> { ([&]() -> void {
            excludeCompIds.emplace_back(Internal::GetClass<E>());
        }(), 0)... };

        for (auto& archetype : inRegistry.archetypes | std::views::values) {
            if (!archetype.ContainsAll(includeCompIds) || !archetype.NotContainsAny(excludeCompIds)) {
                continue;
            }

            query.emplace_back(QueryEntry {
                archetype.Id(),
                { archetype.GetCompIndex(Internal::GetClass<std::decay_t<C>>())... }
            });
        }
    }

    template <ECRegistryOrConst R>
    BasicRuntimeView<R>::BasicRuntimeView(R& inRegistry, const RuntimeFilter& inFilter)
        : registry(&inRegistry)
        , materialized(false)
    {
        Evaluate(inRegistry, inFilter);
    }

    template <ECRegistryOrConst R>
    template <typename F>
    void BasicRuntimeView<R>::Each(F&& inFunc) const
    {
        using Traits = Internal::MemberFuncPtrTraits<decltype(&F::operator())>;
        const auto compSlots = BuildCompSlots<typename Traits::ArgsTupleType>(std::make_index_sequence<Traits::ArgSize - 1> {});
        std::vector<CompColumnPtr> compColumns;
        compColumns.reserve(includes.size());

        for (const auto& entry : query) {
            auto& archetype = registry->archetypes.at(entry.archetype);
            const auto count = archetype.Count();
            compColumns.clear();
            for (const size_t compIndex : entry.compIndices) {
                compColumns.emplace_back(archetype.GetCompColumn(compIndex));
            }
            for (size_t i = 0; i < count; i++) {
                InvokeTraverseFuncInternal<F, typename Traits::ArgsTupleType>(std::forward<F>(inFunc), archetype, i, compColumns, compSlots, std::make_index_sequence<Traits::ArgSize - 1> {});
            }
        }
    }

    template <ECRegistryOrConst R>
    size_t BasicRuntimeView<R>::Count() const
    {
        size_t resultCount = 0;
        for (const auto& entry : query) {
            resultCount += registry->archetypes.at(entry.archetype).Count();
        }
        return resultCount;
    }

    template <ECRegistryOrConst R>
    typename BasicRuntimeView<R>::ConstIter BasicRuntimeView<R>::Begin() const
    {
        MaterializeEntities();
        return resultEntities.begin();
    }

    template <ECRegistryOrConst R>
    typename BasicRuntimeView<R>::ConstIter BasicRuntimeView<R>::End() const
    {
        MaterializeEntities();
        return resultEntities.end();
    }

    template <ECRegistryOrConst R>
    typename BasicRuntimeView<R>::ConstIter BasicRuntimeView<R>::begin() const
    {
        return Begin();
    }

    template <ECRegistryOrConst R>
    typename BasicRuntimeView<R>::ConstIter BasicRuntimeView<R>::end() const
    {
        return End();
    }

    template <ECRegistryOrConst R>
    template <typename ArgTuple, size_t... I>
    auto BasicRuntimeView<R>::BuildCompSlots(std::index_sequence<I...>) const
    {
        return std::array<size_t, sizeof...(I)> {
            slotMap.at(Internal::GetClass<std::decay_t<std::tuple_element_t<I + 1, ArgTuple>>>())...
        };
    }

    template <ECRegistryOrConst R>
    template <typename F, typename ArgTuple, typename A, size_t... I>
    void BasicRuntimeView<R>::InvokeTraverseFuncInternal(F&& inFunc, A& inArchetype, size_t inElemIndex, const std::vector<CompColumnPtr>& inCompColumns, const std::array<size_t, sizeof...(I)>& inCompSlots, std::index_sequence<I...>) const
    {
        inFunc(inArchetype.EntityAt(inElemIndex), GetCompRef<std::tuple_element_t<I + 1, ArgTuple>>(inElemIndex, inCompColumns, inCompSlots[I])...);
    }

    template <ECRegistryOrConst R>
    template <typename C>
    decltype(auto) BasicRuntimeView<R>::GetCompRef(size_t inElemIndex, const std::vector<CompColumnPtr>& inCompColumns, size_t inCompSlot) const
    {
        static_assert(std::is_reference_v<C>);
        using Value = std::remove_cv_t<std::remove_reference_t<C>>;
        if constexpr (std::is_const_v<std::remove_reference_t<C>> || std::is_const_v<R>) {
            return static_cast<const Value*>(inCompColumns[inCompSlot])[inElemIndex];
        } else {
            return static_cast<Value*>(inCompColumns[inCompSlot])[inElemIndex];
        }
    }

    template <ECRegistryOrConst R>
    void BasicRuntimeView<R>::MaterializeEntities() const
    {
        if (materialized) {
            return;
        }

        resultEntities.reserve(Count());
        for (const auto& entry : query) {
            const auto& archetype = registry->archetypes.at(entry.archetype);
            const auto count = archetype.Count();
            for (size_t i = 0; i < count; i++) {
                resultEntities.emplace_back(archetype.EntityAt(i));
            }
        }
        materialized = true;
    }

    template <ECRegistryOrConst R>
    void BasicRuntimeView<R>::Evaluate(R& inRegistry, const RuntimeFilter& inFilter)
    {
        includes.assign(inFilter.includes.begin(), inFilter.includes.end());
        const std::vector excludes(inFilter.excludes.begin(), inFilter.excludes.end());

        slotMap.reserve(includes.size());
        for (auto i = 0; i < includes.size(); i++) {
            slotMap.emplace(includes[i], i);
        }

        for (auto& archetype : inRegistry.archetypes | std::views::values) {
            if (!archetype.ContainsAll(includes) || !archetype.NotContainsAny(excludes)) {
                continue;
            }

            auto& entry = query.emplace_back();
            entry.archetype = archetype.Id();
            entry.compIndices.reserve(includes.size());
            for (const auto* clazz : includes) {
                entry.compIndices.emplace_back(archetype.GetCompIndex(clazz));
            }
        }
    }

    template <typename C>
    RuntimeFilter& RuntimeFilter::Include()
    {
        const auto* clazz = Internal::GetClass<C>();
        Assert(!includes.contains(clazz));
        includes.emplace(clazz);
        return *this;
    }

    template <typename C>
    RuntimeFilter& RuntimeFilter::Exclude()
    {
        const auto* clazz = Internal::GetClass<C>();
        Assert(!excludes.contains(clazz));
        excludes.emplace(clazz);
        return *this;
    }

    template <typename C>
    Observer& Observer::ObConstructed()
    {
        return OnEvent(registry.Events<C>().onConstructed);
    }

    template <typename C>
    Observer& Observer::ObUpdated()
    {
        return OnEvent(registry.Events<C>().onUpdated);
    }

    template <typename C>
    Observer& Observer::ObRemoved()
    {
        return OnEvent(registry.Events<C>().onRemove);
    }

    template <typename C>
    EventsObserver<C>::EventsObserver(ECRegistry& inRegistry)
        : constructedObserver(inRegistry.Observer())
        , updatedObserver(inRegistry.Observer())
        , removedObserver(inRegistry.Observer())
    {
        constructedObserver.ObConstructed<C>();
        updatedObserver.ObUpdated<C>();
        removedObserver.ObRemoved<C>();
    }

    template <typename C>
    EventsObserver<C>::~EventsObserver() = default;

    template <typename C>
    size_t EventsObserver<C>::ConstructedCount() const
    {
        return constructedObserver.Count();
    }

    template <typename C>
    size_t EventsObserver<C>::UpdatedCount() const
    {
        return updatedObserver.Count();
    }

    template <typename C>
    size_t EventsObserver<C>::RemovedCount() const
    {
        return removedObserver.Count();
    }

    template <typename C>
    void EventsObserver<C>::EachConstructed(const EntityTraverseFunc& inFunc) const
    {
        constructedObserver.Each(inFunc);
    }

    template <typename C>
    void EventsObserver<C>::EachUpdated(const EntityTraverseFunc& inFunc) const
    {
        updatedObserver.Each(inFunc);
    }

    template <typename C>
    void EventsObserver<C>::EachRemoved(const EntityTraverseFunc& inFunc) const
    {
        removedObserver.Each(inFunc);
    }

    template <typename C>
    void EventsObserver<C>::ClearConstructed()
    {
        constructedObserver.Clear();
    }

    template <typename C>
    void EventsObserver<C>::ClearUpdated()
    {
        updatedObserver.Clear();
    }

    template <typename C>
    void EventsObserver<C>::ClearRemoved()
    {
        removedObserver.Clear();
    }

    template <typename C>
    void EventsObserver<C>::Clear()
    {
        ClearConstructed();
        ClearUpdated();
        ClearRemoved();
    }

    template <typename C>
    auto& EventsObserver<C>::Constructed()
    {
        return constructedObserver;
    }

    template <typename C>
    auto& EventsObserver<C>::Updated()
    {
        return updatedObserver;
    }

    template <typename C>
    auto& EventsObserver<C>::Removed()
    {
        return removedObserver;
    }

    template <typename C>
    const Observer& EventsObserver<C>::Constructed() const
    {
        return constructedObserver;
    }

    template <typename C>
    const Observer& EventsObserver<C>::Updated() const
    {
        return updatedObserver;
    }

    template <typename C>
    const Observer& EventsObserver<C>::Removed() const
    {
        return removedObserver;
    }

    template <typename C, typename ... Args>
    C& ECRegistry::Emplace(Entity inEntity, Args&&... inArgs)
    {
        const Internal::CompRtti rtti = Internal::CompRtti::Create<C>();
        Internal::Archetype& archetype = MoveEntityForAdd(rtti, inEntity);
        C& result = archetype.template EmplaceComp<C>(entities.GetElemIndex(inEntity), std::forward<Args>(inArgs)...);
        NotifyConstructedDyn(Internal::GetClass<C>(), inEntity);
        return result;
    }

    template <typename C>
    void ECRegistry::Remove(Entity inEntity)
    {
        MoveEntityForRemove(Internal::GetClass<C>(), inEntity);
    }

    template <typename C, typename F>
    void ECRegistry::Update(Entity inEntity, F&& inFunc)
    {
        UpdateDyn(Internal::GetClass<C>(), inEntity, [&](const Mirror::Any& ref) -> void {
            inFunc(ref.As<C&>());
        });
    }

    template <typename C>
    ScopedUpdater<C> ECRegistry::Update(Entity inEntity)
    {
        Assert(Valid(inEntity) && Has<C>(inEntity));
        return { *this, inEntity, Get<C>(inEntity) };
    }

    template <typename C>
    bool ECRegistry::Has(Entity inEntity) const
    {
        Assert(Valid(inEntity));
        return entities.GetArchetypePtr(inEntity)->Contains(Internal::GetClass<C>());
    }

    template <typename C>
    C* ECRegistry::Find(Entity inEntity)
    {
        return Has<C>(inEntity) ? &Get<C>(inEntity) : nullptr;
    }

    template <typename C>
    const C* ECRegistry::Find(Entity inEntity) const
    {
        return Has<C>(inEntity) ? &Get<C>(inEntity) : nullptr;
    }

    template <typename C>
    C& ECRegistry::Get(Entity inEntity)
    {
        Assert(Valid(inEntity));
        return entities.GetArchetypePtr(inEntity)->template GetComp<C>(entities.GetElemIndex(inEntity));
    }

    template <typename C>
    const C& ECRegistry::Get(Entity inEntity) const
    {
        Assert(Valid(inEntity));
        return entities.GetArchetypePtr(inEntity)->template GetComp<C>(entities.GetElemIndex(inEntity));
    }

    template <typename... C, typename... E>
    Runtime::View<ECRegistry, Exclude<E...>, C...> ECRegistry::View(Exclude<E...>)
    {
        return Runtime::View<ECRegistry, Exclude<E...>, C...>(*this);
    }

    template <typename ... C, typename ... E>
    Runtime::ConstView<ECRegistry, Exclude<E...>, C...> ECRegistry::View(Exclude<E...>) const
    {
        return Runtime::ConstView<ECRegistry, Exclude<E...>, C...>(*this);
    }

    template <typename ... C, typename ... E>
    Runtime::ConstView<ECRegistry, Exclude<E...>, C...> ECRegistry::ConstView(Exclude<E...>) const
    {
        return Runtime::ConstView<ECRegistry, Exclude<E...>, C...>(*this);
    }

    template <typename C>
    ECRegistry::CompEvents& ECRegistry::Events()
    {
        return EventsDyn(Internal::GetClass<C>());
    }

    template <typename C>
    EventsObserver<C> ECRegistry::EventsObserver()
    {
        return Runtime::EventsObserver<C> { *this };
    }

    template <typename C>
    void ECRegistry::NotifyUpdated(Entity inEntity)
    {
        NotifyUpdatedDyn(Internal::GetClass<C>(), inEntity);
    }

    template <typename G, typename ... Args>
    G& ECRegistry::GEmplace(Args&&... inArgs)
    {
        return GEmplaceDyn(Internal::GetClass<G>(), Mirror::ForwardAsArgList(std::forward<Args>(inArgs)...)).template As<G&>();
    }

    template <typename G>
    void ECRegistry::GRemove()
    {
        return GRemoveDyn(Internal::GetClass<G>());
    }

    template <typename G, typename F>
    void ECRegistry::GUpdate(F&& inFunc)
    {
        GUpdateDyn(Internal::GetClass<G>(), [&](const Mirror::Any& ref) -> void {
            inFunc(ref.As<G&>());
        });
    }

    template <typename G>
    GScopedUpdater<G> ECRegistry::GUpdate()
    {
        Assert(GHas<G>());
        return { *this, GGet<G>() };
    }

    template <typename G>
    bool ECRegistry::GHas() const
    {
        return GHasDyn(Internal::GetClass<G>());
    }

    template <typename G>
    G* ECRegistry::GFind()
    {
        return GHas<G>() ? &GGet<G>() : nullptr;
    }

    template <typename G>
    const G* ECRegistry::GFind() const
    {
        return GHas<G>() ? &GGet<G>() : nullptr;
    }

    template <typename G>
    G& ECRegistry::GGet()
    {
        return GGetDyn(Internal::GetClass<G>()).template As<G&>();
    }

    template <typename G>
    const G& ECRegistry::GGet() const
    {
        return GGetDyn(Internal::GetClass<G>()).template As<const G&>();
    }

    template <typename G>
    ECRegistry::GCompEvents& ECRegistry::GEvents()
    {
        return GEventsDyn(Internal::GetClass<G>());
    }

    template <typename G>
    void ECRegistry::GNotifyUpdated()
    {
        GNotifyUpdatedDyn(Internal::GetClass<G>());
    }

    template <typename S>
    Internal::SystemFactory& SystemGroup::EmplaceSystem()
    {
        return EmplaceSystemDyn(Internal::GetClass<S>());
    }

    template <typename S>
    void SystemGroup::RemoveSystem()
    {
        RemoveSystemDyn(Internal::GetClass<S>());
    }

    template <typename S>
    bool SystemGroup::HasSystem() const
    {
        return HasSystemDyn(Internal::GetClass<S>());
    }

    template <typename S>
    Internal::SystemFactory& SystemGroup::GetSystem()
    {
        return GetSystemDyn(Internal::GetClass<S>());
    }

    template <typename S>
    const Internal::SystemFactory& SystemGroup::GetSystem() const
    {
        return GetSystemDyn(Internal::GetClass<S>());
    }

    template <typename SrcSys, typename DstSys>
    const Internal::SystemFactory& SystemGroup::MoveSystemTo()
    {
        return MoveSystemToDyn(Internal::GetClass<SrcSys>(), Internal::GetClass<DstSys>());
    }
} // namespace Runtime
