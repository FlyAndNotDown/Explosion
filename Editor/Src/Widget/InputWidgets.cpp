#include <algorithm>

#include <imgui.h>
#include <imgui_stdlib.h>

#include <Editor/Widget/InputWidgets.h>

namespace Editor::Internal {
    constexpr float inputLabelColumnWeight = 0.3f;
    constexpr float inputValueColumnWeight = 1.0f - inputLabelColumnWeight;

    template <typename F>
    static bool RenderLabeledInput(const std::string& inLabel, F&& inRenderInput)
    {
        InputWidgetRow row(inLabel);
        if (!row.IsVisible()) {
            return false;
        }
        ImGui::SetNextItemWidth(-1.0f);
        return inRenderInput();
    }

    template <typename T>
    static bool RenderScalarValue(const std::string& inLabel, ImGuiDataType inDataType, T& inValue, float inSpeed)
    {
        return RenderLabeledInput(inLabel, [&]() -> bool {
            return ImGui::DragScalar("##Value", inDataType, &inValue, inSpeed);
        });
    }

    template <typename T>
    static bool RenderUnsignedScalarValue(const std::string& inLabel, ImGuiDataType inDataType, T& inValue)
    {
        const T minValue = 0;
        return RenderLabeledInput(inLabel, [&]() -> bool {
            return ImGui::DragScalar("##Value", inDataType, &inValue, 1.0f, &minValue);
        });
    }
}

namespace Editor {
    InputWidgetRow::InputWidgetRow(const std::string& inLabel)
        : visible(false)
    {
        ImGui::PushID(inLabel.c_str());
        visible = ImGui::BeginTable("##Input", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoSavedSettings);
        if (!visible) {
            return;
        }

        ImGui::TableSetupColumn("##Label", ImGuiTableColumnFlags_WidthStretch, Internal::inputLabelColumnWeight);
        ImGui::TableSetupColumn("##Value", ImGuiTableColumnFlags_WidthStretch, Internal::inputValueColumnWeight);
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(inLabel.c_str());

        ImGui::TableSetColumnIndex(1);
    }

    InputWidgetRow::~InputWidgetRow()
    {
        if (visible) {
            ImGui::EndTable();
        }
        ImGui::PopID();
    }

    bool InputWidgetRow::IsVisible() const
    {
        return visible;
    }

    bool InputWidget<bool>::Render(const std::string& inLabel, bool& inValue)
    {
        return Internal::RenderLabeledInput(inLabel, [&]() -> bool {
            return ImGui::Checkbox("##Value", &inValue);
        });
    }

    bool InputWidget<int8_t>::Render(const std::string& inLabel, int8_t& inValue)
    {
        return Internal::RenderScalarValue(inLabel, ImGuiDataType_S8, inValue, 1.0f);
    }

    bool InputWidget<uint8_t>::Render(const std::string& inLabel, uint8_t& inValue)
    {
        return Internal::RenderUnsignedScalarValue(inLabel, ImGuiDataType_U8, inValue);
    }

    bool InputWidget<int16_t>::Render(const std::string& inLabel, int16_t& inValue)
    {
        return Internal::RenderScalarValue(inLabel, ImGuiDataType_S16, inValue, 1.0f);
    }

    bool InputWidget<uint16_t>::Render(const std::string& inLabel, uint16_t& inValue)
    {
        return Internal::RenderUnsignedScalarValue(inLabel, ImGuiDataType_U16, inValue);
    }

    bool InputWidget<int32_t>::Render(const std::string& inLabel, int32_t& inValue)
    {
        return Internal::RenderScalarValue(inLabel, ImGuiDataType_S32, inValue, 1.0f);
    }

    bool InputWidget<uint32_t>::Render(const std::string& inLabel, uint32_t& inValue)
    {
        return Internal::RenderUnsignedScalarValue(inLabel, ImGuiDataType_U32, inValue);
    }

    bool InputWidget<int64_t>::Render(const std::string& inLabel, int64_t& inValue)
    {
        return Internal::RenderScalarValue(inLabel, ImGuiDataType_S64, inValue, 1.0f);
    }

