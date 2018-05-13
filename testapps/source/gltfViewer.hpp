#pragma once

// framework for test apps
#include <framework/application.hpp>

// implement our engine in same as application
#define RT_ENGINE_IMPLEMENT 
#include <satellite/rtengine.hpp>

// load tinygltf in same implementation as application
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include <tiny_gltf.h>

// args library
#include <args.hxx>


// application space itself
namespace SatelliteExample {

    class CameraController {
    protected:
        bool monteCarlo = true;

    public:
        glm::vec3 dir = glm::normalize(glm::vec3(0.0, -1.0, 0.01));
        glm::vec3 eye = glm::vec3(0.0, 4.0, 0.0);
        glm::vec3 view = glm::vec3(0.0, 0.0, 0.0);

        glm::vec2 mposition;
        std::shared_ptr<rt::Pipeline> raysp;

        glm::mat4 project() {
            view = eye + dir;
            return glm::lookAt(eye, view, glm::vec3(0.0f, 1.0f, 0.0f));
        }

        void setRays(std::shared_ptr<rt::Pipeline>& r) {
            raysp = r;
        }

        void work(const glm::vec2 &position, const double &diff, ControlMap& map) {
            glm::mat4 viewm = project();
            glm::mat4 unviewm = glm::inverse(viewm);
            glm::vec3 ca = (viewm * glm::vec4(eye, 1.0f)).xyz();
            glm::vec3 vi = glm::normalize((glm::vec4(dir, 0.0) * unviewm).xyz());
            bool isFocus = true;

            if (map.mouseleft && isFocus)
            {
                glm::vec2 mpos = glm::vec2(position) - mposition;
                double diffX = mpos.x, diffY = mpos.y;
                if (glm::abs(diffX) > 0.0) this->rotateX(vi, diffX);
                if (glm::abs(diffY) > 0.0) this->rotateY(vi, diffY);
                if (monteCarlo) raysp->clearSampling();
            }
            mposition = glm::vec2(position);

            if (map.keys[ControlMap::kW] && isFocus)
            {
                this->forwardBackward(ca, -diff);
                if (monteCarlo) raysp->clearSampling();
            }

            if (map.keys[ControlMap::kS] && isFocus)
            {
                this->forwardBackward(ca, diff);
                if (monteCarlo) raysp->clearSampling();
            }

            if (map.keys[ControlMap::kA] && isFocus)
            {
                this->leftRight(ca, -diff);
                if (monteCarlo) raysp->clearSampling();
            }

            if (map.keys[ControlMap::kD] && isFocus)
            {
                this->leftRight(ca, diff);
                if (monteCarlo) raysp->clearSampling();
            }

            if ((map.keys[ControlMap::kE] || map.keys[ControlMap::kSpc]) && isFocus)
            {
                this->topBottom(ca, diff);
                if (monteCarlo) raysp->clearSampling();
            }

            if ((map.keys[ControlMap::kQ] || map.keys[ControlMap::kSft] || map.keys[ControlMap::kC]) && isFocus)
            {
                this->topBottom(ca, -diff);
                if (monteCarlo) raysp->clearSampling();
            }

            dir = glm::normalize((glm::vec4(vi, 0.0) * viewm).xyz());
            eye = (unviewm * glm::vec4(ca, 1.0f)).xyz();
            view = eye + dir;
        }

        void leftRight(glm::vec3 &ca, const float &diff) {
            ca.x += diff / 100.0f;
        }
        void topBottom(glm::vec3 &ca, const float &diff) {
            ca.y += diff / 100.0f;
        }
        void forwardBackward(glm::vec3 &ca, const float &diff) {
            ca.z += diff / 100.0f;
        }
        void rotateY(glm::vec3 &vi, const float &diff) {
            glm::mat4 rot = glm::rotate(diff / float(raysp->getCanvasHeight()) / 0.5f, glm::vec3(-1.0f, 0.0f, 0.0f));
            vi = (rot * glm::vec4(vi, 1.0f)).xyz();
        }
        void rotateX(glm::vec3 &vi, const float &diff) {
            glm::mat4 rot = glm::rotate(diff / float(raysp->getCanvasHeight()) / 0.5f, glm::vec3(0.0f, -1.0f, 0.0f));
            vi = (rot * glm::vec4(vi, 1.0f)).xyz();
        }
    };


    int32_t getTextureIndex(std::map<std::string, double> &mapped) {
        return int32_t(mapped.size() > 0 ? double(mapped["index"]) : -1.0);
    }

