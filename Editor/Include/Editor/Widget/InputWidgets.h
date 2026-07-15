#pragma once

#include <cstdint>
#include <string>

#include <Common/Math/Color.h>
#include <Common/Math/Quaternion.h>
#include <Common/Math/Transform.h>
#include <Common/Math/Vector.h>
#include <Core/Uri.h>
#include <Mirror/Mirror.h>

namespace Editor {
    template <typename T>
    class InputWidget;

    template <>
    class InputWidget<bool> final {
    public:
        static bool Render(const std::string& inLabel, bool& inValue);
    };

    template <>
    class InputWidget<int8_t> final {
    public:
        static bool Render(const std::string& inLabel, int8_t& inValue);
    };

    template <>
    class InputWidget<uint8_t> final {
    public:
        static bool Render(const std::string& inLabel, uint8_t& inValue);
    };

    template <>
    class InputWidget<int16_t> final {
    public:
        static bool Render(const std::string& inLabel, int16_t& inValue);
    };

    template <>
    class InputWidget<uint16_t> final {
    public:
        static bool Render(const std::string& inLabel, uint16_t& inValue);
    };

    template <>
    class InputWidget<int32_t> final {
    public:
        static bool Render(const std::string& inLabel, int32_t& inValue);
    };

    template <>
    class InputWidget<uint32_t> final {
    public:
        static bool Render(const std::string& inLabel, uint32_t& inValue);
    };

    template <>
    class InputWidget<int64_t> final {
    public:
        static bool Render(const std::string& inLabel, int64_t& inValue);
    };

    template <>
    class InputWidget<uint64_t> final {
    public:
        static bool Render(const std::string& inLabel, uint64_t& inValue);
    };

    template <>
    class InputWidget<float> final {
    public:
        static bool Render(const std::string& inLabel, float& inValue);
    };

    template <>
    class InputWidget<double> final {
    public:
        static bool Render(const std::string& inLabel, double& inValue);
    };

    template <>
    class InputWidget<std::string> final {
    public:
        static bool Render(const std::string& inLabel, std::string& inValue);
    };

    template <>
    class InputWidget<Core::Uri> final {
    public:
        static bool Render(const std::string& inLabel, Core::Uri& inValue);
    };

    template <>
    class InputWidget<Common::FVec2> final {
    public:
        static bool Render(const std::string& inLabel, Common::FVec2& inValue);
    };

    template <>
    class InputWidget<Common::FVec3> final {
    public:
        static bool Render(const std::string& inLabel, Common::FVec3& inValue);
    };

    template <>
    class InputWidget<Common::FVec4> final {
    public:
        static bool Render(const std::string& inLabel, Common::FVec4& inValue);
    };

    template <>
    class InputWidget<Common::FQuat> final {
    public:
        static bool Render(const std::string& inLabel, Common::FQuat& inValue);
    };

    template <>
    class InputWidget<Common::FTransform> final {
    public:
        static bool Render(const std::string& inLabel, Common::FTransform& inValue);
    };

    template <>
    class InputWidget<Common::LinearColor> final {
    public:
        static bool Render(const std::string& inLabel, Common::LinearColor& inValue);
    };

    template <>
    class InputWidget<Common::Color> final {
    public:
        static bool Render(const std::string& inLabel, Common::Color& inValue);
    };

    bool RenderInputWidget(const std::string& inLabel, Mirror::Any& inValue);
}
