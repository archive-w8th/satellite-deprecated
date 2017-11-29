#include "marble/application.hpp"
#include "marble/triangleHierarchy.hpp"
#include "marble/pipeline.hpp"


#include "tiny_obj_loader.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

namespace NSM {

    class HelloTriangleApplication : public ApplicationBase {
    protected:

        // application window title
        const std::string title = "PUKAN_API";

        // uniform data
        struct UniformAppData {
            glm::mat4 projectionMatrix;
            glm::mat4 modelMatrix;
            glm::mat4 viewMatrix;
        } uboVS;

        // vertex buffer structure
        struct Vertex {
            float position[3];
            float color[3];
        };

        // usable contexts
        GraphicsContext context;

        // vertex and uniforms 
        VertexLayout vertexLayout;
        UniformBuffer uniformDataVS;

        // declared pipelines for application
        vk::Pipeline trianglePipeline;
        vk::Pipeline computePipeline;

        // default width and height of application
        const int WIDTH = 1280;
        const int HEIGHT = 720;


        std::shared_ptr<rt::TriangleHierarchy> trih;
        std::shared_ptr<rt::Pipeline> rtpl;
        std::shared_ptr<rt::MaterialSet> materialSet;
        std::shared_ptr<rt::TextureSet> textureSet;


        glm::mat4 perspective = glm::perspective(glm::radians(80.f), float(WIDTH) / float(HEIGHT), 0.0001f, 10000.f);
        //glm::mat4 lookAt = glm::lookAt(glm::vec3(0.f, 1.0f, 2.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, -1.f, 0.f));
        glm::mat4 lookAt = glm::lookAt(glm::vec3(-8.f, 5.0f, 0.0f), glm::vec3(10.f, 10.f, 0.f), glm::vec3(0.f, -1.f, 0.f));

        // inbound geometries
        std::vector<std::shared_ptr<rt::VertexInstance>> geometries;

        virtual void initRaytracer(DeviceQueueType& device) {
            materialSet = std::shared_ptr<rt::MaterialSet>(new rt::MaterialSet(device));
            textureSet = std::shared_ptr<rt::TextureSet>(new rt::TextureSet(device));

            trih = std::shared_ptr<rt::TriangleHierarchy>(new rt::TriangleHierarchy(device));
            trih->allocate(1024 * 1024 * 2);



            std::string inputfile = "Chess_Set.obj";
            tinyobj::attrib_t attrib;
            std::vector<tinyobj::shape_t> shapes;
            std::vector<tinyobj::material_t> materials;

            std::string err;
            bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, inputfile.c_str());

            if (!err.empty()) { // `err` may contain warning message.
                std::cerr << err << std::endl;
            }



            // load geometry to ray tracer
            {

                // temporal struct
                struct ObjVertex {
                    float position[3];
                    float texcoord[2];
                    float normal[3];
                };



                // regions for meshes
                std::vector<uint32_t> loadingOffsets;
                std::vector<uint32_t> materialIds;
                std::vector<uint32_t> elementCounts;

                // load obj data
                std::vector<uint32_t> indexBuffer;
                std::vector<ObjVertex> vertexBuffer;
                

                if (ret) {
                    size_t indiceCounter = 0;
                    size_t offsetCounter = 0;
                    for (size_t s = 0; s < shapes.size(); s++) {
                        size_t index_offset = 0;

                        // load meshes headers
                        elementCounts.push_back(shapes[s].mesh.num_face_vertices.size());
                        materialIds.push_back(shapes[s].mesh.material_ids[0]);
                        loadingOffsets.push_back(offsetCounter * 3);
                        offsetCounter += shapes[s].mesh.num_face_vertices.size();

                        // load vertices
                        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
                            int fv = shapes[s].mesh.num_face_vertices[f];
                            for (size_t v = 0; v < 3; v++) {
                                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                                ObjVertex vert;
                                vert.position[0] = attrib.vertices[3 * idx.vertex_index + 0];
                                vert.position[1] = attrib.vertices[3 * idx.vertex_index + 1];
                                vert.position[2] = attrib.vertices[3 * idx.vertex_index + 2];
                                vert.texcoord[0] = attrib.texcoords.size() > 0 ? attrib.texcoords[2 * idx.texcoord_index + 0] : 0;
                                vert.texcoord[1] = attrib.texcoords.size() > 0 ? attrib.texcoords[2 * idx.texcoord_index + 1] : 0;
                                vert.normal[0] = attrib.normals.size() > 0 ? attrib.normals[3 * idx.normal_index + 0] : 0;
                                vert.normal[1] = attrib.normals.size() > 0 ? attrib.normals[3 * idx.normal_index + 1] : 0;
                                vert.normal[2] = attrib.normals.size() > 0 ? attrib.normals[3 * idx.normal_index + 2] : 0;

                                vertexBuffer.push_back(vert);
                                indexBuffer.push_back(indiceCounter++);
                            }
                            index_offset += fv;
                        }

                    }
                }


