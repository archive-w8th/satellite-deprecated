#pragma once

#include "./gltfViewer.hpp"

// application space itself
namespace SatelliteExample {

    int GltfViewer::parseArguments(const int32_t& argc, const char ** argv, GLFWwindow * window) {
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
            std::cout << parser; glfwDestroyWindow(window); glfwTerminate(); system("pause"); return 0;
        }

        // read arguments
        if (deviceflag) gpuID = args::get(deviceflag);
        if (shaderflag) shaderPack = args::get(shaderflag);
        if (bgflag) bgTexName = args::get(bgflag);
        if (modelflag) model_input = args::get(modelflag);
        if (scaleflag) mscale = args::get(scaleflag);
        if (directoryflag) directory = args::get(directoryflag);
        if (help || argc <= 1) { std::cout << parser; glfwDestroyWindow(appfw->window()); glfwTerminate(); system("pause"); exit(0); } // required help or no arguments
        if (model_input == "") std::cerr << "No model found :(" << std::endl;
        return 1;
    }

    void GltfViewer::init(Queue queue, const int32_t& argc, const char ** argv) {
        
        rays = std::make_shared<rt::Pipeline>(queue, shaderPack);

        // init material system
        materialManager = std::make_shared<rt::MaterialSet>(queue);
        textureManager = std::make_shared<rt::TextureSet>(queue);
        samplerManager = std::make_shared<rt::SamplerSet>(queue);
        vTextureSet = std::make_shared<rt::VirtualTextureSet>(queue);

        // load env map 
        auto cbmap = loadEnvmap(bgTexName, queue);
        if (cbmap && cbmap->initialized) {
            rays->setSkybox(cbmap);
        }

        // camera contoller
        cam = std::make_shared<CameraController>();
        cam->setRays(rays);

#ifdef EXPERIMENTAL_GLTF
        tinygltf::TinyGLTF loader;
        std::string err = "";
        loader.LoadASCIIFromFile(&gltfModel, &err, directory + "/" + model_input);

        // load textures (TODO - native samplers support in ray tracers)
        for (int i = 0; i < gltfModel.textures.size(); i++) {
            tinygltf::Texture& gltfTexture = gltfModel.textures[i];
            rtTextures.push_back(textureManager->loadTexture(&gltfModel.images[gltfTexture.source]));
        }

        // load materials (include PBR)
        for (int i = 0; i < gltfModel.materials.size(); i++) {
            tinygltf::Material & material = gltfModel.materials[i];
            rt::VirtualMaterial submat;

            // initial material props
            submat.diffuse = glm::vec4(1.0f);
            submat.emissive = glm::vec4(0.0f);
            submat.specular = glm::vec4(1.f, 1.f, 0.f, 1.f);

            // base color (diffuse)
            if (material.values["baseColorFactor"].number_array.size() >= 3) {
                submat.diffuse = glm::vec4(glm::make_vec3(&material.values["baseColorFactor"].number_array[0]), 1.0f);
            }
            
            // metallic
            //if (material.values["metallicFactor"].number_array.size() >= 1) {
                submat.specular.z = material.values["metallicFactor"].number_value;
            //}

            // rought
            //if (material.values["roughnessFactor"].number_array.size() >= 1) {
                submat.specular.y = material.values["roughnessFactor"].number_value;
            //}

            // emission
            if (material.additionalValues["emissiveFactor"].number_array.size() >= 3) {
                submat.emissive = glm::vec4(glm::make_vec3(&material.additionalValues["emissiveFactor"].number_array[0]), 1.0f);
            }


            // diffuse texture
            int32_t texId = getTextureIndex(material.values["baseColorTexture"].json_double_value);
            submat.diffuseTexture = texId >= 0 ? vTextureSet->addElement(rt::VirtualTexture{ uint32_t(rtTextures[texId]), 0u }) + 1 : 0;

            // metallic/roughness texture
            texId = getTextureIndex(material.values["metallicRoughnessTexture"].json_double_value);
            submat.specularTexture = texId >= 0 ? vTextureSet->addElement(rt::VirtualTexture{ uint32_t(rtTextures[texId]), 0u }) + 1 : 0;

            // emissive texture
            texId = getTextureIndex(material.additionalValues["emissiveTexture"].json_double_value);
            submat.emissiveTexture = texId >= 0 ? vTextureSet->addElement(rt::VirtualTexture{ uint32_t(rtTextures[texId]), 0u }) + 1 : 0;

            // normal map
            texId = getTextureIndex(material.additionalValues["normalTexture"].json_double_value);
            submat.bumpTexture = texId >= 0 ? vTextureSet->addElement(rt::VirtualTexture{ uint32_t(rtTextures[texId]), 0u }) + 1 : 0;


            // load material
            materialManager->addElement(submat);
        }



        // calculate minimal buffer space size
        size_t gsize = 0;
        for (int i = 0; i < gltfModel.buffers.size(); i++) { gsize += tiled(gltfModel.buffers[i].data.size(), 4) * 4; }

        // create binding buffers
        vtbSpace = std::make_shared<rt::BufferSpace>(queue, gsize); // allocate buffer space (with some spacing)
        bfvi = std::make_shared<rt::BufferViewSet>(queue);
        acs = std::make_shared<rt::DataAccessSet>(queue);
        bnds = std::make_shared<rt::DataBindingSet>(queue);
        bfst = std::make_shared<rt::BufferRegionSet>(queue);

        // unify buffers
        for (int i = 0; i < gltfModel.buffers.size(); i++) {
            intptr_t offset = vtbSpace->copyHostBuffer(gltfModel.buffers[i].data.data(), tiled(gltfModel.buffers[i].data.size(), 4) * 4);
            //vtbSpace->addRegionDesc(rt::BufferRegion{ uint32_t(offset), uint32_t(tiled(gltfModel.buffers[i].data.size(),4) * 4) });
            bfst->addElement(rt::VirtualBufferRegion{ uint32_t(offset), uint32_t(tiled(gltfModel.buffers[i].data.size(),4) * 4) });
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
            tinygltf::Mesh &glMesh = gltfModel.meshes[m];


            // create vertex instance
            std::shared_ptr<rt::MeshUniformSet> murst = std::make_shared<rt::MeshUniformSet>(queue, glMesh.primitives.size());
            std::shared_ptr<rt::VertexInstance> geom = std::make_shared<rt::VertexInstance>(queue);
            geom->setBufferSpace(vtbSpace);
            geom->setDataAccessSet(acs);
            geom->setBufferViewSet(bfvi);
            geom->setBindingSet(bnds);
            geom->setBufferRegionSet(bfst);
            geom->setUniformSet(murst);

            for (int i = 0; i < glMesh.primitives.size(); i++) {
                tinygltf::Primitive & prim = glMesh.primitives[i];

                rt::MeshUniformStruct geni;

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
                        //geom->setVertexBinding(bnds->addElement(vattr));
                        geni.vertexAccessor = bnds->addElement(vattr);
                    } else

                    // normal
                    if (it.first.compare("NORMAL") == 0) {
                        //geom->setNormalBinding(bnds->addElement(vattr));
                        geni.normalAccessor = bnds->addElement(vattr);
                    } else

                    // texcoord
                    if (it.first.compare("TEXCOORD_0") == 0) {
                        //geom->setTexcoordBinding(bnds->addElement(vattr));
                        geni.texcoordAccessor = bnds->addElement(vattr);
                    }
                }

                geni.int16bit = 0;

                // indices
                bool isInt16 = false;
                if (prim.indices >= 0) {
                    tinygltf::Accessor &idcAccessor = gltfModel.accessors[prim.indices];
                    //geom->setNodeCount(idcAccessor.count / 3);
                    geni.nodeCount = idcAccessor.count / 3;

                    // access binding  
                    rt::VirtualBufferBinding vattr;
                    vattr.dataAccess = prim.indices;
                    //geom->setIndiceBinding(bnds->addElement(vattr));
                    geni.indiceAccessor = bnds->addElement(vattr);

                    // is 16-bit indices?
                    isInt16 = idcAccessor.componentType == TINYGLTF_COMPONENT_TYPE_SHORT || idcAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
                }

                geni.materialID = prim.material;
                geni.int16bit = isInt16 ? 1 : 0;
                murst->addElement(geni);

                // if triangles, then load mesh
                if (prim.mode == TINYGLTF_MODE_TRIANGLES) { primitiveVec.push_back(geom); } // anyways will loaded same "geom" pointer i.e. VertexInstance, so you can oriented by only uniform pointer index
            }

            meshVec.push_back(primitiveVec);
        }
