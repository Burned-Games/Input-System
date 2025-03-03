#include "SceneTreePanel.h"

#include "CoffeeEngine/Core/Base.h"
#include "CoffeeEngine/Core/FileDialog.h"
#include "CoffeeEngine/IO/Resource.h"
#include "CoffeeEngine/Project/Project.h"
#include "CoffeeEngine/Renderer/Camera.h"
#include "CoffeeEngine/Renderer/Material.h"
#include "CoffeeEngine/Renderer/Texture.h"
#include "CoffeeEngine/Scene/Components.h"
#include "CoffeeEngine/Scene/Entity.h"
#include "CoffeeEngine/Scene/PrimitiveMesh.h"
#include "CoffeeEngine/Scene/Scene.h"
#include "CoffeeEngine/Scene/SceneCamera.h"
#include "CoffeeEngine/Scene/SceneTree.h"
#include "CoffeeEngine/Scripting/Lua/LuaScript.h"
#include "entt/entity/entity.hpp"
#include "entt/entity/fwd.hpp"
#include "imgui_internal.h"
#include <IconsLucide.h>

#include <CoffeeEngine/Scripting/Script.h>
#include <any>
#include <array>
#include <cstdint>
#include <cstring>
#include <glm/fwd.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <memory>
#include <string>

namespace Coffee {

    SceneTreePanel::SceneTreePanel(const Ref<Scene>& scene)
    {
        m_Context = scene;
    }

    void SceneTreePanel::SetContext(const Ref<Scene>& scene)
    {
        m_Context = scene;
    }

    void SceneTreePanel::OnImGuiRender()
    {
        if (!m_Visible) return;

        ImGui::Begin("Scene Tree");

        //delete node and all children if supr is pressed and the node is selected
        if(ImGui::IsKeyPressed(ImGuiKey_Delete) && m_SelectionContext)
        {
            m_Context->DestroyEntity(m_SelectionContext);
            m_SelectionContext = {};
        }

        //Button for adding entities to the scene tree
        if(ImGui::Button(ICON_LC_PLUS, {24,24}))
        {
            ImGui::OpenPopup("Add Entity...");
        }
        ShowCreateEntityMenu();
        ImGui::SameLine();

        static std::array<char, 256> searchBuffer;
        ImGui::InputTextWithHint("##searchbar", ICON_LC_SEARCH " Search by name:", searchBuffer.data(), searchBuffer.size());

        ImGui::BeginChild("entity tree", {0,0}, ImGuiChildFlags_Border);

        auto view = m_Context->m_Registry.view<entt::entity>();
        for(auto entityID: view)
        {
            Entity entity{ entityID, m_Context.get()};
            auto& hierarchyComponent = entity.GetComponent<HierarchyComponent>();

            if(hierarchyComponent.m_Parent == entt::null)
            {
                DrawEntityNode(entity);
            }
        }

        ImGui::EndChild();
        
        // Entity Tree Drag and Drop functionality
        if(ImGui::BeginDragDropTarget())
        {
            if(const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE"))
            {
                const Ref<Resource>& resource = *(Ref<Resource>*)payload->Data;
                switch(resource->GetType())
                {
                    case ResourceType::Model:
                    {
                        const Ref<Model>& model = std::static_pointer_cast<Model>(resource);
                        AddModelToTheSceneTree(m_Context.get(), model);
                        break;
                    }
                    default:
                        break;
                }
            }
            ImGui::EndDragDropTarget();
        }

        if(ImGui::IsWindowHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            m_SelectionContext = {};
        }

        ImGui::End();

        ImGui::Begin("Inspector");
        if(m_SelectionContext)
        {
            DrawComponents(m_SelectionContext);
        }

        ImGui::End();
    }

    void SceneTreePanel::DrawEntityNode(Entity entity)
    {
        auto& entityNameTag = entity.GetComponent<TagComponent>().Tag;

        auto& hierarchyComponent = entity.GetComponent<HierarchyComponent>();

        ImGuiTreeNodeFlags flags = ((m_SelectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0) |
                                   ((hierarchyComponent.m_First == entt::null) ? ImGuiTreeNodeFlags_Leaf : 0) |
                                   ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_SpanAvailWidth;

        bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, entityNameTag.c_str());

        if(ImGui::IsItemClicked())
        {
            m_SelectionContext = entity;
        }

        //Code of Double clicking the item for changing the name (WIP)

        ImVec2 itemSize = ImGui::GetItemRectSize();

        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            ImVec2 popupPos = ImGui::GetItemRectMin();
            float indent = ImGui::GetStyle().IndentSpacing;
            ImGui::SetNextWindowPos({popupPos.x + indent, popupPos.y});
            ImGui::OpenPopup("EntityPopup");
        }

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

        if (ImGui::BeginPopup("EntityPopup"/*, ImGuiWindowFlags_NoBackground*/))
        {
            auto buff = entity.GetComponent<TagComponent>().Tag.c_str();
            ImGui::SetNextItemWidth(itemSize.x - ImGui::GetStyle().IndentSpacing);
            ImGui::InputText("##entity-name", (char*)buff, 128);
            ImGui::EndPopup();
        }

        ImGui::PopStyleVar();

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
        {
            ImGui::SetDragDropPayload("ENTITY_NODE", &entity, sizeof(Entity)); // Use the entity ID or a pointer as payload
            ImGui::Text("%s", entityNameTag.c_str());
            ImGui::EndDragDropSource();
        }

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_NODE"))
            {
                // Assuming payload is an Entity, but you need to cast and check appropriately
                Entity payloadEntity = *(const Entity*)payload->Data;
                // Process the drop, e.g., reparenting the entity in the hierarchy
                // This is where you would update the ECS or scene graph
                HierarchyComponent::Reparent(m_Context->m_Registry, (entt::entity)payloadEntity, entity); //I think is not necessary do the casting, it does it automatically;
            }
            ImGui::EndDragDropTarget();
        }

