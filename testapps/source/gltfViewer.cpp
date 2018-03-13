#pragma once

#include "gltfViewer.hpp"

// application space itself
namespace SatelliteExample {

    void GltfViewer::parseArguments(const int32_t& argc, const char ** argv) {
        args::ArgumentParser parser("This is a test rendering program.", "GeForce GTX 560 still sucks in 2018 year...");
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

    void GltfViewer::init(DeviceQueueType& device, const int32_t& argc, const char ** argv) {
        
        rays = std::shared_ptr<rt::Pipeline>(new rt::Pipeline(device, shaderPack));

        // init material system
        materialManager = std::shared_ptr<rt::MaterialSet>(new rt::MaterialSet(device));
        textureManager = std::shared_ptr<rt::TextureSet>(new rt::TextureSet(device));
        samplerManager = std::shared_ptr<rt::SamplerSet>(new rt::SamplerSet(device));

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
            int32_t rtTexture = textureManager->loadTexture(uri);
            // todo with rtTexture processing
            rtTextures.push_back(rtTexture);
        }

        // load materials (include PBR)
        for (int i = 0; i < gltfModel.materials.size(); i++) {
            tinygltf::Material & material = gltfModel.materials[i];
            rt::VirtualMaterial submat;

            // diffuse? 

            int32_t texId = getTextureIndex(material.values["baseColorTexture"].json_double_value);
            submat.diffuseTexture = texId >= 0 ? materialManager->addVTexture(glm::uvec2(rtTextures[texId], 0)) : 0;

            if (material.values["baseColorFactor"].number_array.size() >= 3) {
                submat.diffuse = glm::vec4(glm::make_vec3(&material.values["baseColorFactor"].number_array[0]), 1.0f);
            }
            else {
                submat.diffuse = glm::vec4(1.0f);
            }

            // metallic roughness
            texId = getTextureIndex(material.values["metallicRoughnessTexture"].json_double_value);
            submat.specularTexture = texId >= 0 ? materialManager->addVTexture(glm::uvec2(rtTextures[texId], 0)) : 0;
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
            submat.emissiveTexture = texId >= 0 ? materialManager->addVTexture(glm::uvec2(rtTextures[texId], 0)) : 0;

            // normal map
            texId = getTextureIndex(material.additionalValues["normalTexture"].json_double_value);
            submat.bumpTexture = texId >= 0 ? materialManager->addVTexture(glm::uvec2(rtTextures[texId], 0)) : 0;

            // load material
            materialManager->addMaterial(submat);
        }



        // calculate minimal buffer space size
        size_t gsize = 0;
        for (int i = 0; i < gltfModel.buffers.size(); i++) { gsize += tiled(gltfModel.buffers[i].data.size(), 4) * 4; }

        // create binding buffers
        vtbSpace = std::shared_ptr<rt::BufferSpace>(new rt::BufferSpace(device, gsize)); // allocate buffer space (with some spacing)
        bfvi = std::shared_ptr<rt::BufferViewSet>(new rt::BufferViewSet(device));
        acs = std::shared_ptr<rt::DataAccessSet>(new rt::DataAccessSet(device));
        bnds = std::shared_ptr<rt::DataBindingSet>(new rt::DataBindingSet(device));

        // unify buffers
        for (int i = 0; i < gltfModel.buffers.size(); i++) {
            intptr_t offset = vtbSpace->copyHostBuffer(gltfModel.buffers[i].data.data(), tiled(gltfModel.buffers[i].data.size(), 4) * 4);
            vtbSpace->addRegionDesc(rt::BufferRegion{ uint32_t(offset), uint32_t(tiled(gltfModel.buffers[i].data.size(),4) * 4) });
        }

        // buffer views
        for (auto const &bv : gltfModel.bufferViews) {
            rt::VirtualBufferView bfv;
            bfv.byteOffset = bv.byteOffset;
            bfv.byteStride = bv.byteStride;
            bfv.bufferID = bv.buffer;
            bfvi->addElement(bfv);
        }

        // accessors 
        for (auto const &it : gltfModel.accessors) {
            rt::VirtualDataAccess dst;
            dst.bufferView = it.bufferView;
            dst.byteOffset = it.byteOffset;
            dst.components = it.type - 1;//_byType(it.type)-1;
            acs->addElement(dst);
        }

        // load mesh templates (better view objectivity)
        for (int m = 0; m < gltfModel.meshes.size(); m++) {
            std::vector<std::shared_ptr<rt::VertexInstance>> primitiveVec = std::vector<std::shared_ptr<rt::VertexInstance>>();

            // load instances
            tinygltf::Mesh &glMesh = gltfModel.meshes[m];
            for (int i = 0; i < glMesh.primitives.size(); i++) {
                tinygltf::Primitive & prim = glMesh.primitives[i];

                // create vertex instance
                std::shared_ptr<rt::VertexInstance> geom = std::shared_ptr<rt::VertexInstance>(new rt::VertexInstance(device));
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
                    rt::VirtualBufferBinding vattr;
                    vattr.dataAccess = it.second;

                    // vertex
                    if (it.first.compare("POSITION") == 0) { // vertices
                        geom->setVertexBinding(bnds->addElement(vattr));
                    } else

                    // normal
                    if (it.first.compare("NORMAL") == 0) {
                        geom->setNormalBinding(bnds->addElement(vattr));
                    } else

                    // texcoord
                    if (it.first.compare("TEXCOORD_0") == 0) {
                        geom->setTexcoordBinding(bnds->addElement(vattr));
                    }
                }

                // indices
                geom->useIndex16bit(false);
                bool isInt16 = false;
                if (prim.indices >= 0) {
                    tinygltf::Accessor &idcAccessor = gltfModel.accessors[prim.indices];
                    geom->setNodeCount(idcAccessor.count / 3);

                    // access binding  
                    rt::VirtualBufferBinding vattr;
                    vattr.dataAccess = prim.indices;
                    geom->setIndiceBinding(bnds->addElement(vattr));

                    // is 16-bit indices?
                    isInt16 = idcAccessor.componentType == TINYGLTF_COMPONENT_TYPE_SHORT || idcAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
                }

                // use constant material for instance 
                geom->setMaterialOffset(prim.material);
                geom->useIndex16bit(isInt16);

                // if triangles, then load mesh
                if (prim.mode == TINYGLTF_MODE_TRIANGLES) { primitiveVec.push_back(geom); }
            }

            meshVec.push_back(primitiveVec);
        }
#endif

