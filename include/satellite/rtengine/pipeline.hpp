#pragma once

#include "./structs.hpp"
#include "./materialSet.hpp"
#include "./samplerSet.hpp"
#include "./textureSet.hpp"
#include "./vertexSubnodes.hpp"
#include "./triangleHierarchy.hpp"

namespace NSM
{
    namespace rt
    {

        class Pipeline
        {
        protected:
            const size_t INTENSIVITY = 4096;
            const size_t MAX_SURFACE_IMAGES = 72;
            const size_t BLOCK_COUNTER = 0;
            const size_t PREPARING_BLOCK_COUNTER = 1;
            const size_t CLEANING_COUNTER = 2;
            const size_t UNORDERED_COUNTER = 3;
            const size_t AVAILABLE_COUNTER = 4;
            const size_t BLOCK_INDEX_COUNTER = 5;
            const size_t HIT_COUNTER = 6;
            const size_t HIT_PAYLOAD_COUNTER = 7;

            // current compute device
            Device device;
            Queue queue;

            // compute pipelines
            ComputeContext rayGeneration, sampleCollection, surfaceShadingPpl,
                clearSamples, vertexInterp, rayShadePipeline, binCollect,
                unorderedFormer;

            // swap PTR's (now no swapping)
            Buffer indicesSwap[2];
            Buffer availableSwap[2];

            // for staging
            Buffer generalStagingBuffer;
            Buffer generalLoadingBuffer;

            // static buffers
            Buffer zerosBufferReference;
            Buffer debugOnes32BufferReference;

            // blocks
            Buffer rayNodeBuffer, rayBlockBuffer, rayIndexSpaceBuffer, blockBinBuffer, texelBuffer,
                hitBuffer, hitPayloadBuffer, countersBuffer, unorderedTempBuffer; //

            // active blocks
            Buffer currentBlocks, preparingBlocks;

            // block where will rewrite
            Buffer clearingBlocks, availableBlocks;

            // traverse buffers
            /*Buffer traverseBlockData, traverseCacheData, prepareTraverseBuffer*/;

            // output images
            Image accumulationImage, filteredImage, flagsImage, depthImage,
                normalImage, albedoImage;

            // uniforms
            std::vector<LightUniformStruct> lightUniformData;
            UniformBuffer lightUniform;

            std::vector<RayBlockUniform> rayBlockData;
            UniformBuffer rayBlockUniform;

            std::vector<RayStream> rayStreamsData;
            UniformBuffer rayStreamsUniform;

            std::vector<uint32_t> shuffledSeqData;
            UniformBuffer shuffledSeqUniform;

            // vk::DescriptorPool descriptorPool;

            std::vector<vk::DescriptorSet> rayTracingDescriptors, samplingDescriptors, surfaceDescriptors;
            std::vector<vk::DescriptorSetLayout> 
                rayTracingDescriptorsLayout, 
                //rayTraverseDescriptorsLayout, 
                samplingDescriptorsLayout, 
                surfaceDescriptorsLayout;

            vk::PipelineLayout rayTracingPipelineLayout,
                rayTraversePipelineLayout, samplingPipelineLayout, surfacePipelineLayout;

            double starttime = 0.f;
            uint32_t canvasWidth = 1, canvasHeight = 1;
            size_t sequenceId = 0;

            std::string shadersPathPrefix = "shaders-spv";
            bool hitCountGot = false, doCleanSamples = false;

            std::shared_ptr<MaterialSet> boundMaterialSet;
            std::shared_ptr<VirtualTextureSet> boundVirtualTextureSet;

            std::shared_ptr<pcg64> rndEngine;

        protected:
            void syncUniforms();
            void initDescriptorSets();
            void initPipelines();
            void initLights();
            void reloadQueuedRays();
            void initBuffers();
            void init(Queue &device);


        protected:
            void clearRays();
            void generate();
            void collectSamples();
            void rayShading();
            void traverse();

            std::vector<std::shared_ptr<HieararchyStorage>> hstorages;

        public:
            void clearSampling();
            void dispatchRayTracing();

            void setSkybox(Image &skybox);
            void resizeCanvas(uint32_t width, uint32_t height);
            void reallocRays(uint32_t width, uint32_t height);
            
            void setPerspective(const glm::dmat4 &persp);
            void setModelView(const glm::dmat4 &mv);

            void setSamplerSet(std::shared_ptr<SamplerSet> &samplerSet);
            void setTextureSet(std::shared_ptr<TextureSet> &textureSet);
            void setMaterialSet(std::shared_ptr<MaterialSet> &materialSet);
            void setVirtualTextureSet(std::shared_ptr<VirtualTextureSet> &vTexSet);
            void setHierarchyStorage(std::shared_ptr<HieararchyStorage> &hierarchy);
            void setHierarchyStorages(const std::vector<std::shared_ptr<HieararchyStorage>> &hierarchies);
            //TraversibleData& getTraverseDescriptorSet() { return tbsData; };

            uint32_t getCanvasWidth();
            uint32_t getCanvasHeight();

            Image &getRawImage();
            Image &getFilteredImage();
            Image &getAlbedoImage();
            Image &getNormalImage();

            // panorama mode
            void enable360mode(bool mode);
            size_t getRayCount();

        public:
            Pipeline() {}
            Pipeline(Queue &queue, std::string shadersPack)
            {
                shadersPathPrefix = shadersPack;
                init(queue);
            }
        };

    } // namespace rt
} // namespace NSM