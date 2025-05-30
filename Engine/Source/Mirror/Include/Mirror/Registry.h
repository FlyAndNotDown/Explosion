//
// Created by johnk on 2022/9/10.
//

#pragma once

#include <Common/Debug.h>
#include <Common/Container.h>
#include <Mirror/Api.h>
#include <Mirror/Mirror.h>

namespace Mirror::Internal {
    template <typename T> struct VariableTraits {};
    template <typename T> struct FunctionTraits {};
    template <typename T> struct MemberVariableTraits {};
    template <typename T> struct MemberFunctionTraits {};

    template <typename ArgsTuple, size_t... I> auto GetArgTypeInfosByArgsTuple(std::index_sequence<I...>);
    template <auto Ptr, typename ArgsTuple, size_t... I> decltype(auto) InvokeFunction(const ArgumentList& args, std::index_sequence<I...>);
    template <typename Class, auto Ptr, typename ArgsTuple, size_t... I> decltype(auto) InvokeMemberFunction(Class& object, const ArgumentList& args, std::index_sequence<I...>);
    template <typename Class, typename ArgsTuple, size_t... I> decltype(auto) InvokeConstructorStack(const ArgumentList& args, std::index_sequence<I...>);
    template <typename Class, typename ArgsTuple, size_t... I> decltype(auto) InvokeConstructorNew(const ArgumentList& args, std::index_sequence<I...>);
    template <typename Class, typename ArgsTuple, size_t... I> decltype(auto) InvokeConstructorInplace(void* ptr, const ArgumentList& args, std::index_sequence<I...>);

    class MIRROR_API ScopedReleaser {
    public:
        using ReleaseFunc = std::function<void()>;
        explicit ScopedReleaser(ReleaseFunc inReleaseFunc = {});
        ~ScopedReleaser();

    private:
        ReleaseFunc releaseFunc;
    };
}

namespace Mirror {
    template <typename Derived>
    class MetaDataRegistry {
    public:
        virtual ~MetaDataRegistry();

        Derived& MetaData(const Id& inKey, const std::string& inValue);

    protected:
        explicit MetaDataRegistry(ReflNode* inContext);

        Derived& SetContext(ReflNode* inContext);

    private:
        ReflNode* context;
    };

    template <typename C>
    class ClassRegistry final : public MetaDataRegistry<ClassRegistry<C>> {
    public:
        ~ClassRegistry() override;

        template <typename... Args> ClassRegistry& Constructor(const Id& inId);
        template <FieldAccess Access, typename... Args> ClassRegistry& Constructor(const Id& inId);
        template <auto Ptr, FieldAccess Access = FieldAccess::faPublic> ClassRegistry& StaticVariable(const Id& inId);
        template <auto Ptr, FieldAccess Access = FieldAccess::faPublic> ClassRegistry& StaticFunction(const Id& inId);
        template <auto Ptr, FieldAccess Access = FieldAccess::faPublic> ClassRegistry& MemberVariable(const Id& inId);
        template <auto Ptr, FieldAccess Access = FieldAccess::faPublic> ClassRegistry& MemberFunction(const Id& inId);

    private:
        friend class Registry;

        explicit ClassRegistry(Class& inClass);

        Class& clazz;
    };

    class MIRROR_API GlobalRegistry final : public MetaDataRegistry<GlobalRegistry> {
    public:
        ~GlobalRegistry() override;

        template <auto Ptr> GlobalRegistry& Variable(const Id& inId);
        template <auto Ptr> GlobalRegistry& Function(const Id& inId);
        void UnloadVariable(const Id& inId);
        void UnloadFunction(const Id& inId);

    private:
        friend class Registry;

        explicit GlobalRegistry(GlobalScope& inGlobalScope);

        GlobalScope& globalScope;
    };

    template <typename T>
    class EnumRegistry final : public MetaDataRegistry<EnumRegistry<T>> {
    public:
        ~EnumRegistry() override;

        template <T V> EnumRegistry& Value(const Id& inId);

