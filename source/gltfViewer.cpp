﻿#include "Windows.h"
#include <tiny_gltf.h>

#include "satellite/vkutils/vkUtils.hpp"
#include "satellite/ambientIO.hpp"
#include "application.hpp"


// load implementations
#define RT_ENGINE_IMPLEMENT 
#include "satellite/rtengine/rtengine.hpp"
#include "satellite/impl/radixSort.inl"


// application space itself
namespace SatelliteExample {

    int32_t getTextureIndex(std::map<std::string, double> &mapped) {
        return mapped.count("index") > 0 ? mapped["index"] : -1;
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
        std::shared_ptr<ste::rt::TriangleHierarchy> intersector;
        std::shared_ptr<CameraController> cam;
        std::shared_ptr<ste::rt::MaterialSet> materialManager;
        std::shared_ptr<ste::rt::TextureSet> textureManager;

        // experimental GUI
        bool show_test_window = true, show_another_window = false;
        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

        double mscale = 1.0f;
#ifdef EXPERIMENTAL_GLTF
        tinygltf::Model gltfModel;
        std::vector<std::vector<std::shared_ptr<ste::rt::VertexInstance>>> meshVec = std::vector<std::vector<std::shared_ptr<ste::rt::VertexInstance>>>();
        std::vector<BufferType> glBuffers = std::vector<BufferType>();
        std::vector<uint32_t> rtTextures = std::vector<uint32_t>();
        std::shared_ptr<rt::BufferSpace> vtbSpace;
#endif


    public:
        GltfViewer(const int32_t& argc, const char ** argv, GLFWwindow * wind) : PathTracerApplication(argc, argv, wind) { execute(argc, argv, wind); };
        GltfViewer() : PathTracerApplication() {};
        virtual void init(DeviceQueueType& device, const int32_t& argc, const char ** argv) override;
        virtual void process() override;
        virtual void parseArguments(const int32_t& argc, const char ** argv) override;
        virtual void handleGUI() override;
    };


    void GltfViewer::parseArguments(const int32_t& argc, const char ** argv) {
        args::ArgumentParser parser("This is a test rendering program.", "Frik edition, YOBA! XYU tebe, Tereshin, a ne Bazuki!");
        args::HelpFlag help(parser, "help", "Available flags", { 'h', "help" });
        args::ValueFlag<int> computeflag(parser, "compute-device-id", "Vulkan compute device (UNDER CONSIDERATION)", { 'c' });
        args::ValueFlag<int> deviceflag(parser, "graphics-device-id", "Vulkan graphics device to use (also should support compute)", { 'g' });
        args::ValueFlag<float> scaleflag(parser, "scale", "Scaling of model object", { 's' });
        args::ValueFlag<std::string> directoryflag(parser, "directory", "Directory of resources", { 'd' });
        args::ValueFlag<std::string> shaderflag(parser, "shaders", "Used SPIR-V shader pack", { 'p' });
        args::ValueFlag<std::string> bgflag(parser, "background", "Environment background", { 'b' });
        args::ValueFlag<std::string> modelflag(parser, "model", "Model to view (planned multiple models support)", { 'm' });

        try {
            parser.ParseCLI(argc, argv);
        }
        catch (args::Help)
        {
            std::cout << parser; glfwDestroyWindow(applicationWindow.window); glfwTerminate(); system("pause"); exit(0);
        }

        // read arguments
        if (deviceflag) gpuID = args::get(deviceflag);
        if (shaderflag) shaderPack = args::get(shaderflag);
        if (bgflag) bgTexName = args::get(bgflag);
        if (modelflag) model_input = args::get(modelflag);
        if (scaleflag) mscale = args::get(scaleflag);
        if (directoryflag) directory = args::get(directoryflag);
        if (help || argc <= 1) { std::cout << parser; glfwDestroyWindow(applicationWindow.window); glfwTerminate(); system("pause"); exit(0); } // required help or no arguments
        if (model_input == "") std::cerr << "No model found :(" << std::endl;
    }


    void GltfViewer::handleGUI() {
        
        static float f = 0.0f;
        ImGui::Text("Hello, world!");
        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
        ImGui::ColorEdit3("clear color", (float*)&clear_color);
        if (ImGui::Button("Test Window")) show_test_window ^= 1;
        if (ImGui::Button("Another Window")) show_another_window ^= 1;
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        
    }