                // vertex and index buffer
                VertexBuffer vertices;
                IndexBuffer indices;

                // 
                indices.count = static_cast<uint32_t>(indexBuffer.size());
                uint32_t indexBufferSize = indices.count * sizeof(uint32_t);
                uint32_t vertexBufferSize = static_cast<uint32_t>(vertexBuffer.size()) * sizeof(ObjVertex);

                // staging buffers
                struct {
                    BufferType vertices;
                    BufferType indices;
                } stagingBuffers;

                // staging vertex and indice buffer
                stagingBuffers.vertices = createBuffer(device, vertexBufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
                stagingBuffers.indices = createBuffer(device, indexBufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

                // fill staging buffers
                bufferSubData(stagingBuffers.vertices, vertexBuffer, 0);
                bufferSubData(stagingBuffers.indices, indexBuffer, 0);

                // create vertex and indice buffer
                vertices.binding = 0;
                vertices.buffer = createBuffer(device, vertexBufferSize, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer);
                indices.buffer = createBuffer(device, indexBufferSize, vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer);

                {
                    // copy staging buffers
                    vk::CommandBuffer copyCmd = getCommandBuffer(device, true);
                    memoryCopyCmd(copyCmd, stagingBuffers.vertices, vertices.buffer, { 0, 0, vertexBufferSize });
                    memoryCopyCmd(copyCmd, stagingBuffers.indices, indices.buffer, { 0, 0, indexBufferSize });

                    // execute command and asynchronous destroy staging buffers
                    flushCommandBuffer(device, copyCmd, [&]() {
                        destroyBuffer(stagingBuffers.vertices);
                        destroyBuffer(stagingBuffers.indices);
                    });
                }




                // create accessors
                auto acst = std::shared_ptr<rt::AccessorSet>(new rt::AccessorSet(device));
                rt::VirtualAccessor vac;
                vac.bufferView = 0;
                vac.components = 3;
                vac.offset4 = 0;
                acst->addElement(vac);
                //vit->setVertexAccessor(acst->addElement(vac));

                vac.bufferView = 0;
                vac.components = 2;
                vac.offset4 = 3;
                acst->addElement(vac);
                //vit->setTexcoordAccessor(acst->addElement(vac));

                vac.bufferView = 0;
                vac.components = 3;
                vac.offset4 = 5;
                acst->addElement(vac);
                //vit->setNormalAccessor(acst->addElement(vac));

                // create buffer views
                auto bvst = std::shared_ptr<rt::BufferViewSet>(new rt::BufferViewSet(device));
                rt::VirtualBufferView vbv;
                vbv.offset4 = 0;
                vbv.stride4 = 8;
                bvst->addElement(vbv);


                /*
                {
                    // create vertex instance
                    auto vit = std::shared_ptr<rt::VertexInstance>(new rt::VertexInstance(device));

                    // set other datas
                    vit->setVertexAccessor(0);
                    vit->setTexcoordAccessor(1);
                    vit->setNormalAccessor(2);
                    vit->setDataBuffer(vertices.buffer);
                    vit->setIndicesBuffer(indices.buffer);
                    vit->setAccessorSet(acst);
                    vit->setBufferViewSet(bvst);
                    vit->setIndexed(false);

                    // set region of
                    vit->setMaterialOffset(0);
                    vit->setNodeCount(indexBufferSize / 3);
                    vit->setLoadingOffset(0);

                    // load to BVH builder
                    trih->loadGeometry(vit);
                }
                */
                
                for (int i = 0; i < elementCounts.size();i++) {

                    // create vertex instance
                    auto vit = std::shared_ptr<rt::VertexInstance>(new rt::VertexInstance(device));

                    // set other datas
                    vit->setVertexAccessor(0);
                    vit->setTexcoordAccessor(1);
                    vit->setNormalAccessor(2);
                    vit->setDataBuffer(vertices.buffer);
                    vit->setIndicesBuffer(indices.buffer);
                    vit->setAccessorSet(acst);
                    vit->setBufferViewSet(bvst);
                    vit->setIndexed(true);

                    // set region of
                    vit->setMaterialOffset(materialIds[i]);
                    vit->setNodeCount(elementCounts[i]);
                    vit->setLoadingOffset(loadingOffsets[i]);

                    // load to BVH builder
                    trih->loadGeometry(vit);

                }
            }



            // load materials
            for (int i = 0; i < materials.size();i++) {
                rt::VirtualMaterial vmat;

                // null textures
                vmat.diffusePart = 0;
                vmat.specularPart = 0;
                vmat.bumpPart = 0;
                vmat.emissivePart = 0;

                // textured colors
                if (materials[i].diffuse_texname != "") vmat.diffusePart = textureSet->loadTexture(materials[i].diffuse_texname);
                if (materials[i].specular_texname != "") vmat.specularPart = textureSet->loadTexture(materials[i].specular_texname);
                if (materials[i].roughness_texname != "") vmat.specularPart = textureSet->loadTexture(materials[i].roughness_texname);
                if (materials[i].bump_texname != "") vmat.bumpPart = textureSet->loadTexture(materials[i].bump_texname);
                if (materials[i].normal_texname != "") vmat.bumpPart = textureSet->loadTexture(materials[i].normal_texname);
                if (materials[i].emissive_texname != "") vmat.emissivePart = textureSet->loadTexture(materials[i].emissive_texname);

                // static colors
                vmat.diffuse = glm::vec4(glm::make_vec3(materials[i].diffuse), 1.f);
                vmat.specular = glm::vec4(materials[i].metallic, materials[i].roughness, 0.f, 0.f);
                vmat.emissive = glm::vec4(glm::make_vec3(materials[i].emission), 1.f);

                // add material to list
                materialSet->addMaterial(vmat);
            }

            materialSet->loadToVGA();

            rtpl = std::shared_ptr<rt::Pipeline>(new rt::Pipeline(device));
            rtpl->resizeCanvas(WIDTH, HEIGHT);
            rtpl->reallocRays(WIDTH, HEIGHT);
        }