    private:
        friend class Registry;

        explicit EnumRegistry(Enum& inEnum);

        Enum& enumInfo;
    };

    template <typename B, typename C> concept CppBaseClassOrVoid = Common::CppVoid<B> || Common::CppClass<B> && Common::CppClass<C> && std::is_base_of_v<B, C>;

    class MIRROR_API Registry {
    public:
        static Registry& Get();

        ~Registry();

        GlobalRegistry Global();

        template <Common::CppClass C, CppBaseClassOrVoid<C> B = void, FieldAccess DefaultCtorAccess = FieldAccess::faPublic, FieldAccess DestructorAccess = FieldAccess::faPublic> ClassRegistry<C> Class(const Id& inId);
        template <Common::CppEnum T> EnumRegistry<T> Enum(const Id& inId);
        void UnloadClass(const Id& inId);
        void UnloadEnum(const Id& inId);

    private:
        friend class GlobalScope;
        friend class Class;
        friend class Enum;

        Registry() noexcept;

        Mirror::Class& EmplaceClass(const Id& inId, Class::ConstructParams&& inParams);
        Mirror::Enum& EmplaceEnum(const Id& inId, Enum::ConstructParams&& inParams);

        GlobalScope globalScope;
        Common::StableUnorderedMap<Id, Mirror::Class, 128, IdHashProvider> classes;
        Common::StableUnorderedMap<Id, Mirror::Enum, 128, IdHashProvider> enums;
    };
}

namespace Mirror::Internal {
    template <typename T>
    struct VariableTraits<T*> {
        using ValueType = T;
    };

    template <typename Ret, typename... Args>
    struct FunctionTraits<Ret(*)(Args...)> {
        using RetType = Ret;
        using ArgsTupleType = std::tuple<Args...>;
    };

    template <typename Class, typename T>
    struct MemberVariableTraits<T Class::*> {
        using ClassType = Class;
        using ValueType = T;
    };

    template <typename Class, typename Ret, typename... Args>
    struct MemberFunctionTraits<Ret(Class::*)(Args...)> {
        using ClassType = Class;
        using RetType = Ret;
        using ArgsTupleType = std::tuple<Args...>;
    };

    template <typename Class, typename Ret, typename... Args>
    struct MemberFunctionTraits<Ret(Class::*)(Args...) const> {
        using ClassType = const Class;
        using RetType = Ret;
        using ArgsTupleType = std::tuple<Args...>;
    };

    template <typename ArgsTuple, size_t... I>
    auto GetArgTypeInfosByArgsTuple(std::index_sequence<I...>)
    {
        return std::vector<const TypeInfo*> { GetTypeInfo<std::tuple_element_t<I, ArgsTuple>>()... };
    }

    template <auto Ptr, typename ArgsTuple, size_t... I>
    decltype(auto) InvokeFunction(const ArgumentList& args, std::index_sequence<I...>)
    {
        return Ptr(args[I].template As<std::tuple_element_t<I, ArgsTuple>>()...);
    }

    template <typename Class, auto Ptr, typename ArgsTuple, size_t... I>
    decltype(auto) InvokeMemberFunction(Class& object, const ArgumentList& args, std::index_sequence<I...>)
    {
        return (object.*Ptr)(args[I].template As<std::tuple_element_t<I, ArgsTuple>>()...);
    }

    template <typename Class, typename ArgsTuple, size_t... I>
    decltype(auto) InvokeConstructorStack(const ArgumentList& args, std::index_sequence<I...>)
    {
        return Class(args[I].template As<std::tuple_element_t<I, ArgsTuple>>()...);
    }

    template <typename Class, typename ArgsTuple, size_t... I>
    decltype(auto) InvokeConstructorNew(const ArgumentList& args, std::index_sequence<I...>)
    {
        return new Class(args[I].template As<std::tuple_element_t<I, ArgsTuple>>()...);
    }