    bool InputWidget<uint64_t>::Render(const std::string& inLabel, uint64_t& inValue)
    {
        return Internal::RenderUnsignedScalarValue(inLabel, ImGuiDataType_U64, inValue);
    }

    bool InputWidget<float>::Render(const std::string& inLabel, float& inValue)
    {
        return Internal::RenderScalarValue(inLabel, ImGuiDataType_Float, inValue, 0.05f);
    }

    bool InputWidget<double>::Render(const std::string& inLabel, double& inValue)
    {
        return Internal::RenderScalarValue(inLabel, ImGuiDataType_Double, inValue, 0.05f);
    }

    bool InputWidget<std::string>::Render(const std::string& inLabel, std::string& inValue)
    {
        return Internal::RenderLabeledInput(inLabel, [&]() -> bool {
            return ImGui::InputText("##Value", &inValue);
        });
    }

    bool InputWidget<Core::Uri>::Render(const std::string& inLabel, Core::Uri& inValue)
    {
        std::string value = inValue.Str();
        if (!InputWidget<std::string>::Render(inLabel, value)) {
            return false;
        }
        inValue = value;
        return true;
    }

    bool InputWidget<Common::FVec2>::Render(const std::string& inLabel, Common::FVec2& inValue)
    {
        float value[2] = { inValue.x, inValue.y };
        if (!Internal::RenderLabeledInput(inLabel, [&]() -> bool {
                return ImGui::DragFloat2("##Value", value, 0.05f);
            })) {
            return false;
        }
        inValue = Common::FVec2(value[0], value[1]);
        return true;
    }

    bool InputWidget<Common::FVec3>::Render(const std::string& inLabel, Common::FVec3& inValue)
    {
        float value[3] = { inValue.x, inValue.y, inValue.z };
        if (!Internal::RenderLabeledInput(inLabel, [&]() -> bool {
                return ImGui::DragFloat3("##Value", value, 0.05f);
            })) {
            return false;
        }
        inValue = Common::FVec3(value[0], value[1], value[2]);
        return true;
    }

    bool InputWidget<Common::FVec4>::Render(const std::string& inLabel, Common::FVec4& inValue)
    {
        float value[4] = { inValue.x, inValue.y, inValue.z, inValue.w };
        if (!Internal::RenderLabeledInput(inLabel, [&]() -> bool {
                return ImGui::DragFloat4("##Value", value, 0.05f);
            })) {
            return false;
        }
        inValue = Common::FVec4(value[0], value[1], value[2], value[3]);
        return true;
    }

    bool InputWidget<Common::FQuat>::Render(const std::string& inLabel, Common::FQuat& inValue)
    {
        float value[4] = { inValue.w, inValue.x, inValue.y, inValue.z };
        if (!Internal::RenderLabeledInput(inLabel, [&]() -> bool {
                return ImGui::DragFloat4("##Value", value, 0.01f);
            })) {
            return false;
        }
        inValue = Common::FQuat(value[0], value[1], value[2], value[3]);
        return true;
    }