        // create geometry intersector
        geometryCollector = std::shared_ptr<rt::GeometryAccumulator>(new rt::GeometryAccumulator(device, shaderPack));
        geometryCollector->allocatePrimitiveReserve(1024 * 2048);

        bvhStore = std::shared_ptr<rt::HieararchyStorage>(new rt::HieararchyStorage(device, shaderPack));
        bvhStore->allocatePrimitiveReserve(1024 * 2048);
        bvhStore->allocateNodeReserve(1024 * 2048);

        bvhBuilder = std::shared_ptr<rt::HieararchyBuilder>(new rt::HieararchyBuilder(device, shaderPack));
        bvhBuilder->allocateNodeReserve(1024 * 2048);
        bvhBuilder->setHieararchyOutput(bvhStore);
        bvhBuilder->setPrimitiveSource(geometryCollector);

        // init timing state
        time = glfwGetTime() * 1000.f, diff = 0;

        // matrix with scaling
        glm::dmat4 matrix(1.0);
        matrix *= glm::scale(glm::dvec3(mscale))*glm::scale(glm::dvec3(1.f, 1.f, 1.f)); // invert Z coordinate

        // loading/setup meshes 
        geometryCollector->resetAccumulationCounter();

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
                std::vector<std::shared_ptr<rt::VertexInstance>>& mesh = meshVec[node.mesh]; // load mesh object (it just vector of primitives)
                for (int p = 0; p < mesh.size(); p++) { // load every primitive
                    std::shared_ptr<rt::VertexInstance>& geom = mesh[p];
                    geom->setTransform(transform); // here is bottleneck with host-GPU exchange
                    geometryCollector->pushGeometry(geom);
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
                traverse(node, glm::dmat4(matrix), 16);
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

        glm::dmat4 perspView = glm::perspectiveFov(glm::radians(60.0), double(canvasWidth), double(canvasHeight), 0.0001, 10000.0); // ray tracing better pickup coordinates at LH for RH dimension
        glm::dmat4 modelView = glm::lookAt(glm::dvec3(cam->eye), glm::dvec3(cam->view), glm::dvec3(0.f, 1.f, 0.f)); // need rotated coordinate system in 180 degree (Vulkan API)

        // build BVH in device (with linked data)
        bvhBuilder->build(glm::dmat4(1.0));

        // bind resources
        rays->setMaterialSet(materialManager);
        rays->setTextureSet(textureManager);
        rays->setSamplerSet(samplerManager);
        rays->setHierarchyStorage(bvhStore);
        rays->setModelView(modelView);
        rays->setPerspective(perspView);

        // dispatch ray tracing pipeline
        rays->dispatchRayTracing();
    }
};



// main? 
//////////////////////

const int32_t baseWidth = 960, baseHeight = 540;
//const int32_t baseWidth = 640, baseHeight = 360;
int main(const int argc, const char ** argv)
{
    if (!glfwInit()) exit(EXIT_FAILURE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    // create application window (planned to merge)
    GLFWwindow* window = glfwCreateWindow(baseWidth, baseHeight, "Running...", NULL, NULL);
    if (!window) { glfwTerminate(); exit(EXIT_FAILURE); }

    // use ambient static event provider
    ambientIO::handleGlfw(window);

    // create application
    auto app = new SatelliteExample::GltfViewer(argc, argv, window);

    glfwTerminate();
    exit(EXIT_SUCCESS);
}