    template <typename Class, typename ArgsTuple, size_t... I>
    decltype(auto) InvokeConstructorInplace(void* ptr, const ArgumentList& args, std::index_sequence<I...>)
    {
        new(ptr) Class(args[I].template As<std::tuple_element_t<I, ArgsTuple>>()...);
        return *static_cast<Class*>(ptr);
    }
}

namespace Mirror {
    template <typename Derived>
    MetaDataRegistry<Derived>::MetaDataRegistry(ReflNode* inContext)
        : context(inContext)
    {
        Assert(context);
    }

    template <typename Derived>
    MetaDataRegistry<Derived>::~MetaDataRegistry() = default;

    template <typename Derived>
    Derived& MetaDataRegistry<Derived>::MetaData(const Id& inKey, const std::string& inValue)
    {
        context->metas[inKey] = inValue;
        return static_cast<Derived&>(*this);
    }

    template <typename Derived>
    Derived& MetaDataRegistry<Derived>::SetContext(ReflNode* inContext)
    {
        Assert(inContext);
        context = inContext;
        return static_cast<Derived&>(*this);
    }

    template <typename C>
    ClassRegistry<C>::ClassRegistry(Class& inClass)
        : MetaDataRegistry<ClassRegistry<C>>(&inClass), clazz(inClass)
    {
    }

    template <typename C>
    ClassRegistry<C>::~ClassRegistry() = default;

    template <typename C>
    template <typename ... Args>
    ClassRegistry<C>& ClassRegistry<C>::Constructor(const Id& inId)
    {
        return Constructor<FieldAccess::faPublic, Args...>(inId);
    }

    template <typename C>
    template <FieldAccess Access, typename... Args>
    ClassRegistry<C>& ClassRegistry<C>::Constructor(const Id& inId)
    {
        using ArgsTupleType = std::tuple<Args...>;
        constexpr size_t argsTupleSize = std::tuple_size_v<ArgsTupleType>;

        const auto iter = clazz.constructors.find(inId);
        Assert(iter == clazz.constructors.end());

        Constructor::ConstructParams params;
        params.id = inId;
        params.owner = clazz.GetId();
        params.access = Access;
        params.argsNum = sizeof...(Args);
        params.argTypeInfos = { GetTypeInfo<Args>()... };
        params.argRemoveRefTypeInfos = { GetTypeInfo<std::remove_reference_t<Args>>()... };
        params.argRemovePointerTypeInfos = { GetTypeInfo<std::remove_pointer_t<Args>>()... };
        params.stackConstructor = [](const ArgumentList& args) -> Any {
            if constexpr (!std::is_abstract_v<C> && (std::is_copy_constructible_v<C> || std::is_move_constructible_v<C>)) {
                Assert(argsTupleSize == args.size());
                return ForwardAsAny(Internal::InvokeConstructorStack<C, ArgsTupleType>(args, std::make_index_sequence<argsTupleSize> {}));
            } else {
                QuickFail();
                return {};
            }
        };
        params.heapConstructor = [](const ArgumentList& args) -> Any {
            if constexpr (!std::is_abstract_v<C>) {
                Assert(argsTupleSize == args.size());
                return ForwardAsAny(Internal::InvokeConstructorNew<C, ArgsTupleType>(args, std::make_index_sequence<argsTupleSize> {}));
            } else {
                QuickFail();
                return {};
            }
        };
        params.inplaceConstructor = [](void* ptr, const ArgumentList& args) -> Any {
            if constexpr (!std::is_abstract_v<C>) {
                Assert(argsTupleSize == args.size());
                return ForwardAsAny(std::ref(Internal::InvokeConstructorInplace<C, ArgsTupleType>(ptr, args, std::make_index_sequence<argsTupleSize> {})));
            } else {
                QuickFail();
                return {};
            }
        };

        return MetaDataRegistry<ClassRegistry>::SetContext(&clazz.EmplaceConstructor(inId, std::move(params)));
    }