#endif

        // create geometry intersector
        geometryCollector = std::make_shared<rt::GeometryAccumulator>(queue, shaderPack);
        geometryCollector->allocatePrimitiveReserve(1024 * 2048);

        bvhStore = std::make_shared<rt::HieararchyStorage>(queue, shaderPack);
        bvhStore->allocatePrimitiveReserve(1024 * 2048);
        bvhStore->allocateNodeReserve(1024 * 2048);

        bvhBuilder = std::make_shared<rt::HieararchyBuilder>(queue, shaderPack);
        bvhBuilder->allocateNodeReserve(1024 * 2048);
        bvhBuilder->setHieararchyOutput(bvhStore);
        bvhBuilder->setPrimitiveSource(geometryCollector);

        // init timing state
        time = glfwGetTime() * 1000.f, diff = 0;


        // make loader dispatcher
#ifdef EXPERIMENTAL_GLTF
        vertexLoader = std::make_shared<std::function<void(tinygltf::Node &, glm::dmat4, int)>>([=](tinygltf::Node & node, glm::dmat4 inTransform, int recursive)->void {
            glm::dmat4 localTransform(1.0);
            localTransform *= (node.matrix.size() >= 16 ? glm::make_mat4(node.matrix.data()) : glm::dmat4(1.0));
            localTransform *= (node.translation.size() >= 3 ? glm::translate(glm::make_vec3(node.translation.data())) : glm::dmat4(1.0));
            localTransform *= (node.scale.size() >= 3 ? glm::scale(glm::make_vec3(node.scale.data())) : glm::dmat4(1.0));
            localTransform *= (node.rotation.size() >= 4 ? glm::mat4_cast(glm::make_quat(node.rotation.data())) : glm::dmat4(1.0));

            glm::dmat4 transform = inTransform * localTransform;
            if (node.mesh >= 0) {
                std::vector<std::shared_ptr<rt::VertexInstance>>& mesh = meshVec[node.mesh]; // load mesh object (it just vector of primitives)
                std::shared_ptr<rt::VertexInstance>& geom = mesh[0];
                for (int p = 0; p < mesh.size(); p++) {
                    geom->getUniformSet()->getStructure(p).setTransform(transform); // here is bottleneck with host-GPU exchange
                }
                geometryCollector->pushGeometryMulti(geom, true, mesh.size()); // multi-loader 
            } else 
            if (node.children.size() > 0) {
                for (int n = 0; n < node.children.size(); n++) {
                    if (recursive >= 0) (*vertexLoader)(gltfModel.nodes[node.children[n]], transform, recursive - 1);
                }
            }
        });