        virtual void initVulkan() override {
            glfwInit();
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

            // create vulkan instance
            auto instance = createInstance();

            // get physical devices
            auto physicalDevices = instance.enumeratePhysicalDevices();
            if (physicalDevices.size() < 0) {
                glfwTerminate();
                std::cerr << "Vulkan does not supported, or driver broken." << std::endl;
                exit(0);
            }

            // choice device
            auto& gpu = physicalDevices[0];

            // create surface
            applicationWindow = createWindowSurface(WIDTH, HEIGHT, title);

            // get surface format from physical device
            applicationWindow.surfaceFormat = getSurfaceFormat(applicationWindow.surface, gpu);

            // create basic Vulkan objects
            auto deviceQueue = createDevice(gpu); // create default graphical device
            auto renderpass = createRenderpass(deviceQueue, applicationWindow.surfaceFormat);
            auto swapchain = createSwapchain(deviceQueue, applicationWindow.surface, applicationWindow.surfaceFormat);
            auto framebuffers = createSwapchainFramebuffer(deviceQueue, swapchain, renderpass, applicationWindow.surfaceFormat);

            // init application ray tracer
            initRaytracer(deviceQueue);



            // create command buffers per framebuffers
            for (int i = 0; i < framebuffers.size(); i++) {
                framebuffers[i].commandBuffer = createCommandBuffer(deviceQueue);
                framebuffers[i].waitFence = createFence(deviceQueue);
            }

            // define descriptor pool sizes
            std::vector<vk::DescriptorPoolSize> descriptorPoolSizes = {
                vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1)
            };

            // descriptor set bindings
            std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings = {
                vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, nullptr)
            };