    template <typename C>
    template <auto Ptr, FieldAccess Access>
    ClassRegistry<C>& ClassRegistry<C>::StaticVariable(const Id& inId)
    {
        using ValueType = typename Internal::VariableTraits<decltype(Ptr)>::ValueType;

        const auto iter = clazz.staticVariables.find(inId);
        Assert(iter == clazz.staticVariables.end());

        Variable::ConstructParams params;
        params.id = inId;
        params.owner = clazz.GetId();
        params.access = Access;
        params.memorySize = sizeof(ValueType);
        params.typeInfo = GetTypeInfo<ValueType>();
        params.setter = [](const Argument& value) -> void {
            if constexpr (!std::is_const_v<ValueType>) {
                *Ptr = value.As<const ValueType&>();
            } else {
                QuickFail();
            }
        };
        params.getter = []() -> Any {
            return { std::ref(*Ptr) };
        };

        return MetaDataRegistry<ClassRegistry>::SetContext(&clazz.EmplaceStaticVariable(inId, std::move(params)));
    }

    template <typename C>
    template <auto Ptr, FieldAccess Access>
    ClassRegistry<C>& ClassRegistry<C>::StaticFunction(const Id& inId)
    {
        using ArgsTupleType = typename Internal::FunctionTraits<decltype(Ptr)>::ArgsTupleType;
        using RetType = typename Internal::FunctionTraits<decltype(Ptr)>::RetType;

        const auto iter = clazz.staticFunctions.find(inId);
        Assert(iter == clazz.staticFunctions.end());

        constexpr size_t argsTupleSize = std::tuple_size_v<ArgsTupleType>;

        Function::ConstructParams params;
        params.id = inId;
        params.owner = clazz.GetId();
        params.access = Access;
        params.retTypeInfo = GetTypeInfo<RetType>();
        params.argsNum = argsTupleSize;
        params.argTypeInfos = Internal::GetArgTypeInfosByArgsTuple<ArgsTupleType>(std::make_index_sequence<argsTupleSize> {});
        params.invoker = [](const ArgumentList& args) -> Any {
            Assert(argsTupleSize == args.size());

            if constexpr (std::is_void_v<RetType>) {
                Internal::InvokeFunction<Ptr, ArgsTupleType>(args, std::make_index_sequence<argsTupleSize> {});
                return {};
            } else {
                return ForwardAsAny(Internal::InvokeFunction<Ptr, ArgsTupleType>(args, std::make_index_sequence<argsTupleSize> {}));
            }
        };

        return MetaDataRegistry<ClassRegistry>::SetContext(&clazz.EmplaceStaticFunction(inId, std::move(params)));
    }

    template <typename C>
    template <auto Ptr, FieldAccess Access>
    ClassRegistry<C>& ClassRegistry<C>::MemberVariable(const Id& inId)
    {
        using ClassType = typename Internal::MemberVariableTraits<decltype(Ptr)>::ClassType;
        using ValueType = typename Internal::MemberVariableTraits<decltype(Ptr)>::ValueType;

        const auto iter = clazz.memberVariables.find(inId);
        Assert(iter == clazz.memberVariables.end());

        MemberVariable::ConstructParams params;
        params.id = inId;
        params.owner = clazz.GetId();
        params.access = Access;
        params.memorySize = sizeof(ValueType);
        params.typeInfo = GetTypeInfo<ValueType>();
        params.setter = [](const Argument& object, const Argument& value) -> void {
            Assert(!object.IsConstRef());
            object.As<ClassType&>().*Ptr = value.As<const ValueType&>();
        };
        params.getter = [](const Argument& object) -> Any {
            if (object.IsConstRef()) {
                return { std::ref(object.As<const ClassType&>().*Ptr) };
            }
            return { std::ref(object.As<ClassType&>().*Ptr) };
        };
        return MetaDataRegistry<ClassRegistry>::SetContext(&clazz.EmplaceMemberVariable(inId, std::move(params)));
    }