    bool InputWidget<Common::FTransform>::Render(const std::string& inLabel, Common::FTransform& inValue)
    {
        bool changed = false;
        if (ImGui::TreeNodeEx(inLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
            changed |= InputWidget<Common::FVec3>::Render("Scale", inValue.scale);
            changed |= InputWidget<Common::FQuat>::Render("Rotation", inValue.rotation);
            changed |= InputWidget<Common::FVec3>::Render("Translation", inValue.translation);
            ImGui::TreePop();
        }
        return changed;
    }

    bool InputWidget<Common::LinearColor>::Render(const std::string& inLabel, Common::LinearColor& inValue)
    {
        float value[4] = { inValue.r, inValue.g, inValue.b, inValue.a };
        if (!Internal::RenderLabeledInput(inLabel, [&]() -> bool {
                return ImGui::ColorEdit4("##Value", value);
            })) {
            return false;
        }
        inValue = Common::LinearColor(value[0], value[1], value[2], value[3]);
        return true;
    }

    bool InputWidget<Common::Color>::Render(const std::string& inLabel, Common::Color& inValue)
    {
        float value[4] = {
            static_cast<float>(inValue.r) / 255.0f,
            static_cast<float>(inValue.g) / 255.0f,
            static_cast<float>(inValue.b) / 255.0f,
            static_cast<float>(inValue.a) / 255.0f
        };
        if (!Internal::RenderLabeledInput(inLabel, [&]() -> bool {
                return ImGui::ColorEdit4("##Value", value);
            })) {
            return false;
        }
        inValue = Common::Color(
            static_cast<uint8_t>(std::clamp(value[0], 0.0f, 1.0f) * 255.0f),
            static_cast<uint8_t>(std::clamp(value[1], 0.0f, 1.0f) * 255.0f),
            static_cast<uint8_t>(std::clamp(value[2], 0.0f, 1.0f) * 255.0f),
            static_cast<uint8_t>(std::clamp(value[3], 0.0f, 1.0f) * 255.0f));
        return true;
    }

    bool RenderInputWidget(const std::string& inLabel, Mirror::Any& inValue)
    {
        if (auto* value = inValue.TryAs<bool>()) { return InputWidget<bool>::Render(inLabel, *value); }
        if (auto* value = inValue.TryAs<int8_t>()) { return InputWidget<int8_t>::Render(inLabel, *value); }
        if (auto* value = inValue.TryAs<uint8_t>()) { return InputWidget<uint8_t>::Render(inLabel, *value); }
        if (auto* value = inValue.TryAs<int16_t>()) { return InputWidget<int16_t>::Render(inLabel, *value); }
        if (auto* value = inValue.TryAs<uint16_t>()) { return InputWidget<uint16_t>::Render(inLabel, *value); }
        if (auto* value = inValue.TryAs<int32_t>()) { return InputWidget<int32_t>::Render(inLabel, *value); }
        if (auto* value = inValue.TryAs<uint32_t>()) { return InputWidget<uint32_t>::Render(inLabel, *value); }
        if (auto* value = inValue.TryAs<int64_t>()) { return InputWidget<int64_t>::Render(inLabel, *value); }
        if (auto* value = inValue.TryAs<uint64_t>()) { return InputWidget<uint64_t>::Render(inLabel, *value); }
        if (auto* value = inValue.TryAs<float>()) { return InputWidget<float>::Render(inLabel, *value); }
        if (auto* value = inValue.TryAs<double>()) { return InputWidget<double>::Render(inLabel, *value); }
        if (auto* value = inValue.TryAs<std::string>()) { return InputWidget<std::string>::Render(inLabel, *value); }
        if (auto* value = inValue.TryAs<Core::Uri>()) { return InputWidget<Core::Uri>::Render(inLabel, *value); }
        if (auto* value = inValue.TryAs<Common::FVec2>()) { return InputWidget<Common::FVec2>::Render(inLabel, *value); }
        if (auto* value = inValue.TryAs<Common::FVec3>()) { return InputWidget<Common::FVec3>::Render(inLabel, *value); }
        if (auto* value = inValue.TryAs<Common::FVec4>()) { return InputWidget<Common::FVec4>::Render(inLabel, *value); }
        if (auto* value = inValue.TryAs<Common::FQuat>()) { return InputWidget<Common::FQuat>::Render(inLabel, *value); }
        if (auto* value = inValue.TryAs<Common::FTransform>()) { return InputWidget<Common::FTransform>::Render(inLabel, *value); }
        if (auto* value = inValue.TryAs<Common::LinearColor>()) { return InputWidget<Common::LinearColor>::Render(inLabel, *value); }
        if (auto* value = inValue.TryAs<Common::Color>()) { return InputWidget<Common::Color>::Render(inLabel, *value); }

        ImGui::Text("%s: %s", inLabel.c_str(), inValue.ToString().c_str());
        return false;
    }
}