    void GltfViewer::init(DeviceQueueType& device, const int32_t& argc, const char ** argv) {
        // init material system
        materialManager = std::shared_ptr<ste::rt::MaterialSet>(new ste::rt::MaterialSet(device));
        textureManager = std::shared_ptr<ste::rt::TextureSet>(new ste::rt::TextureSet(device));

        // load env map 
        auto cbmap = loadCubemap(bgTexName, device);
        if (cbmap && cbmap->initialized) rays->setSkybox(cbmap);

        // camera contoller
        cam = std::shared_ptr<CameraController>(new CameraController());
        cam->setRays(rays);

#ifdef EXPERIMENTAL_GLTF
        tinygltf::TinyGLTF loader;
        std::string err = "";
        loader.LoadASCIIFromFile(&gltfModel, &err, directory + "/" + model_input);

        // load textures (TODO - native samplers support in ray tracers)
        for (int i = 0; i < gltfModel.textures.size(); i++) {
            tinygltf::Texture& gltfTexture = gltfModel.textures[i];
            std::string uri = directory + "/" + gltfModel.images[gltfTexture.source].uri;
            uint32_t rtTexture = textureManager->loadTexture(uri);
            // todo with rtTexture processing
            rtTextures.push_back(rtTexture);
        }

        // load materials (include PBR)
        //materialManager->clearSubmats();
        for (int i = 0; i < gltfModel.materials.size(); i++) {
            tinygltf::Material & material = gltfModel.materials[i];
            ste::rt::VirtualMaterial submat;

            // diffuse?

            int32_t texId = getTextureIndex(material.values["baseColorTexture"].json_double_value);
            submat.diffusePart = texId >= 0 ? rtTextures[texId] : 0;

            if (material.values["baseColorFactor"].number_array.size() >= 3) {
                submat.diffuse = glm::vec4(glm::make_vec3(&material.values["baseColorFactor"].number_array[0]), 1.0f);
            }
            else {
                submat.diffuse = glm::vec4(1.0f);
            }

            // metallic roughness
            texId = getTextureIndex(material.values["metallicRoughnessTexture"].json_double_value);
            submat.specularPart = texId >= 0 ? rtTextures[texId] : 0;
            submat.specular = glm::vec4(1.0f);

            if (material.values["metallicFactor"].number_array.size() >= 1) {
                submat.specular.z = material.values["metallicFactor"].number_array[0];
            }

            if (material.values["roughnessFactor"].number_array.size() >= 1) {
                submat.specular.y = material.values["roughnessFactor"].number_array[0];
            }

            // emission
            if (material.additionalValues["emissiveFactor"].number_array.size() >= 3) {
                submat.emissive = glm::vec4(glm::make_vec3(&material.additionalValues["emissiveFactor"].number_array[0]), 1.0f);
            }
            else {
                submat.emissive = glm::vec4(0.0f);
            }

            // emissive texture
            texId = getTextureIndex(material.additionalValues["emissiveTexture"].json_double_value);
            submat.emissivePart = texId >= 0 ? rtTextures[texId] : 0;

            // normal map
            texId = getTextureIndex(material.additionalValues["normalTexture"].json_double_value);
            submat.bumpPart = texId >= 0 ? rtTextures[texId] : 0;

            // load material
            materialManager->addMaterial(submat);
        }

        // calculate minimal buffer space size
        size_t gsize = 0;
        for (int i = 0; i < gltfModel.buffers.size(); i++) { gsize += gltfModel.buffers[i].data.size(); }

        // allocate buffer space (with some spacing)
        vtbSpace = std::shared_ptr<ste::rt::BufferSpace>(new ste::rt::BufferSpace(device, gsize*1.25));

        // make raw mesh buffers
        //glBuffers.resize(gltfModel.buffers.size());
        for (int i = 0; i < gltfModel.buffers.size(); i++) {

            // staging vertex and indice buffer
            //auto staging = createBuffer(device, gltfModel.buffers[i].data.size(), vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
            //bufferSubData(staging, gltfModel.buffers[i].data, 0);

            // create vertex and indice buffer
            //glBuffers[i] = createBuffer(device, gltfModel.buffers[i].data.size(), vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer, VMA_MEMORY_USAGE_GPU_ONLY);
            //copyMemoryProxy<BufferType&, BufferType&, vk::BufferCopy>(device, staging, glBuffers[i], { 0, 0, gltfModel.buffers[i].data.size() }, true);

            // load to unified buffer space
            intptr_t offset = vtbSpace->copyHostBuffer(gltfModel.buffers[i].data, gltfModel.buffers[i].data.size());
            vtbSpace->addRegionDesc(rt::BufferRegion{ uint32_t(offset), uint32_t(gltfModel.buffers[i].data.size()) });
        }

        // make buffer views
        std::shared_ptr<ste::rt::BufferViewSet> bfvi = std::shared_ptr<ste::rt::BufferViewSet>(new ste::rt::BufferViewSet(device));
        std::shared_ptr<ste::rt::DataAccessSet> acs = std::shared_ptr<ste::rt::DataAccessSet>(new ste::rt::DataAccessSet(device));
        std::shared_ptr<ste::rt::DataBindingSet> bnds = std::shared_ptr<ste::rt::DataBindingSet>(new ste::rt::DataBindingSet(device));

        // buffer views
        for (auto const &bv : gltfModel.bufferViews) {
            ste::rt::VirtualBufferView bfv;
            bfv.byteOffset = bv.byteOffset;
            bfv.byteStride = bv.byteStride;
            bfv.bufferID = bv.buffer;
            bfvi->addElement(bfv);
        }

        // accessors 
        for (auto const &it : gltfModel.accessors) {
            ste::rt::VirtualDataAccess dst;
            dst.bufferView = it.bufferView;
            dst.byteOffset = it.byteOffset;
            dst.components = _byType(it.componentType)-1;
            acs->addElement(dst);
        }

        // load mesh templates (better view objectivity)
        for (int m = 0; m < gltfModel.meshes.size(); m++) {
            std::vector<std::shared_ptr<ste::rt::VertexInstance>> primitiveVec = std::vector<std::shared_ptr<ste::rt::VertexInstance>>();

            // load instances
            tinygltf::Mesh &glMesh = gltfModel.meshes[m];
            for (int i = 0; i < glMesh.primitives.size(); i++) {
                tinygltf::Primitive & prim = glMesh.primitives[i];
                std::shared_ptr<ste::rt::VertexInstance> geom = std::shared_ptr<ste::rt::VertexInstance>(new ste::rt::VertexInstance(device));

                // set accessing buffers 
                geom->setBufferSpace(vtbSpace);
                geom->setDataAccessSet(acs);
                geom->setBufferViewSet(bfvi);
                geom->setBindingSet(bnds);

                // make attributes
                std::map<std::string, int>::const_iterator it(prim.attributes.begin());
                std::map<std::string, int>::const_iterator itEnd(prim.attributes.end());

                // load modern mode
                for (auto const &it : prim.attributes) {
                    tinygltf::Accessor &accessor = gltfModel.accessors[it.second];
                    //auto& bufferView = gltfModel.bufferViews[accessor.bufferView];

                    // binding 
                    ste::rt::VirtualBufferBinding vattr;
                    vattr.dataAccess = it.second;

                    // vertex
                    if (it.first.compare("POSITION") == 0) { // vertices
                        geom->setVertexBinding(bnds->addElement(vattr));
                    }
                    else

                    // normal
                    if (it.first.compare("NORMAL") == 0) {
                        geom->setNormalBinding(bnds->addElement(vattr));
                    }
                    else

                    // texcoord
                    if (it.first.compare("TEXCOORD_0") == 0) {
                        geom->setTexcoordBinding(bnds->addElement(vattr));
                    }
                }

                // indices
                if (prim.indices >= 0) {
                    tinygltf::Accessor &idcAccessor = gltfModel.accessors[prim.indices];
                    //auto& bufferView = gltfModel.bufferViews[idcAccessor.bufferView];
                    geom->setNodeCount(idcAccessor.count / 3);

                    ste::rt::VirtualBufferBinding vattr;
                    vattr.dataAccess = prim.indices;
                    geom->setIndiceBinding(bnds->addElement(vattr));

                    // is 16-bit indices?
                    bool isInt16 = idcAccessor.componentType == TINYGLTF_COMPONENT_TYPE_SHORT || idcAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
                    geom->useIndex16bit(isInt16);
                }

                // use constant material for instance 
                geom->setMaterialOffset(prim.material);

                // if triangles, then load mesh
                if (prim.mode == TINYGLTF_MODE_TRIANGLES) { primitiveVec.push_back(geom); }
            }

            meshVec.push_back(primitiveVec);
        }
#endif

        // create geometry intersector
        intersector = std::shared_ptr<ste::rt::TriangleHierarchy>(new ste::rt::TriangleHierarchy(device, shaderPack));
        intersector->allocate(1024 * 2048);

        // init timing state
        time = glfwGetTime() * 1000.f, diff = 0;
        
        // matrix with scaling
        glm::dmat4 matrix(1.0);
        matrix *= glm::scale(glm::dvec3(mscale))*glm::scale(glm::dvec3(1.f, 1.f, 1.f));

        // loading/setup meshes 
        intersector->clearTribuffer();

#ifdef EXPERIMENTAL_GLTF
        // load meshes
        std::function<void(tinygltf::Node &, glm::dmat4, int)> traverse = [&](tinygltf::Node & node, glm::dmat4 inTransform, int recursive)->void {
            glm::dmat4 localTransform(1.0);
            localTransform *= (node.matrix.size() >= 16 ? glm::make_mat4(node.matrix.data()) : glm::dmat4(1.0));
            localTransform *= (node.translation.size() >= 3 ? glm::translate(glm::make_vec3(node.translation.data())) : glm::dmat4(1.0));
            localTransform *= (node.scale.size() >= 3 ? glm::scale(glm::make_vec3(node.scale.data())) : glm::dmat4(1.0));
            localTransform *= (node.rotation.size() >= 4 ? glm::mat4_cast(glm::make_quat(node.rotation.data())) : glm::dmat4(1.0));

            glm::dmat4 transform = inTransform * localTransform;
            if (node.mesh >= 0) {
                std::vector<std::shared_ptr<ste::rt::VertexInstance>>& mesh = meshVec[node.mesh]; // load mesh object (it just vector of primitives)
                for (int p = 0; p < mesh.size(); p++) { // load every primitive
                    std::shared_ptr<ste::rt::VertexInstance>& geom = mesh[p];
                    geom->setTransform(transform); // here is bottleneck with host-GPU exchange
                    intersector->loadGeometry(geom);
                }
            }
            else
                if (node.children.size() > 0) {
                    for (int n = 0; n < node.children.size(); n++) {
                        if (recursive >= 0) traverse(gltfModel.nodes[node.children[n]], transform, recursive - 1);
                    }
                }
        };

        // load scene
        uint32_t sceneID = 0;
        if (gltfModel.scenes.size() > 0) {
            for (int n = 0; n < gltfModel.scenes[sceneID].nodes.size(); n++) {
                tinygltf::Node & node = gltfModel.nodes[gltfModel.scenes[sceneID].nodes[n]];
                traverse(node, glm::dmat4(matrix), 8);
            }
        }
#endif
    }