    template <typename C>
    template <auto Ptr, FieldAccess Access>
    ClassRegistry<C>& ClassRegistry<C>::MemberFunction(const Id& inId)
    {
        // ClassType here contains const, #see Internal::MemberFunctionTraits
        using ClassType = typename Internal::MemberFunctionTraits<decltype(Ptr)>::ClassType;
        using ArgsTupleType = typename Internal::MemberFunctionTraits<decltype(Ptr)>::ArgsTupleType;
        using RetType = typename Internal::MemberFunctionTraits<decltype(Ptr)>::RetType;

        const auto iter = clazz.memberFunctions.find(inId);
        Assert(iter == clazz.memberFunctions.end());

        constexpr size_t argsTupleSize = std::tuple_size_v<ArgsTupleType>;

        MemberFunction::ConstructParams params;
        params.id = inId;
        params.owner = clazz.GetId();
        params.access = Access;
        params.retTypeInfo = GetTypeInfo<RetType>();
        params.argsNum = argsTupleSize;
        params.argTypeInfos = Internal::GetArgTypeInfosByArgsTuple<ArgsTupleType>(std::make_index_sequence<argsTupleSize> {});
        params.invoker = [](const Argument& object, const ArgumentList& args) -> Any {
            Assert(argsTupleSize == args.size());

            if constexpr (std::is_void_v<RetType>) {
                Internal::InvokeMemberFunction<ClassType, Ptr, ArgsTupleType>(object.As<ClassType&>(), args, std::make_index_sequence<argsTupleSize> {});
                return {};
            } else {
                return ForwardAsAny(Internal::InvokeMemberFunction<ClassType, Ptr, ArgsTupleType>(object.As<ClassType&>(), args, std::make_index_sequence<argsTupleSize> {}));
            }
        };

        return MetaDataRegistry<ClassRegistry>::SetContext(&clazz.EmplaceMemberFunction(inId, std::move(params)));
    }

    template <auto Ptr>
    GlobalRegistry& GlobalRegistry::Variable(const Id& inId)
    {
        using ValueType = typename Internal::VariableTraits<decltype(Ptr)>::ValueType;

        Assert(!globalScope.variables.Contains(inId));

        Variable::ConstructParams params;
        params.id = inId;
        params.owner = Id::null;
        params.access = FieldAccess::max;
        params.memorySize = sizeof(ValueType);
        params.typeInfo = GetTypeInfo<ValueType>();
        params.setter = [](const Argument& argument) -> void {
            if (!std::is_const_v<ValueType>) {
                *Ptr = argument.As<const ValueType&>();
            } else {
                QuickFail();
            }
        };
        params.getter = []() -> Any {
            return { std::ref(*Ptr) };
        };
        return SetContext(&globalScope.EmplaceVariable(inId, std::move(params)));
    }

    template <auto Ptr>
    GlobalRegistry& GlobalRegistry::Function(const Id& inId)
    {
        using ArgsTupleType = typename Internal::FunctionTraits<decltype(Ptr)>::ArgsTupleType;
        using RetType = typename Internal::FunctionTraits<decltype(Ptr)>::RetType;
        constexpr size_t argsTupleSize = std::tuple_size_v<ArgsTupleType>;

        Assert(!globalScope.functions.Contains(inId));

        Function::ConstructParams params;
        params.id = inId;
        params.owner = Id::null;
        params.access = FieldAccess::max;
        params.retTypeInfo = GetTypeInfo<RetType>();
        params.argsNum = argsTupleSize;
        params.argTypeInfos = Internal::GetArgTypeInfosByArgsTuple<ArgsTupleType>(std::make_index_sequence<argsTupleSize> {});
        params.invoker = [](const ArgumentList& args) -> Any {
            Assert(argsTupleSize == args.size());

            if constexpr (std::is_void_v<RetType>) {
                Internal::InvokeFunction<Ptr, ArgsTupleType>(args, std::make_index_sequence<argsTupleSize> {});
                return {};
            } else {
                return ForwardAsAny(Internal::InvokeFunction<Ptr, ArgsTupleType>(args, std::make_index_sequence<argsTupleSize> {}));
            }
        };

        return SetContext(&globalScope.EmplaceFunction(inId, std::move(params)));
    }