    uint32_t _byType(const int &type) {
        switch (type) {
        case TINYGLTF_TYPE_VEC4:
            return 4;
            break;

        case TINYGLTF_TYPE_VEC3:
            return 3;
            break;

        case TINYGLTF_TYPE_VEC2:
            return 2;
            break;

        case TINYGLTF_TYPE_SCALAR:
            return 1;
            break;
        }
        return 1;
    }




    class GltfViewer : public PathTracerApplication {
    protected:
        bool e360m = false;
        std::string model_input = "";
        std::string directory = ".";
        std::string bgTexName = "background.jpg";
        std::shared_ptr<CameraController> cam;


        std::shared_ptr<std::function<void(tinygltf::Node &, glm::dmat4, int)>> vertexLoader;

        std::shared_ptr<rt::Pipeline> rays;
        std::shared_ptr<rt::HieararchyStorage> bvhStore;
        std::shared_ptr<rt::HieararchyBuilder> bvhBuilder;
        std::shared_ptr<rt::GeometryAccumulator> geometryCollector;
        std::shared_ptr<rt::MaterialSet> materialManager;
        std::shared_ptr<rt::VirtualTextureSet> vTextureSet;
        std::shared_ptr<rt::TextureSet> textureManager;
        std::shared_ptr<rt::SamplerSet> samplerManager;

        double mscale = 1.0f;
#ifdef EXPERIMENTAL_GLTF
        tinygltf::Model gltfModel;
        std::vector<std::vector<std::shared_ptr<rt::VertexInstance>>> meshVec = std::vector<std::vector<std::shared_ptr<rt::VertexInstance>>>(0);
        std::vector<int32_t> rtTextures = std::vector<int32_t>(0);
        std::shared_ptr<rt::BufferSpace> vtbSpace;
        std::shared_ptr<rt::BufferViewSet> bfvi;
        std::shared_ptr<rt::DataAccessSet> acs;
        std::shared_ptr<rt::DataBindingSet> bnds;
        std::shared_ptr<rt::BufferRegionSet> bfst;
#endif

        virtual void resizeBuffers(const int32_t& width, const int32_t& height) override;
        virtual void resize(const int32_t& width, const int32_t& height) override;

    public:
        GltfViewer(const int32_t& argc, const char ** argv, GLFWwindow * wind) : PathTracerApplication(argc, argv, wind) { execute(argc, argv, wind); };
        GltfViewer() : PathTracerApplication() {};
        virtual void init(DeviceQueueType& device, const int32_t& argc, const char ** argv) override;
        virtual void process() override;
        virtual void parseArguments(const int32_t& argc, const char ** argv) override;
        virtual void handleGUI() override;

        virtual ImageType getOutputImage() override;
        virtual void saveHdr(std::string name) override;
    };



    // pre-implement

    ImageType GltfViewer::getOutputImage() { return rays->getRawImage(); }
    void GltfViewer::handleGUI() {}
    void GltfViewer::resizeBuffers(const int32_t& width, const int32_t& height) { rays->reallocRays(width, height); }
    void GltfViewer::resize(const int32_t& width, const int32_t& height) { rays->resizeCanvas(width, height); }

    void GltfViewer::saveHdr(std::string name) {
#ifdef USE_CIMG
        auto width = rays->getCanvasWidth(), height = rays->getCanvasHeight();
        std::vector<float> imageData(width * height * 4);

        {
            auto texture = rays->getRawImage();
            flushCommandBuffer(currentContext->device, createCopyCmd<ImageType&, BufferType&, vk::BufferImageCopy>(currentContext->device, texture, memoryBufferToHost, vk::BufferImageCopy()
                .setImageExtent({ width, height, 1 })
                .setImageOffset({ 0, int32_t(height), 0 }) // copy ready (rendered) image
                .setBufferOffset(0)
                .setBufferRowLength(width)
                .setBufferImageHeight(height)
                .setImageSubresource(texture->subresourceLayers)), false);
        }

        {
            getBufferSubData(memoryBufferToHost, (U_MEM_HANDLE)imageData.data(), sizeof(float) * width * height * 4, 0);
        }

        {
            cil::CImg<float> image(imageData.data(), 4, width, height, 1, true);
            image.permute_axes("yzcx");
            image.mirror("y");
            image.get_shared_channel(3).fill(1.f);
            image.save_exr_adv(name.c_str());
        }
#endif
    }
};
