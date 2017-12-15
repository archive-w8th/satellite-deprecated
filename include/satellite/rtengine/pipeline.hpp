#pragma once

#include "./structs.hpp"
#include "./triangleHierarchy.hpp"
#include "./materialSet.hpp"

namespace NSM {
    namespace rt {

        class Pipeline {
        protected:
            const size_t WORK_SIZE = 128;
            const size_t INTENSIVITY = 256;
            const size_t MAX_SURFACE_IMAGES = 72;
            const size_t BLOCK_COUNTER = 0;
            const size_t PREPARING_BLOCK_COUNTER = 1;
            const size_t CLEANING_COUNTER = 2;
            const size_t UNORDERED_COUNTER = 3;
            const size_t AVAILABLE_COUNTER = 4;
            const size_t HIT_COUNTER = 6;
            const size_t INTERSECTIONS_COUNTER = 7;

            // current compute device
            DeviceQueueType device;

            // compute pipelines
            ComputeContext rayGeneration, sampleCollection, surfaceShadingPpl, clearSamples, bvhTraverse, rayShadePipeline, binCollect;

            // swap PTR's (now no swapping)
            BufferType indicesSwap[2];
            BufferType availableSwap[2];

            // for staging 
            BufferType generalStagingBuffer;
            BufferType generalLoadingBuffer;

            // static buffers
            BufferType zerosBufferReference;
            BufferType debugOnes32BufferReference;

            // blocks
            BufferType rayNodeBuffer, rayBlockBuffer, blockBinBuffer, texelBuffer, hitBuffer, countersBuffer, unorderedTempBuffer; // 

            // active blocks 
            BufferType currentBlocks, preparingBlocks;

            // block where will rewrite
            BufferType clearingBlocks, availableBlocks;

            // traverse buffers
            BufferType traverseBlockData, traverseCacheData, leafTraverseBuffer, prepareTraverseBuffer;

            // output images
            TextureType accumulationImage, filteredImage, flagsImage, depthImage;


            // uniforms
            std::vector<LightUniformStruct> lightUniformData;
            UniformBuffer lightUniform;

            std::vector<RayBlockUniform> rayBlockData;
            UniformBuffer rayBlockUniform;

            std::vector<RayStream> rayStreamsData;
            UniformBuffer rayStreamsUniform;

            vk::DescriptorPool descriptorPool;

            std::vector<vk::DescriptorSet> 
                rayShadingDescriptors, rayTracingDescriptors, rayTraverseDescriptors, samplingDescriptors, surfaceDescriptors;

            std::vector<vk::DescriptorSetLayout> 
                rayShadingDescriptorsLayout, rayTracingDescriptorsLayout, rayTraverseDescriptorsLayout, samplingDescriptorsLayout, surfaceDescriptorsLayout;

            vk::PipelineLayout 
                rayShadingPipelineLayout, rayTracingPipelineLayout, rayTraversePipelineLayout, samplingPipelineLayout, surfacePipelineLayout;

            double starttime = 0.f;
            uint32_t canvasWidth = 1, canvasHeight = 1; size_t sequenceId = 0;

            std::string shadersPathPrefix = "shaders-spv";
            bool hitCountGot = false, doCleanSamples = false;

        protected:
            void syncUniforms();
            void initDescriptorSets();
            void initPipelines();
            void initLights();
            void reloadQueuedRays();
            void initBuffers();
            void init(DeviceQueueType& device);

        public:
            void clearRays();
            void resizeCanvas(uint32_t width, uint32_t height);
            void setSkybox(TextureType& skybox);
            void reallocRays(uint32_t width, uint32_t height);
            void clearSampling();
            void collectSamples();
            void rayShading();
            void generate(const glm::mat4 &persp, const glm::mat4 &frontSide);
            void surfaceShading(std::shared_ptr<MaterialSet>& materialSet);
            void setTextureSet(std::shared_ptr<TextureSet>& textureSet);
            void surfaceShading(std::shared_ptr<MaterialSet>& materialSet, std::shared_ptr<TextureSet>& textureSet);
            void traverse(std::shared_ptr<TriangleHierarchy>& hierarchy);

            uint32_t getCanvasWidth();
            uint32_t getCanvasHeight();

            TextureType& getRawImage();
            TextureType& getFilteredImage();

            // panorama mode
            void enable360mode(bool mode);
            size_t getRayCount();

        public:
            Pipeline() {}
            Pipeline(DeviceQueueType& device, std::string shadersPack) {
                shadersPathPrefix = shadersPack;
                init(device);
            }
        };

    }
}