#endif

        // bind resources
        rays->setMaterialSet(materialManager);
        rays->setVirtualTextureSet(vTextureSet);
        rays->setTextureSet(textureManager);
        rays->setSamplerSet(samplerManager);
        rays->setHierarchyStorage(bvhStore);

        // preload geometry and transform (may used dynamicly)
#ifdef EXPERIMENTAL_GLTF
        // matrix with scaling
        glm::dmat4 matrix(1.0);
        matrix *= glm::scale(glm::dvec3(mscale, mscale, mscale));

        // reset scene data collector
        geometryCollector->resetAccumulationCounter();

        // load scene
        uint32_t sceneID = 0;
        if (gltfModel.scenes.size() > 0) {
            for (int n = 0; n < gltfModel.scenes[sceneID].nodes.size(); n++) {
                tinygltf::Node & node = gltfModel.nodes[gltfModel.scenes[sceneID].nodes[n]];
                (*vertexLoader)(node, glm::dmat4(matrix), 16);
            }
        }
#endif
        bvhBuilder->build(glm::dmat4(1.0)); // build static
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

        // make camera projections
        rays->setModelView(glm::lookAt(glm::dvec3(cam->eye), glm::dvec3(cam->view), glm::dvec3(0.f, 1.f, 0.f)));
        rays->setPerspective(glm::perspectiveFov(glm::radians(60.0), double(canvasWidth), double(canvasHeight), 0.0001, 10000.0));

        bvhBuilder->build(glm::dmat4(1.0)); // build BVH dynamicly in device (with linked data)
        rays->dispatchRayTracing(); // dispatch ray tracing pipeline
    }
};



// main? 
//////////////////////

//const int32_t baseWidth = 1280, baseHeight = 720;
//const int32_t baseWidth = 640, baseHeight = 360; // super sampled, DPI adapative
const int32_t baseWidth = 960, baseHeight = 560;

int main(const int argc, const char ** argv)
{
    if (!glfwInit()) exit(EXIT_FAILURE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    // create application window (planned to merge)
    GLFWwindow* window = glfwCreateWindow(baseWidth, baseHeight, "Running...", NULL, NULL);
    if (!window) { glfwTerminate(); exit(EXIT_FAILURE); }
    volkInitialize();

    // use ambient static event provider
    ambientIO::handleGlfw(window);

    // create application
    auto app = std::make_shared<SatelliteExample::GltfViewer>(argc, argv, window);

    glfwTerminate();
    exit(EXIT_SUCCESS);
}