            // layouts of descriptor sets 
            std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = {
                deviceQueue->logical.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo().setPBindings(descriptorSetLayoutBindings.data()).setBindingCount(1))
            };

            // create descriptor sets and pool
            auto descriptorPool = deviceQueue->logical.createDescriptorPool(
                vk::DescriptorPoolCreateInfo()
                .setPPoolSizes(descriptorPoolSizes.data())
                .setPoolSizeCount(descriptorPoolSizes.size())
                .setMaxSets(1)
            );

            // descriptor sets (where will writing binding)
            auto descriptorSets = deviceQueue->logical.allocateDescriptorSets(
                vk::DescriptorSetAllocateInfo()
                .setDescriptorPool(descriptorPool)
                .setDescriptorSetCount(descriptorSetLayouts.size())
                .setPSetLayouts(descriptorSetLayouts.data())
            );




            VertexBuffer vertices;
            IndexBuffer indices;

            // vertex data
            std::vector<Vertex> vertexBuffer = {
                { { -1.0f, -1.0f, 0.0f },{ 0.0f, 0.0f, 0.0f } },
                { { -1.0f,  1.0f, 0.0f },{ 0.0f, 0.0f, 0.0f } },
                { {  1.0f, -1.0f, 0.0f },{ 0.0f, 0.0f, 0.0f } },
                { {  1.0f,  1.0f, 0.0f },{ 0.0f, 0.0f, 0.0f } }
            };
            uint32_t vertexBufferSize = static_cast<uint32_t>(vertexBuffer.size()) * sizeof(Vertex);

            // index data
            std::vector<uint32_t> indexBuffer = { 0, 1, 2, 2, 1, 3 };
            indices.count = static_cast<uint32_t>(indexBuffer.size());
            uint32_t indexBufferSize = indices.count * sizeof(uint32_t);



            // staging buffers
            struct {
                BufferType vertices;
                BufferType indices;
            } stagingBuffers;

            // staging vertex and indice buffer
            stagingBuffers.vertices = createBuffer(deviceQueue, vertexBufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
            stagingBuffers.indices = createBuffer(deviceQueue, indexBufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

            // fill staging buffers
            bufferSubData(stagingBuffers.vertices, vertexBuffer, 0);
            bufferSubData(stagingBuffers.indices, indexBuffer, 0);

            // create vertex and indice buffer
            vertices.binding = 0;
            vertices.buffer = createBuffer(deviceQueue, vertexBufferSize, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst);
            indices.buffer = createBuffer(deviceQueue, indexBufferSize, vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst);
            indices.count = 6;

            {
                // copy staging buffers
                vk::CommandBuffer copyCmd = getCommandBuffer(deviceQueue, true);
                memoryCopyCmd(copyCmd, stagingBuffers.vertices, vertices.buffer, { 0, 0, vertexBufferSize });
                memoryCopyCmd(copyCmd, stagingBuffers.indices, indices.buffer, { 0, 0, indexBufferSize });

                // execute command and asynchronous destroy staging buffers
                flushCommandBuffer(deviceQueue, copyCmd, [&]() {
                    destroyBuffer(stagingBuffers.vertices);
                    destroyBuffer(stagingBuffers.indices);
                });
            }

            // resize attributes and bindings
            vertexLayout.inputAttributes.resize(2);
            vertexLayout.inputBindings.resize(1);

            {
                // vertex buffer (binding 0)
                vertexLayout.inputBindings[0].binding = 0;
                vertexLayout.inputBindings[0].stride = sizeof(Vertex);
                vertexLayout.inputBindings[0].inputRate = vk::VertexInputRate::eVertex;
            }

            {
                // vertex attribute (location 0)
                vertexLayout.inputAttributes[0].binding = 0;
                vertexLayout.inputAttributes[0].location = 0;
                vertexLayout.inputAttributes[0].format = vk::Format::eR32G32B32Sfloat;
                vertexLayout.inputAttributes[0].offset = offsetof(Vertex, position);
            }

            {
                // color attribute (location 1)
                vertexLayout.inputAttributes[1].binding = 0;
                vertexLayout.inputAttributes[1].location = 1;
                vertexLayout.inputAttributes[1].format = vk::Format::eR32G32B32Sfloat;
                vertexLayout.inputAttributes[1].offset = offsetof(Vertex, color);
            }

            // test texture
            {
                // create sampler
                vk::SamplerCreateInfo samplerInfo;
                samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
                samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
                samplerInfo.minFilter = vk::Filter::eLinear;
                samplerInfo.magFilter = vk::Filter::eLinear;
                samplerInfo.compareEnable = false;
                auto sampler = deviceQueue->logical.createSampler(samplerInfo);
                auto image = rtpl->getRawImage();

                // desc texture texture
                vk::DescriptorImageInfo imageDesc;
                imageDesc.imageLayout = image->layout;
                imageDesc.imageView = image->view;
                imageDesc.sampler = sampler;

                // update descriptors
                deviceQueue->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                    vk::WriteDescriptorSet()
                        .setDstSet(descriptorSets[0])
                        .setDstBinding(0)
                        .setDstArrayElement(0)
                        .setDescriptorCount(1)
                        .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                        .setPImageInfo(&imageDesc),
                }, nullptr);
            }

            // surface sizing
            auto renderArea = vk::Rect2D(vk::Offset2D(), applicationWindow.surfaceSize);
            auto viewport = vk::Viewport(0.0f, 0.0f, applicationWindow.surfaceSize.width, applicationWindow.surfaceSize.height, 0, 1.0f);
            std::vector<vk::Viewport> viewports = { viewport };
            std::vector<vk::Rect2D> scissors = { renderArea };

            // pipeline layout and cache
            auto pipelineLayout = deviceQueue->logical.createPipelineLayout(vk::PipelineLayoutCreateInfo().setPSetLayouts(descriptorSetLayouts.data()).setSetLayoutCount(descriptorSetLayouts.size()));
            auto pipelineCache = deviceQueue->logical.createPipelineCache(vk::PipelineCacheCreateInfo());

            // create pipeline
            {
                // pipeline stages
                std::vector<vk::PipelineShaderStageCreateInfo> pipelineShaderStages = {
                    vk::PipelineShaderStageCreateInfo().setModule(loadAndCreateShaderModule(deviceQueue, "shaders-spv/raytracing/render.vert.spv")).setPName("main").setStage(vk::ShaderStageFlagBits::eVertex),
                    vk::PipelineShaderStageCreateInfo().setModule(loadAndCreateShaderModule(deviceQueue, "shaders-spv/raytracing/render.frag.spv")).setPName("main").setStage(vk::ShaderStageFlagBits::eFragment)
                };

                // vertex state
                auto pvi = vk::PipelineVertexInputStateCreateInfo()
                    .setPVertexBindingDescriptions(vertexLayout.inputBindings.data()).setVertexBindingDescriptionCount(vertexLayout.inputBindings.size())
                    .setPVertexAttributeDescriptions(vertexLayout.inputAttributes.data()).setVertexAttributeDescriptionCount(vertexLayout.inputAttributes.size());

                // tesselation state
                auto pt = vk::PipelineTessellationStateCreateInfo();

                // viewport and scissors state
                auto pv = vk::PipelineViewportStateCreateInfo()
                    .setPViewports(viewports.data()).setViewportCount(viewports.size())
                    .setPScissors(scissors.data()).setScissorCount(scissors.size());

                // input assembly
                auto pia = vk::PipelineInputAssemblyStateCreateInfo()
                    .setTopology(vk::PrimitiveTopology::eTriangleList);

                // rasterization state
                auto pr = vk::PipelineRasterizationStateCreateInfo()
                    .setDepthClampEnable(false)
                    .setRasterizerDiscardEnable(false)
                    .setPolygonMode(vk::PolygonMode::eFill)
                    .setCullMode(vk::CullModeFlagBits::eNone)
                    .setFrontFace(vk::FrontFace::eCounterClockwise)
                    .setDepthBiasEnable(false)
                    .setDepthBiasConstantFactor(0)
                    .setDepthBiasClamp(0)
                    .setDepthBiasSlopeFactor(0)
                    .setLineWidth(1.f);

                // multisampling
                auto pm = vk::PipelineMultisampleStateCreateInfo()
                    .setRasterizationSamples(vk::SampleCountFlagBits::e1);

                // depth stencil comparsion modes
                auto pds = vk::PipelineDepthStencilStateCreateInfo()
                    .setDepthTestEnable(true)
                    .setDepthWriteEnable(true)
                    .setDepthCompareOp(vk::CompareOp::eLessOrEqual)
                    .setDepthBoundsTestEnable(false)
                    .setStencilTestEnable(false);

                // blend modes per framebuffer targets
                std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments = {
                    vk::PipelineColorBlendAttachmentState()
                    .setBlendEnable(false)
                    .setSrcColorBlendFactor(vk::BlendFactor::eZero).setDstColorBlendFactor(vk::BlendFactor::eOne).setColorBlendOp(vk::BlendOp::eAdd)
                    .setSrcAlphaBlendFactor(vk::BlendFactor::eZero).setDstAlphaBlendFactor(vk::BlendFactor::eOne).setAlphaBlendOp(vk::BlendOp::eAdd)
                    .setColorWriteMask(vk::ColorComponentFlags(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA))
                };

                // blend state
                auto pbs = vk::PipelineColorBlendStateCreateInfo()
                    .setLogicOpEnable(false)
                    .setLogicOp(vk::LogicOp::eClear)
                    .setPAttachments(colorBlendAttachments.data())
                    .setAttachmentCount(colorBlendAttachments.size());

                // dynamic states
                std::vector<vk::DynamicState> dynamicStates = {
                    vk::DynamicState::eViewport,
                    vk::DynamicState::eScissor
                };

                // dynamic states
                auto pdy = vk::PipelineDynamicStateCreateInfo()
                    .setPDynamicStates(dynamicStates.data()).setDynamicStateCount(dynamicStates.size());

                // create graphics pipeline
                trianglePipeline = deviceQueue->logical.createGraphicsPipeline(pipelineCache,
                    vk::GraphicsPipelineCreateInfo()
                    .setPStages(pipelineShaderStages.data()).setStageCount(pipelineShaderStages.size())
                    //.setFlags(vk::PipelineCreateFlagBits::eDerivative)
                    .setPVertexInputState(&pvi)
                    .setPInputAssemblyState(&pia)
                    .setPViewportState(&pv)
                    .setPRasterizationState(&pr)
                    .setPDepthStencilState(&pds)
                    .setPColorBlendState(&pbs)
                    .setLayout(pipelineLayout)
                    .setRenderPass(renderpass)
                    .setBasePipelineIndex(0)
                    .setPMultisampleState(&pm)
                    .setPDynamicState(&pdy)
                    .setPTessellationState(&pt)
                );
            }

            // clear values
            std::vector<vk::ClearValue> clearValues = {
                vk::ClearColorValue(std::array<float,4>{0.2f, 0.2f, 0.2f, 1.0f}),
                vk::ClearDepthStencilValue(1.0f, 0)
            };

            // create command buffer per FBO
            for (int32_t i = 0; i < framebuffers.size(); ++i) {
                framebuffers[i].commandBuffer.begin(vk::CommandBufferBeginInfo());
                framebuffers[i].commandBuffer.beginRenderPass(
                    vk::RenderPassBeginInfo(renderpass, framebuffers[i].frameBuffer, renderArea, clearValues.size(), clearValues.data()),
                    vk::SubpassContents::eInline
                );
                framebuffers[i].commandBuffer.setViewport(0, viewports);
                framebuffers[i].commandBuffer.setScissor(0, scissors);
                framebuffers[i].commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, trianglePipeline);
                framebuffers[i].commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descriptorSets, nullptr);
                framebuffers[i].commandBuffer.bindVertexBuffers(vertices.binding, 1, &vertices.buffer->buffer, &vertices.voffset);
                framebuffers[i].commandBuffer.bindIndexBuffer(indices.buffer->buffer, 0, indices.indexType);
                framebuffers[i].commandBuffer.drawIndexed(indices.count, 1, 0, 0, 1);
                framebuffers[i].commandBuffer.endRenderPass();
                framebuffers[i].commandBuffer.end();
            }

            // create graphics context
            context.device = deviceQueue;
            context.swapchain = swapchain;
            context.framebuffers = framebuffers;
            context.pipeline = trianglePipeline;
            context.descriptorPool = descriptorPool;
            context.descriptorSets = descriptorSets;

            // context draw function
            context.cachedSemaphores.resize(1);
            context.draw = [&]() {
                auto& currentContext = context;
                //auto previousSemaphore = currentContext.device->currentSemaphore; // get compute waiting semaphore
                //currentContext.device->currentSemaphore = currentContext.device->logical.createSemaphore(vk::SemaphoreCreateInfo()); // create new semaphore that will wait (before compute)

                // acquire next image where will rendered (and get semaphore when will presented finally)
                uint32_t currentBuffer = 0;
                currentContext.device->logical.acquireNextImageKHR(currentContext.swapchain, std::numeric_limits<uint64_t>::max(), currentContext.device->presentCompleteSemaphore, nullptr, &currentBuffer);

                // wait when this image will previously rendered (i.e. when will signaled and rendered)
                currentContext.device->logical.waitForFences(1, &currentContext.framebuffers[currentBuffer].waitFence, true, std::numeric_limits<uint64_t>::max()); // wait when will ready rendering
                currentContext.device->logical.resetFences(1, &currentContext.framebuffers[currentBuffer].waitFence); // unsignal before next work

                // at now not possible to vaildly remove semaphore, becuase used by command buffers (Vulkan API have no mechanism for checking occupy of Semaphores, etc.)
                //if (currentContext.cachedSemaphores[0]) currentContext.device->logical.destroySemaphore(currentContext.cachedSemaphores[0]); // destroy not needed semaphore
                //currentContext.cachedSemaphores[0] = previousSemaphore; // add to destruction after fence

                // pipeline stage flags
                std::vector<vk::PipelineStageFlags> waitStages = { vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eAllCommands };

                // first, wait computing, second, wait presenting
                //std::vector<vk::Semaphore> waitSemaphores = { currentContext.device->presentCompleteSemaphore, previousSemaphore }; // await present and compute semaphore
                std::vector<vk::Semaphore> waitSemaphores = { currentContext.device->presentCompleteSemaphore, currentContext.device->currentSemaphore }; // await present and compute semaphore

                // signal semaphores
                std::vector<vk::Semaphore> signalSemaphores = { currentContext.device->renderCompleteSemaphore, currentContext.device->currentSemaphore }; // signal to both semaphores

                // submit rendering (and wait presentation in device)
                auto kernel = vk::SubmitInfo()
                    .setPWaitDstStageMask(waitStages.data())
                    .setPCommandBuffers(&currentContext.framebuffers[currentBuffer].commandBuffer).setCommandBufferCount(1)
                    .setPWaitSemaphores(waitSemaphores.data()).setWaitSemaphoreCount(waitSemaphores.size())
                    .setPSignalSemaphores(signalSemaphores.data()).setSignalSemaphoreCount(signalSemaphores.size());
                currentContext.device->queue.submit(1, &kernel, currentContext.framebuffers[currentBuffer].waitFence);

                // present for displaying of this image
                currentContext.device->queue.presentKHR(vk::PresentInfoKHR(
                    1, &currentContext.device->renderCompleteSemaphore,
                    1, &currentContext.swapchain,
                    &currentBuffer, nullptr
                ));
            };

            // use this context
            currentContext = context;
        }


        virtual void rayTrace() {
            // build BVH
            trih->markDirty();
            trih->buildBVH();

            // raytrace
            rtpl->generate(perspective, lookAt);
            for (int i = 0; i < 16; i++) {
                if (rtpl->getRayCount() < 32) break;
                rtpl->traverse(trih);
                rtpl->surfaceShading(materialSet, textureSet); // surface shading
                rtpl->rayShading();
            }
            rtpl->collectSamples();
        }


        virtual void mainLoop() override {
            uint32_t currentBuffer = 0;
            uint32_t imageIndex = 0;
            uint64_t frameCounter = 0;
            double frameTimer = 0.0;
            double fpsTimer = 0.0;
            double lastFPS = 0.0;



            // main loop
            while (!glfwWindowShouldClose(applicationWindow.window)) {
                glfwPollEvents();

                // get current time
                auto tStart = std::chrono::high_resolution_clock::now();

                // do ray tracing
                rayTrace();

                // display ray traced
                currentContext.draw();

                // calculate time of rendered
                frameCounter++;
                auto tEnd = std::chrono::high_resolution_clock::now();
                auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
                frameTimer = tDiff / 1000.0;

                // calculate FPS
                fpsTimer += tDiff;
                if (fpsTimer > 1000.0) {
                    //std::string windowTitle = title + " - " + std::to_string(frameCounter) + " fps";
                    std::string windowTitle = title + " - " + std::to_string(int(glm::round(frameTimer*1000))) + "ms of frame time";
                    glfwSetWindowTitle(applicationWindow.window, windowTitle.c_str());

                    lastFPS = roundf(1.0 / frameTimer);
                    fpsTimer = 0.0;
                    frameCounter = 0;
                }
            }

            // correctly terminate application
            currentContext.device->logical.waitIdle(); // wait presentations or other actions
            glfwTerminate();
        }

    };
};

int main() {
    NSM::HelloTriangleApplication app;

    try {
        app.run();
    }
    catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}