    // processing
    void GltfViewer::process() {
        double t = glfwGetTime() * 1000.f;
        diff = t - time, time = t;

        // switch to 360 degree view
        cam->work(mousepos, diff, kmap);
        if (switch360key) {
            e360m = !e360m; rays->enable360mode(e360m);
            switch360key = false;
        }

        // load materials to GPU
        materialManager->loadToVGA();

        // build BVH in device
        intersector->markDirty();
        intersector->buildBVH();

        // process ray tracing
        glm::mat4 perspective = glm::perspective(glm::radians(60.f), float(canvasWidth) / float(canvasHeight), 0.0001f, 10000.f);
        glm::mat4 lookAt = glm::lookAt(glm::vec3(cam->eye), glm::vec3(cam->view), glm::vec3(0.f, 1.f, 0.f));
        rays->generate(perspective, lookAt);
        for (int32_t j = 0; j < depth; j++) {
            if (rays->getRayCount() <= 0) break;
            rays->traverse(intersector);
            rays->surfaceShading(materialManager, textureManager); // low level function for getting surface materials (may have few materials)
            rays->rayShading(); // low level function for change rays
        }
        rays->collectSamples();
    }

};


const int32_t baseWidth = 640;
const int32_t baseHeight = 360;

//const int32_t baseWidth = 400;
//const int32_t baseHeight = 300;

//const int32_t baseWidth = 960;
//const int32_t baseHeight = 540;



// main? 
//////////////////////

int main(const int argc, const char ** argv)
{
    if (!glfwInit()) exit(EXIT_FAILURE);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    // create application window (planned to merge)
    GLFWwindow* window = glfwCreateWindow(baseWidth, baseHeight, "Sample unbiased renderer", NULL, NULL);
    if (!window) { glfwTerminate(); exit(EXIT_FAILURE); }

    // use ambient static event provider
    ambientIO::handleGlfw(window);

    // create application
    auto app = new SatelliteExample::GltfViewer(argc, argv, window);

    glfwTerminate();
    exit(EXIT_SUCCESS);
}