        if(opened)
        {
            if(hierarchyComponent.m_First != entt::null)
            {
                // Recursively draw all children
                Entity childEntity{ hierarchyComponent.m_First, m_Context.get()};
                while((entt::entity)childEntity != entt::null)
                {
                    DrawEntityNode(childEntity);
                    auto& childHierarchyComponent = childEntity.GetComponent<HierarchyComponent>();
                    childEntity = Entity{ childHierarchyComponent.m_Next, m_Context.get() };
                }
            }
            ImGui::TreePop();
        }
    }

    void SceneTreePanel::DrawComponents(Entity entity)
    {
        if(entity.HasComponent<TagComponent>())
        {
            auto& entityNameTag = entity.GetComponent<TagComponent>().Tag;

            ImGui::Text(ICON_LC_TAG " Tag");
            ImGui::SameLine();

            char buffer[256];
            memset(buffer, 0, sizeof(buffer));
            strcpy(buffer, entityNameTag.c_str());

            if(ImGui::InputText("##", buffer, sizeof(buffer)))
            {
                entityNameTag = std::string(buffer);
            }

            ImGui::Separator();
        }

        if(entity.HasComponent<TransformComponent>())
        {
            auto& transformComponent = entity.GetComponent<TransformComponent>();

            if(ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Text("Position");
                ImGui::DragFloat3("##Position", glm::value_ptr(transformComponent.Position), 0.1f);

                ImGui::Text("Rotation");
                ImGui::DragFloat3("##Rotation", glm::value_ptr(transformComponent.Rotation),  0.1f);

                ImGui::Text("Scale");
                ImGui::DragFloat3("##Scale", glm::value_ptr(transformComponent.Scale),  0.1f);
            }
        }

        if(entity.HasComponent<CameraComponent>())
        {
            auto& cameraComponent = entity.GetComponent<CameraComponent>();
            SceneCamera& sceneCamera = cameraComponent.Camera;
            bool isCollapsingHeaderOpen = true;
            if(ImGui::CollapsingHeader("Camera", &isCollapsingHeaderOpen, ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Text("Projection Type");
                if(ImGui::BeginCombo("##Projection Type", sceneCamera.GetProjectionType() == Camera::ProjectionType::PERSPECTIVE ? "Perspective" : "Orthographic"))
                {
                    if(ImGui::Selectable("Perspective", sceneCamera.GetProjectionType() == Camera::ProjectionType::PERSPECTIVE))
                    {
                        sceneCamera.SetProjectionType(Camera::ProjectionType::PERSPECTIVE);
                    }
                    if(ImGui::Selectable("Orthographic", sceneCamera.GetProjectionType() == Camera::ProjectionType::ORTHOGRAPHIC))
                    {
                        sceneCamera.SetProjectionType(Camera::ProjectionType::ORTHOGRAPHIC);
                    }
                    ImGui::EndCombo();
                }

                if(sceneCamera.GetProjectionType() == Camera::ProjectionType::PERSPECTIVE)
                {
                    ImGui::Text("Field of View");
                    float fov = sceneCamera.GetFOV();
                    if (ImGui::DragFloat("##Field of View", &fov, 0.1f, 0.0f, 180.0f))
                    {
                        sceneCamera.SetFOV(fov);
                    }

                    ImGui::Text("Near Clip");
                    float nearClip = sceneCamera.GetNearClip();
                    if (ImGui::DragFloat("##Near Clip", &nearClip, 0.1f))
                    {
                        sceneCamera.SetNearClip(nearClip);
                    }

                    ImGui::Text("Far Clip");
                    float farClip = sceneCamera.GetFarClip();
                    if (ImGui::DragFloat("##Far Clip", &farClip, 0.1f))
                    {
                        sceneCamera.SetFarClip(farClip);
                    }
                }

                if(sceneCamera.GetProjectionType() == Camera::ProjectionType::ORTHOGRAPHIC)
                {
                    ImGui::Text("Orthographic Size");
                    float orthoSize = sceneCamera.GetFOV();
                    if (ImGui::DragFloat("##Orthographic Size", &orthoSize, 0.1f))
                    {
                        sceneCamera.SetFOV(orthoSize);
                    }

                    ImGui::Text("Near Clip");
                    float nearClip = sceneCamera.GetNearClip();
                    if (ImGui::DragFloat("##Near Clip", &nearClip, 0.1f))
                    {
                        sceneCamera.SetNearClip(nearClip);
                    }

                    ImGui::Text("Far Clip");
                    float farClip = sceneCamera.GetFarClip();
                    if (ImGui::DragFloat("##Far Clip", &farClip, 0.1f))
                    {
                        sceneCamera.SetFarClip(farClip);
                    }
                }

                if(!isCollapsingHeaderOpen)
                {
                    entity.RemoveComponent<CameraComponent>();
                }
            }
        }

        if(entity.HasComponent<LightComponent>())
        {
            auto& lightComponent = entity.GetComponent<LightComponent>();
            bool isCollapsingHeaderOpen = true;
            if(ImGui::CollapsingHeader("Light", &isCollapsingHeaderOpen, ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Text("Light Type");
                ImGui::Combo("##Light Type", (int*)&lightComponent.type, "Directional\0Point\0Spot\0");

                ImGui::Text("Color");
                ImGui::ColorEdit3("##Color", glm::value_ptr(lightComponent.Color));

                ImGui::Text("Intensity");
                ImGui::DragFloat("##Intensity", &lightComponent.Intensity, 0.1f);

                if(lightComponent.type == LightComponent::Type::PointLight || lightComponent.type == LightComponent::Type::SpotLight)
                {
                    ImGui::Text("Range");
                    ImGui::DragFloat("##Range", &lightComponent.Range, 0.1f);
                }

                if(lightComponent.type == LightComponent::Type::PointLight)
                {
                    ImGui::Text("Attenuation");
                    ImGui::DragFloat("##Attenuation", &lightComponent.Attenuation, 0.1f);
                }
                if(!isCollapsingHeaderOpen)
                {
                    entity.RemoveComponent<LightComponent>();
                }
            }
        }

        if(entity.HasComponent<MeshComponent>())
        {
            auto& meshComponent = entity.GetComponent<MeshComponent>();
            bool isCollapsingHeaderOpen = true;
            if(ImGui::CollapsingHeader("Mesh", &isCollapsingHeaderOpen, ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Text("Mesh");
                ImGui::SameLine();
                const std::string& meshName = meshComponent.GetMesh() ? meshComponent.GetMesh()->GetName() : "Missing Mesh!!";
                if(ImGui::Button(meshName.c_str(), {64, 32}))
                {
                    ImGui::OpenPopup("MeshPopup");
                }
                if(ImGui::BeginPopup("MeshPopup"))
                {
                    if(ImGui::MenuItem("Quad"))
                    {
                        meshComponent.mesh = PrimitiveMesh::CreateQuad();
                    }
                    if(ImGui::MenuItem("Cube"))
                    {
                        meshComponent.mesh = PrimitiveMesh::CreateCube();
                    }
                    if(ImGui::MenuItem("Sphere"))
                    {
                        meshComponent.mesh = PrimitiveMesh::CreateSphere();
                    }
                    if(ImGui::MenuItem("Plane"))
                    {
                        meshComponent.mesh = PrimitiveMesh::CreatePlane();
                    }
                    if(ImGui::MenuItem("Cylinder"))
                    {
                        meshComponent.mesh = PrimitiveMesh::CreateCylinder();
                    }
                    if(ImGui::MenuItem("Cone"))
                    {
                        meshComponent.mesh = PrimitiveMesh::CreateCone();
                    }
                    if(ImGui::MenuItem("Torus"))
                    {
                        meshComponent.mesh = PrimitiveMesh::CreateTorus();
                    }
                    if(ImGui::MenuItem("Capsule"))
                    {
                        meshComponent.mesh = PrimitiveMesh::CreateCapsule();
                    }
                    if(ImGui::MenuItem("Save Mesh"))
                    {
                        COFFEE_ERROR("Save Mesh not implemented yet!");
                    }
                    ImGui::EndPopup();
                }
                ImGui::Checkbox("Draw AABB", &meshComponent.drawAABB);

                if(!isCollapsingHeaderOpen)
                {
                    entity.RemoveComponent<MeshComponent>();
                }
            }
        }

        if(entity.HasComponent<MaterialComponent>())
        {
            // Move this function to another site
            auto DrawTextureWidget = [&](const std::string& label, Ref<Texture2D>& texture)
            {
                auto& materialComponent = entity.GetComponent<MaterialComponent>();
                uint32_t textureID = texture ? texture->GetID() : 0;
                ImGui::ImageButton(label.c_str(), (ImTextureID)textureID, {64, 64});

                auto textureImageFormat = [](ImageFormat format) -> std::string {
                    switch (format)
                    {
                        case ImageFormat::R8: return "R8";
                        case ImageFormat::RGB8: return "RGB8";
                        case ImageFormat::RGBA8: return "RGBA8";
                        case ImageFormat::SRGB8: return "SRGB8";
                        case ImageFormat::SRGBA8: return "SRGBA8";
                        case ImageFormat::RGBA32F: return "RGBA32F";
                        case ImageFormat::DEPTH24STENCIL8: return "DEPTH24STENCIL8";
                    }
                };

                if (ImGui::IsItemHovered() and texture)
                {
                    ImGui::SetTooltip("Name: %s\nSize: %d x %d\nPath: %s",
                      texture->GetName().c_str(),
                      texture->GetWidth(),
                      texture->GetHeight(),
                      textureImageFormat(texture->GetImageFormat()).c_str(),
                      texture->GetPath().c_str()
                      );
                }

                if(ImGui::BeginDragDropTarget())
                {
                    if(const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE"))
                    {
                        const Ref<Resource>& resource = *(Ref<Resource>*)payload->Data;
                        if(resource->GetType() == ResourceType::Texture2D)
                        {
                            const Ref<Texture2D>& t = std::static_pointer_cast<Texture2D>(resource);
                            texture = t;
                        }
                    }
                    ImGui::EndDragDropTarget();
                }
                
                ImGui::SameLine();
                if(ImGui::BeginCombo((label + "texture").c_str(), "", ImGuiComboFlags_NoPreview))
                {
                    if(ImGui::Selectable("Clear"))
                    {
                        texture = nullptr;
                    }
                    if(ImGui::Selectable("Open"))
                    {
                        std::string path = FileDialog::OpenFile({}).string();
                        if(!path.empty())
                        {
                            Ref<Texture2D> t = Texture2D::Load(path);
                            texture = t;
                        }
                    }
                    ImGui::EndCombo();
                }
            };
            auto DrawCustomColorEdit4 = [&](const std::string& label, glm::vec4& color, const glm::vec2& size = {100, 32})
            {
                //ImGui::ColorEdit4("##Albedo Color", glm::value_ptr(materialProperties.color), ImGuiColorEditFlags_NoInputs);
                if(ImGui::ColorButton(label.c_str(), ImVec4(color.r, color.g, color.b, color.a), NULL, {size.x, size.y}))
                {
                    ImGui::OpenPopup("AlbedoColorPopup");
                }
                if(ImGui::BeginPopup("AlbedoColorPopup"))
                {
                    ImGui::ColorPicker4((label + "Picker").c_str(), glm::value_ptr(color), ImGuiColorEditFlags_NoInputs);
                    ImGui::EndPopup();
                }
            };

            auto& materialComponent = entity.GetComponent<MaterialComponent>();
            bool isCollapsingHeaderOpen = true;
            if(ImGui::CollapsingHeader("Material", &isCollapsingHeaderOpen, ImGuiTreeNodeFlags_DefaultOpen))
            {
                MaterialTextures& materialTextures = materialComponent.material->GetMaterialTextures();
                MaterialProperties& materialProperties = materialComponent.material->GetMaterialProperties();

                if(ImGui::TreeNode("Albedo"))
                {
                    ImGui::BeginChild("##Albedo Child", {0, 0}, ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                    
                    ImGui::Text("Color");
                    DrawCustomColorEdit4("##Albedo Color", materialProperties.color);

                    ImGui::Text("Texture");
                    DrawTextureWidget("##Albedo", materialTextures.albedo);

                    ImGui::EndChild();
                    ImGui::TreePop();
                }
                if(ImGui::TreeNode("Metallic"))
                {
                    ImGui::BeginChild("##Metallic Child", {0, 0}, ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                    ImGui::Text("Metallic");
                    ImGui::SliderFloat("##Metallic Slider", &materialProperties.metallic, 0.0f, 1.0f);
                    ImGui::Text("Texture");
                    DrawTextureWidget("##Metallic", materialTextures.metallic);
                    ImGui::EndChild();
                    ImGui::TreePop();
                }
                if(ImGui::TreeNode("Roughness"))
                {
                    ImGui::BeginChild("##Roughness Child", {0, 0}, ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                    ImGui::Text("Roughness");
                    ImGui::SliderFloat("##Roughness Slider", &materialProperties.roughness, 0.1f, 1.0f);
                    ImGui::Text("Texture");
                    DrawTextureWidget("##Roughness", materialTextures.roughness);
                    ImGui::EndChild();
                    ImGui::TreePop();
                }
                if(ImGui::TreeNode("Emission"))
                {
                    ImGui::BeginChild("##Emission Child", {0, 0}, ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                    //FIXME: Emissive color variable is local and do not affect the materialProperties.emissive!!
                    glm::vec4& emissiveColor = reinterpret_cast<glm::vec4&>(materialProperties.emissive);
                    emissiveColor.a = 1.0f;
                    DrawCustomColorEdit4("Color", emissiveColor);
                    ImGui::Text("Texture");
                    DrawTextureWidget("##Emissive", materialTextures.emissive);
                    ImGui::EndChild();
                    ImGui::TreePop();
                }
                if(ImGui::TreeNode("Normal Map"))
                {
                    ImGui::BeginChild("##Normal Child", {0, 0}, ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                    ImGui::Text("Texture");
                    DrawTextureWidget("##Normal", materialTextures.normal);
                    ImGui::EndChild();
                    ImGui::TreePop();
                }
                if(ImGui::TreeNode("Ambient Occlusion"))
                {
                    ImGui::BeginChild("##AO Child", {0, 0}, ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                    ImGui::Text("AO");
                    ImGui::SliderFloat("##AO Slider", &materialProperties.ao, 0.0f, 1.0f);
                    ImGui::Text("Texture");
                    DrawTextureWidget("##AO", materialTextures.ao);
                    ImGui::EndChild();
                    ImGui::TreePop();
                }
            
                if(!isCollapsingHeaderOpen)
                {
                    entity.RemoveComponent<MaterialComponent>();
                }
            }
        }

        if (entity.HasComponent<AudioSourceComponent>())
        {
            auto& audioSourceComponent = entity.GetComponent<AudioSourceComponent>();
            bool isCollapsingHeaderOpen = true;
            if (ImGui::CollapsingHeader("Audio Source", &isCollapsingHeaderOpen, ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (!Audio::audioBanks.empty() && ImGui::BeginCombo("Audio Bank", audioSourceComponent.audioBankName.c_str()))
                {
                    for (auto& bank : Audio::audioBanks)
                    {
                        const bool isSelected = (audioSourceComponent.audioBankName == bank->name);

                        if (bank->name != "Init" && ImGui::Selectable(bank->name.c_str()))
                        {
                            if (audioSourceComponent.audioBank != bank)
                            {
                                audioSourceComponent.audioBank = bank;
                                audioSourceComponent.audioBankName = bank->name;

                                if (!audioSourceComponent.eventName.empty())
                                {
                                    audioSourceComponent.eventName.clear();
                                    Audio::StopEvent(audioSourceComponent);
                                }
                            }
                        }

                        if (isSelected)
                            ImGui::SetItemDefaultFocus();
                    }

                    ImGui::EndCombo();
                }

                if (audioSourceComponent.audioBank && ImGui::BeginCombo("Audio Clip", audioSourceComponent.eventName.c_str()))
                {
                    for (const auto& event : audioSourceComponent.audioBank->events)
                    {
                        const bool isSelected = audioSourceComponent.eventName == event;

                        if (ImGui::Selectable(event.c_str()))
                        {
                            if (!audioSourceComponent.eventName.empty())
                                Audio::StopEvent(audioSourceComponent);

                            audioSourceComponent.eventName = event;
                        }

                        if (isSelected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                ImGui::Checkbox("Play On Awake", &audioSourceComponent.playOnAwake);

                if (ImGui::Checkbox("Mute", &audioSourceComponent.mute))
                    Audio::SetVolume(audioSourceComponent.gameObjectID, audioSourceComponent.mute ? 0.f : audioSourceComponent.volume);

                if (ImGui::SliderFloat("Volume", &audioSourceComponent.volume, 0.f, 1.f))
                {
                    if (audioSourceComponent.mute)
                        audioSourceComponent.mute = false;

                    Audio::SetVolume(audioSourceComponent.gameObjectID, audioSourceComponent.volume);
                }

                if (audioSourceComponent.audioBank && !audioSourceComponent.eventName.empty())
                {
                    if (!audioSourceComponent.isPlaying)
                    {
                        if (ImGui::SmallButton("Play"))
                        {
                            Audio::PlayEvent(audioSourceComponent);
                        }
                    }
                    else
                    {
                        if (!audioSourceComponent.isPaused)
                        {
                            if (ImGui::SmallButton("Pause"))
                            {
                                Audio::PauseEvent(audioSourceComponent);
                            }
                        }
                        else if (ImGui::SmallButton("Resume"))
                        {
                            Audio::ResumeEvent(audioSourceComponent);
                        }

                        ImGui::SameLine();

                        if (ImGui::SmallButton("Stop"))
                        {
                            Audio::StopEvent(audioSourceComponent);
                        }
                    }
                }
            }

            if(!isCollapsingHeaderOpen)
            {
                AudioZone::UnregisterObject(audioSourceComponent.gameObjectID);
                Audio::UnregisterAudioSourceComponent(audioSourceComponent);
                entity.RemoveComponent<AudioSourceComponent>();
            }
        }

        if (entity.HasComponent<AudioListenerComponent>())
        {
            auto& audioListenerComponent = entity.GetComponent<AudioListenerComponent>();
            bool isCollapsingHeaderOpen = true;
            if (ImGui::CollapsingHeader("Audio Listener", &isCollapsingHeaderOpen, ImGuiTreeNodeFlags_DefaultOpen))
            {

            }

            if(!isCollapsingHeaderOpen)
            {
                Audio::UnregisterAudioListenerComponent(audioListenerComponent);
                entity.RemoveComponent<AudioListenerComponent>();
            }
        }

        if (entity.HasComponent<AudioZoneComponent>())
        {
            auto& audioZoneComponent = entity.GetComponent<AudioZoneComponent>();
            bool isCollapsingHeaderOpen = true;
            if (ImGui::CollapsingHeader("Audio Zone", &isCollapsingHeaderOpen, ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (ImGui::BeginCombo("Bus Channels", audioZoneComponent.audioBusName.c_str()))
                {
                    for (auto& busName : AudioZone::busNames)
                    {
                        const bool isSelected = (audioZoneComponent.audioBusName == busName);

                        if (ImGui::Selectable(busName.c_str()))
                        {
                            if (audioZoneComponent.audioBusName != busName)
                            {
                                audioZoneComponent.audioBusName = busName;
                                AudioZone::UpdateReverbZone(audioZoneComponent);
                            }
                        }

                        if (isSelected)
                            ImGui::SetItemDefaultFocus();
                    }

                    ImGui::EndCombo();
                }

                ImGui::Text("Position");
                if (ImGui::DragFloat3("##ZonePosition", glm::value_ptr(audioZoneComponent.position), 0.1f)==true)
                    AudioZone::UpdateReverbZone(audioZoneComponent);

                ImGui::Text("Radius");
                if (ImGui::SliderFloat("##ZoneRadius", &audioZoneComponent.radius, 1.f, 100.f))
                    AudioZone::UpdateReverbZone(audioZoneComponent);
            }

            if(!isCollapsingHeaderOpen)
            {
                AudioZone::RemoveReverbZone(audioZoneComponent);
                entity.RemoveComponent<AudioZoneComponent>();
            }
        }

        
        if (entity.HasComponent<ScriptComponent>())
        {
            auto& scriptComponent = entity.GetComponent<ScriptComponent>();
            bool isCollapsingHeaderOpen = true;
            if (ImGui::CollapsingHeader("Script", &isCollapsingHeaderOpen, ImGuiTreeNodeFlags_DefaultOpen))
            {
                //ImGui::Text("Script Path: ");
                //ImGui::Text(scriptComponent.script->GetPath().c_str());

                // Get the exposed variables
                auto& exposedVariables = scriptComponent.script->GetExportedVariables();

                // print the exposed variables
                for (auto& [name, variable] : exposedVariables)
                {
                    switch (variable.type)
                    {
                    case ExportedVariableType::Bool: {
                        bool value = variable.value.has_value() ? std::any_cast<bool>(variable.value) : false;
                        if (ImGui::Checkbox(name.c_str(), &value))
                        {
                            std::dynamic_pointer_cast<LuaScript>(scriptComponent.script)->SetVariable(name, value);
                            variable.value = value;
                        }
                        break;
                    }
                    case ExportedVariableType::Int: {
                        int value = variable.value.has_value() ? std::any_cast<int>(variable.value) : 0;
                        if (ImGui::InputInt(name.c_str(), &value))
                        {
                            std::dynamic_pointer_cast<LuaScript>(scriptComponent.script)->SetVariable(name, value);
                            variable.value = value;
                        }
                        break;
                    }
                    case ExportedVariableType::Float: {
                        float value = variable.value.has_value() ? std::any_cast<float>(variable.value) : 0.0f;
                        if (ImGui::InputFloat(name.c_str(), &value))
                        {
                            std::dynamic_pointer_cast<LuaScript>(scriptComponent.script)->SetVariable(name, value);
                            variable.value = value;
                        }
                        break;
                    }
                    case ExportedVariableType::String: {
                        std::string value =
                            variable.value.has_value() ? std::any_cast<std::string>(variable.value) : "";
                        char buffer[256];
                        memset(buffer, 0, sizeof(buffer));
                        strcpy(buffer, value.c_str());
                        if (ImGui::InputText(name.c_str(), buffer, sizeof(buffer)))
                        {
                            std::dynamic_pointer_cast<LuaScript>(scriptComponent.script)
                                ->SetVariable(name, std::string(buffer));
                            variable.value = std::string(buffer);
                        }
                        break;
                    }
                    case ExportedVariableType::Entity: {
                        Entity value = variable.value.has_value() ? std::any_cast<Entity>(variable.value) : Entity{};
                        if (ImGui::Button(name.c_str()))
                        {
                            ImGui::OpenPopup("EntityPopup");
                        }
                        if (ImGui::BeginPopup("EntityPopup"))
                        {
                            auto view = m_Context->m_Registry.view<TagComponent>();
                            for (auto entityID : view)
                            {
                                Entity e{entityID, m_Context.get()};
                                auto& tag = e.GetComponent<TagComponent>().Tag;
                                if (ImGui::Selectable(tag.c_str()))
                                {
                                    value = e;
                                    std::dynamic_pointer_cast<LuaScript>(scriptComponent.script)
                                        ->SetVariable(name, value);
                                    variable.value = value;
                                }
                            }
                            ImGui::EndPopup();
                        }
                        break;
                    }
                    case ExportedVariableType::Vector2: {
                        glm::vec2 value =
                            variable.value.has_value() ? std::any_cast<glm::vec2>(variable.value) : glm::vec2{};
                        if (ImGui::DragFloat2(name.c_str(), glm::value_ptr(value)))
                        {
                            std::dynamic_pointer_cast<LuaScript>(scriptComponent.script)->SetVariable(name, value);
                            variable.value = value;
                        }
                        break;
                    }
                    case ExportedVariableType::Vector3: {
                        glm::vec3 value =
                            variable.value.has_value() ? std::any_cast<glm::vec3>(variable.value) : glm::vec3{};
                        if (ImGui::DragFloat3(name.c_str(), glm::value_ptr(value)))
                        {
                            std::dynamic_pointer_cast<LuaScript>(scriptComponent.script)->SetVariable(name, value);
                            variable.value = value;
                        }
                        break;
                    }
                    case ExportedVariableType::Vector4: {
                        glm::vec4 value =
                            variable.value.has_value() ? std::any_cast<glm::vec4>(variable.value) : glm::vec4{};
                        if (ImGui::DragFloat4(name.c_str(), glm::value_ptr(value)))
                        {
                            std::dynamic_pointer_cast<LuaScript>(scriptComponent.script)->SetVariable(name, value);
                            variable.value = value;
                        }
                        break;
                    }
                    }
                }
            }
        }

        ImGui::Separator();

        ImGui::Dummy(ImVec2(0.0f, 10.0f));

        float buttonWidth = 200.0f;
        float buttonHeight = 32.0f;
        float availableWidth = ImGui::GetContentRegionAvail().x;
        float cursorPosX = (availableWidth - buttonWidth) * 0.5f;
        ImGui::SetCursorPosX(cursorPosX);

        if(ImGui::Button("Add Component", {buttonWidth, buttonHeight}))
        {
            ImGui::OpenPopup("Add Component...");
        }

        if(ImGui::BeginPopupModal("Add Component..."))
        {
            static char buffer[256] = "";
            ImGui::InputTextWithHint("##Search Component", "Search Component:",buffer, 256);

            std::string items[] = { "Tag Component", "Transform Component", "Mesh Component", "Material Component", "Light Component", "Camera Component", "Audio Source Component", "Audio Listener Component", "Audio Zone Component", "Lua Script Component" };
            static int item_current = 1;

            if (ImGui::BeginListBox("##listbox 2", ImVec2(-FLT_MIN, ImGui::GetContentRegionAvail().y - 200)))
            {
                for (int n = 0; n < IM_ARRAYSIZE(items); n++)
                {
                    const bool is_selected = (item_current == n);
                    if (ImGui::Selectable(items[n].c_str(), is_selected))
                        item_current = n;

                    // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndListBox();
            }


            ImGui::Text("Description");
            ImGui::TextWrapped("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Cras vel odio lectus. Integer scelerisque lacus a elit consequat, at imperdiet felis feugiat. Nunc rhoncus nisi lacinia elit ornare, eu semper risus consectetur.");

            if (ImGui::Button("Cancel"))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if(ImGui::Button("Add Component"))
            {
                if(items[item_current] == "Tag Component")
                {
                    if(!entity.HasComponent<TagComponent>())
                        entity.AddComponent<TagComponent>();
                    ImGui::CloseCurrentPopup();
                }
                else if(items[item_current] == "Transform Component")
                {
                    if(!entity.HasComponent<TransformComponent>())
                        entity.AddComponent<TransformComponent>();
                    ImGui::CloseCurrentPopup();
                }
                else if(items[item_current] == "Mesh Component")
                {
                    if(!entity.HasComponent<MeshComponent>())
                        entity.AddComponent<MeshComponent>();
                    ImGui::CloseCurrentPopup();
                }
                else if(items[item_current] == "Material Component")
                {
                    if(!entity.HasComponent<MaterialComponent>())
                        entity.AddComponent<MaterialComponent>();
                    ImGui::CloseCurrentPopup();
                }
                else if(items[item_current] == "Light Component")
                {
                    if(!entity.HasComponent<LightComponent>())
                        entity.AddComponent<LightComponent>();
                    ImGui::CloseCurrentPopup();
                }
                else if(items[item_current] == "Camera Component")
                {
                    if(!entity.HasComponent<CameraComponent>())
                        entity.AddComponent<CameraComponent>();
                    ImGui::CloseCurrentPopup();
                }
                else if(items[item_current] == "Audio Source Component")
                {
                    if(!entity.HasComponent<AudioSourceComponent>())
                    {
                        entity.AddComponent<AudioSourceComponent>();
                        Audio::RegisterAudioSourceComponent(entity.GetComponent<AudioSourceComponent>());
                        AudioZone::RegisterObject(entity.GetComponent<AudioSourceComponent>().gameObjectID, entity.GetComponent<AudioSourceComponent>().transform[3]);
                    }

                    ImGui::CloseCurrentPopup();
                }
                else if(items[item_current] == "Audio Listener Component")
                {
                    if(!entity.HasComponent<AudioListenerComponent>())
                    {
                        entity.AddComponent<AudioListenerComponent>();
                        Audio::RegisterAudioListenerComponent(entity.GetComponent<AudioListenerComponent>());
                    }

                    ImGui::CloseCurrentPopup();
                }
                else if(items[item_current] == "Audio Zone Component")
                {
                    if(!entity.HasComponent<AudioZoneComponent>())
                    {
                        entity.AddComponent<AudioZoneComponent>();
                        AudioZone::CreateZone(entity.GetComponent<AudioZoneComponent>());
                    }

                    ImGui::CloseCurrentPopup();
                }
                else if(items[item_current] == "Lua Script Component")
                {
                    if(!entity.HasComponent<ScriptComponent>())
                    {
                        // Pop up a file dialog to select the save location for the new script
                        FileDialogArgs args;
                        args.Filters = {{"Lua Script", "lua"}};
                        args.DefaultName = "NewScript.lua";
                        const std::filesystem::path& path = FileDialog::SaveFile(args);

                        if (!path.empty())
                        {
                            std::ofstream scriptFile(path);
                            if (scriptFile.is_open())
                            {
                                scriptFile << "function on_ready()\n";
                                scriptFile << "    -- Add initialization code here\n";
                                scriptFile << "end\n\n";
                                scriptFile << "function on_update(dt)\n";
                                scriptFile << "    -- Add update code here\n";
                                scriptFile << "end\n\n";
                                scriptFile << "function on_exit()\n";
                                scriptFile << "    -- Add cleanup code here\n";
                                scriptFile << "end\n";
                                scriptFile.close();

                                // Add the script component to the entity
                                entity.AddComponent<ScriptComponent>(path.string(), ScriptingLanguage::Lua);
                            }
                            else
                            {
                                COFFEE_CORE_ERROR("Failed to create Lua script file at: {0}", path.string());
                            }
                        }
                        else
                        {
                            COFFEE_CORE_WARN("Create Lua Script: No file selected");
                        }
                        ImGui::CloseCurrentPopup();
                    }
                }
                else
                {
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::EndPopup();
        }
    }


    // UI functions for scenetree menus
    void SceneTreePanel::ShowCreateEntityMenu()
    {
        if (ImGui::BeginPopupModal("Add Entity..."))
        {
            static char buffer[256] = "";
            ImGui::InputTextWithHint("##Search Component", "Search Component:", buffer, 256);

            std::string items[] = {"Empty", "Camera", "Primitive", "Light"};
            static int item_current = 1;

            if (ImGui::BeginListBox("##listbox 2", ImVec2(-FLT_MIN, ImGui::GetContentRegionAvail().y - 200)))
            {
                for (int n = 0; n < IM_ARRAYSIZE(items); n++)
                {
                    const bool is_selected = (item_current == n);
                    if (ImGui::Selectable(items[n].c_str(), is_selected))
                        item_current = n;

                    // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndListBox();
            }

            ImGui::Text("Description");
            ImGui::TextWrapped("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Cras vel odio lectus. Integer "
                            "scelerisque lacus a elit consequat, at imperdiet felis feugiat. Nunc rhoncus nisi "
                            "lacinia elit ornare, eu semper risus consectetur.");

            if (ImGui::Button("Cancel"))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Add Component"))
            {
                if (items[item_current] == "Empty")
                {
                    Entity e = m_Context->CreateEntity();
                    SetSelectedEntity(e);
                    ImGui::CloseCurrentPopup();
                }
                else if (items[item_current] == "Camera")
                {
                    Entity e = m_Context->CreateEntity("Camera");
                    e.AddComponent<CameraComponent>();
                    SetSelectedEntity(e);
                    ImGui::CloseCurrentPopup();
                }
                else if (items[item_current] == "Primitive")
                {
                    Entity e = m_Context->CreateEntity("Primitive");
                    e.AddComponent<MeshComponent>();
                    e.AddComponent<MaterialComponent>();
                    SetSelectedEntity(e);
                    ImGui::CloseCurrentPopup();
                }
                else if (items[item_current] == "Light")
                {
                    Entity e = m_Context->CreateEntity("Light");
                    e.AddComponent<LightComponent>();
                    SetSelectedEntity(e);
                    ImGui::CloseCurrentPopup();
                }
                else
                {
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::EndPopup();
        }
    }
}