    template <typename T>
    EnumRegistry<T>::EnumRegistry(Enum& inEnum)
        : MetaDataRegistry<EnumRegistry>(&inEnum), enumInfo(inEnum)
    {
    }

    template <typename T>
    EnumRegistry<T>::~EnumRegistry() = default;

    template <typename T>
    template <T V>
    EnumRegistry<T>& EnumRegistry<T>::Value(const Id& inId)
    {
        const auto iter = enumInfo.values.find(inId);
        Assert(iter == enumInfo.values.end());

        EnumValue::ConstructParams params;
        params.id = inId;
        params.owner = enumInfo.GetId();
        params.getter = []() -> Any {
            return { V };
        };
        params.integralGetter = []() -> EnumValue::IntegralValue {
            return static_cast<EnumValue::IntegralValue>(V);
        };
        params.setter = [](const Argument& value) -> void {
            value.As<T&>() = V;
        };
        params.comparer = [](const Argument& value) -> bool {
            return value.As<T>() == V;
        };

        return MetaDataRegistry<EnumRegistry<T>>::SetContext(&enumInfo.EmplaceElement(inId, std::move(params)));
    }

    template <Common::CppClass C, CppBaseClassOrVoid<C> B, FieldAccess DefaultCtorAccess, FieldAccess DetorAccess>
    ClassRegistry<C> Registry::Class(const Id& inId)
    {
        const auto typeId = GetTypeInfo<C>()->id;
        Assert(!Class::typeToIdMap.contains(typeId));
        Assert(!classes.Contains(inId));

        Class::ConstructParams params;
        params.id = inId;
        params.typeInfo = GetTypeInfo<C>();
        params.memorySize = sizeof(C);
        params.baseClassGetter = []() -> const Mirror::Class* {
            if constexpr (std::is_void_v<B>) {
                return nullptr;
            } else {
                return &Mirror::Class::Get<B>();
            }
        };
        params.inplaceGetter = [](void* ptr) -> Any {
            return { std::ref(*static_cast<C*>(ptr)) };
        };
        params.caster = [](const Argument& argument) -> Any {
            if (argument.Type()->isPointer) {
                return argument.Type()->isConstPointer ? argument.As<const C*>() : argument.As<C*>();
            }
            if (argument.IsRef()) {
                return std::ref(argument.IsConstRef() ? argument.As<const C&>() : argument.As<C&>());
            }
            QuickFail();
            return {};
        };
        if constexpr (std::is_default_constructible_v<C>) {
            params.defaultObjectCreator = []() -> Any {
                return { C() };
            };
        }
        if constexpr (std::is_destructible_v<C>) {
            Destructor::ConstructParams detorParams;
            detorParams.owner = inId;
            detorParams.access = DetorAccess;
            detorParams.destructor = [](const Argument& object) -> void {
                object.As<C&>().~C();
            };
            detorParams.deleter = [](const Argument& object) -> void {
                delete object.As<C*>();
            };
            params.destructorParams = std::move(detorParams);
        }
        if constexpr (std::is_default_constructible_v<C>) {
            Constructor::ConstructParams ctorParams;
            ctorParams.id = IdPresets::defaultCtor.name;
            ctorParams.owner = inId;
            ctorParams.access = DefaultCtorAccess;
            ctorParams.argsNum = 0;
            ctorParams.argTypeInfos = {};
            ctorParams.argRemoveRefTypeInfos = {};
            ctorParams.argRemovePointerTypeInfos = {};
            ctorParams.stackConstructor = [](const ArgumentList& args) -> Any {
                if constexpr (std::is_copy_constructible_v<C> || std::is_move_constructible_v<C>) {
                    Assert(args.empty());
                    return { C() };
                } else {
                    QuickFail();
                    return {};
                }
            };
            ctorParams.heapConstructor = [](const ArgumentList& args) -> Any {
                Assert(args.empty());
                return { new C() };
            };
            ctorParams.inplaceConstructor = [](void* ptr, const ArgumentList& args) -> Any {
                Assert(ptr != nullptr && args.empty());
                new(ptr) C();
                return std::ref(*static_cast<C*>(ptr));
            };
            params.defaultConstructorParams = ctorParams;
        }
        if constexpr (std::is_copy_constructible_v<C>) {
            Constructor::ConstructParams copyCtorParams;
            copyCtorParams.id = IdPresets::copyCtor.name;
            copyCtorParams.owner = inId;
            copyCtorParams.access = FieldAccess::faPublic;
            copyCtorParams.argsNum = 1;
            copyCtorParams.argTypeInfos = { GetTypeInfo<const C&>() };
            copyCtorParams.argRemoveRefTypeInfos = { GetTypeInfo<std::remove_reference_t<const C&>>() };
            copyCtorParams.argRemovePointerTypeInfos = { GetTypeInfo<std::remove_pointer_t<const C&>>() };
            copyCtorParams.stackConstructor = [](const ArgumentList& args) -> Any {
                if constexpr (std::is_copy_constructible_v<C> || std::is_move_constructible_v<C>) {
                    Assert(args.size() == 1);
                    return { C(args[0].As<const C&>()) };
                } else {
                    QuickFail();
                    return {};
                }
            };
            copyCtorParams.heapConstructor = [](const ArgumentList& args) -> Any {
                Assert(args.size() == 1);
                return { new C(args[0].As<const C&>()) };
            };
            copyCtorParams.inplaceConstructor = [](void* ptr, const ArgumentList& args) -> Any {
                Assert(ptr != nullptr && args.size() == 1);
                new(ptr) C(args[0].As<const C&>());
                return std::ref(*static_cast<C*>(ptr));
            };
            params.copyConstructorParams = std::move(copyCtorParams);
        }
        if constexpr (std::is_move_constructible_v<C>) {
            Constructor::ConstructParams moveCtorParams;
            moveCtorParams.id = IdPresets::moveCtor.name;
            moveCtorParams.owner = inId;
            moveCtorParams.access = FieldAccess::faPublic;
            moveCtorParams.argsNum = 1;
            moveCtorParams.argTypeInfos = { GetTypeInfo<C&&>() };
            moveCtorParams.argRemoveRefTypeInfos = { GetTypeInfo<std::remove_reference_t<C&&>>() };
            moveCtorParams.argRemovePointerTypeInfos = { GetTypeInfo<std::remove_pointer_t<C&&>>() };
            moveCtorParams.stackConstructor = [](const ArgumentList& args) -> Any {
                if constexpr (std::is_copy_constructible_v<C> || std::is_move_constructible_v<C>) {
                    Assert(args.size() == 1);
                    return { C(args[0].As<C&&>()) };
                } else {
                    QuickFail();
                    return {};
                }
            };
            moveCtorParams.heapConstructor = [](const ArgumentList& args) -> Any {
                Assert(args.size() == 1);
                return { new C(args[0].As<C&&>()) };
            };
            moveCtorParams.inplaceConstructor = [](void* ptr, const ArgumentList& args) -> Any {
                Assert(ptr != nullptr && args.size() == 1);
                new(ptr) C(args[0].As<C&&>());
                return std::ref(*static_cast<C*>(ptr));
            };
            params.moveConstructorParams = std::move(moveCtorParams);
        }

        Class::typeToIdMap[typeId] = inId;
        return ClassRegistry<C>(EmplaceClass(inId, std::move(params)));
    }

    template <Common::CppEnum T>
    EnumRegistry<T> Registry::Enum(const Id& inId)
    {
        const auto typeId = GetTypeInfo<T>()->id;
        Assert(!Enum::typeToIdMap.contains(typeId));
        Assert(!enums.Contains(inId));

        Enum::ConstructParams params;
        params.id = inId;

        Enum::typeToIdMap[typeId] = inId;
        return EnumRegistry<T>(EmplaceEnum(inId, std::move(params)));
    }
}
