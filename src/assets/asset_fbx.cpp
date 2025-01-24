#include "asset_fbx.h"

#include <unordered_map>
#include <unordered_set>
#include <filesystem>

#ifdef _WIN32
  #ifdef _DEBUG
    #ifdef _WIN64
   //   #pragma comment(lib, "D:/sdk/fbxsdk/lib/vs2012/x86/debug/libfbxsdk.lib") 
      #pragma comment(lib, "vs2019/x64/debug/libfbxsdk.lib")// libfbxsdk-md libfbxsdk-mt
    #else
      #pragma comment(lib, "vs2019/x86/debug/libfbxsdk.lib")// libfbxsdk-md libfbxsdk-mt
    #endif
  #else
    #ifdef _WIN64
      #pragma comment(lib, "vs2019/x64/release/libfbxsdk.lib")
    #else
      #pragma comment(lib, "vs2019/x86/release/libfbxsdk.lib")
    #endif
  #endif
#endif

#define FBX_VEC4_2_VEC3(fbxvec)   math::make_vec3((float)fbxvec[0], (float)fbxvec[1], (float)fbxvec[2]);




const char * type2str(FbxNodeAttribute::EType type)
{
    switch (type)
    {
        case FbxNodeAttribute::eUnknown:          return "Unknown";
        case FbxNodeAttribute::eNull:             return "Null";
        case FbxNodeAttribute::eMarker:           return "Marker";
        case FbxNodeAttribute::eSkeleton:         return "Skeleton";
        case FbxNodeAttribute::eMesh:             return "Mesh";
        case FbxNodeAttribute::eNurbs:            return "Nurbs";
        case FbxNodeAttribute::ePatch:            return "Patch";
        case FbxNodeAttribute::eCamera:           return "Camera";
        case FbxNodeAttribute::eCameraStereo:     return "CameraStereo";
        case FbxNodeAttribute::eCameraSwitcher:   return "CameraSwitcher";
        case FbxNodeAttribute::eLight:            return "Light";
        case FbxNodeAttribute::eOpticalReference: return "OpticalReference";
        case FbxNodeAttribute::eOpticalMarker:    return "OpticalMarker";
        case FbxNodeAttribute::eNurbsCurve:       return "NurbsCurve";
        case FbxNodeAttribute::eTrimNurbsSurface: return "TrimNurbsSurface";
        case FbxNodeAttribute::eBoundary:         return "Boundary";
        case FbxNodeAttribute::eNurbsSurface:     return "NurbsSurface";
        case FbxNodeAttribute::eShape:            return "Shape";
        case FbxNodeAttribute::eLODGroup:         return "LODGroup";
        case FbxNodeAttribute::eSubDiv:           return "SubDiv";
        case FbxNodeAttribute::eCachedEffect:     return "CachedEffect";
        case FbxNodeAttribute::eLine:             return "Line";
    }
    return "";
}

template <class vertexType>
void replaceVert(vertexType & vertex, int boneIndex, float weight)
{
    float* ptr = &vertex.weights.x;
    for (int i = 0; i < 4; ++i)
    {
        if (ptr[i] < weight)
        {
            ptr[i] = weight;
            ptr[i] = (float)boneIndex;
            return;
        }
    }
}

static FbxCluster* GetDeformerClasterForBone(FbxNode * boneNode, FbxMesh * fbxMesh)
{
    int lSkinCount = fbxMesh->GetDeformerCount(FbxDeformer::eSkin);
    for (int lSkinIndex = 0; lSkinIndex < lSkinCount; ++lSkinIndex)
    {
        FbxSkin * lSkinDeformer = (FbxSkin *)fbxMesh->GetDeformer(lSkinIndex, FbxDeformer::eSkin);
        for (int i = 0; i < lSkinDeformer->GetClusterCount(); ++i)
        {
            if (lSkinDeformer->GetCluster(i)->GetLink() == boneNode)
                return lSkinDeformer->GetCluster(i);
        }
    }
    return NULL;
}


void AssetFbx::load_static_mesh(const std::string& path, std::vector<vec3>* verts, std::vector<uint16_t>* indx)
{
    AssetFbx asset;
    asset.load(path.c_str());

    auto first = asset.m_meshes_new.front();
    for(uint32_t i = 0; i < first->vertices.size(); ++i)
        verts->push_back(first->vertices[i].position);

    for (uint32_t i = 0; i < first->indexes.size(); ++i)
        indx->push_back(first->indexes[i]);
}

void AssetFbx::save(const char* path)
{
    auto float2uint32 = [](float v){ 
        union conv { float f; uint32_t ui; };
        conv result = {v};
        return  result.ui;
    };
  /*  union conv {
        float       f;
        uint32_t    ui;
    };*/
    /*fwrite(&(m_meshes_new.size()), sizeof(size_t), m_meshes_new.size(), fp);
    for(size_t i = 0; i < m_meshes_new.size(); ++i)
    {
        m_meshes_new[i]->name.size();
    }*/
    std::vector<uint32_t> data;
    for (size_t i = 0; i < m_meshes_new.size(); ++i)
    {
        auto submesh = m_meshes_new[i];
        auto count = submesh->vertices.size();

        for (size_t j = 0; j < submesh->vertices.size(); ++j)
        {        
            auto vert = submesh->vertices[j];
        }
    }

    FILE* fp = fopen(path, "wb");
    if(fp != nullptr)
    {
        fwrite(data.data(), sizeof(uint32_t), data.size(), fp);
        fclose(fp);
    }
}


AssetFbx::AssetFbx()
    :m_pScene(NULL)
    ,m_pFbxSdkManager(NULL) 
{
    m_frames = 0; 
    m_time = 0.0f;
}

bool AssetFbx::load(const char * filePath)
{
//    Measure  ms("\nnewfbx::load");
    assert(filePath);

    m_pFbxSdkManager = FbxManager::Create();
    FbxImporter* importer = FbxImporter::Create(m_pFbxSdkManager, "");

    if (!importer->Initialize(filePath, -1, m_pFbxSdkManager->GetIOSettings()))
    {
        FbxStatus status = importer->GetStatus();
        FBXSDK_printf("\nFailed importer: %s, - %s\n", filePath, status.GetErrorString());
        return false; // failed
    }

    m_pScene = FbxScene::Create(m_pFbxSdkManager, "temp_scene");

    importer->Import(m_pScene);
    importer->Destroy();

    auto sceneAxisSystem = m_pScene->GetGlobalSettings().GetAxisSystem();
    auto customAxisSystem = FbxAxisSystem(FbxAxisSystem::eMayaYUp);
    if (sceneAxisSystem != customAxisSystem)
    {
        customAxisSystem.ConvertScene(m_pScene);
    }

    FbxSystemUnit SceneSystemUnit = m_pScene->GetGlobalSettings().GetSystemUnit();
    if (abs(SceneSystemUnit.GetScaleFactor() - 1.0) < 0.0001)
    {
        //The unit in this example is centimeter.
        FbxSystemUnit::cm.ConvertScene(m_pScene);
    }

    FbxGeometryConverter lGeomConverter(m_pFbxSdkManager);
    lGeomConverter.Triangulate(m_pScene, true);
    lGeomConverter.SplitMeshesPerMaterial(m_pScene, true);

    m_pScene->FillAnimStackNameArray(m_AnimStackNameArray);

    FbxNode* pRootNode = m_pScene->GetRootNode();
    scan(pRootNode);
    printf("\n");

    buildhierarchy();

    std::unordered_set<FbxMesh*> unique(m_fbx_node_meshes.begin(), m_fbx_node_meshes.end());

    for(auto & m: m_fbx_node_meshes) // unique or m_fbx_node_meshes
    {
        s_mesh* mesh = loadmesh(m);
        if(mesh)
            m_meshes_new.push_back(mesh);
    }

    loadanim();

    return true;
}

void AssetFbx::unload()
{
    m_frames = 0;
    m_time = 0.0f;

    for(size_t i = 0; i < m_meshes_new.size(); ++i)
    {
        delete m_meshes_new[i];
    }

    m_meshes_new.clear();
    m_joints.clear();

    m_fbx_node_meshes.clear();
    m_fbx_node_bones.clear();

    if (m_pScene)
        m_pScene->Destroy();

    if (m_pFbxSdkManager)
        m_pFbxSdkManager->Destroy();
}


void AssetFbx::scan(FbxNode * node)
{
    static int deep = 0;
    if (node == NULL)
        return;

    for (int i = 0; i < deep; ++i)
        printf("   ");

    std::string attributes;
    for (int i = 0; i < node->GetNodeAttributeCount(); ++i)
    {
        auto attribute = node->GetNodeAttributeByIndex(i);
        auto type = attribute->GetAttributeType();
        attributes.append("(").append(type2str(type)).append(")");

        if (type == FbxNodeAttribute::eMesh)
            m_fbx_node_meshes.push_back(static_cast<FbxMesh*>(attribute));
        if (type == FbxNodeAttribute::eSkeleton)
            m_fbx_node_bones.push_back(node);
    }

    FbxAMatrix global = node->EvaluateGlobalTransform();
    FbxAMatrix local = node->EvaluateLocalTransform();
    auto qu = global.GetQ();

    printf("%s  %s(%.3f,  %.3f,  %.3f) (%.3f,  %.3f,  %.3f) \n", node->GetName(), attributes.c_str(),
        global.GetT()[0],global.GetT()[1], global.GetT()[2],
        local.GetT()[0], local.GetT()[1], local.GetT()[2]);
     //   qu.DecomposeSphericalXYZ()[0], 
     //   qu.DecomposeSphericalXYZ()[1], 
     //   qu.DecomposeSphericalXYZ()[2]);
    deep++;
    for (int i = 0; i < node->GetChildCount(); ++i)
    {
        FbxNode * child = node->GetChild(i);
        scan(child);
    }
    deep--;
}

AssetFbx::s_mesh * AssetFbx::loadmesh(FbxMesh * fbxMesh)
{
    if (!fbxMesh)
        return NULL;

    s_mesh * mesh = new s_mesh();
    mesh->name = fbxMesh->GetName();
    mesh->skinned = false;

    std::vector<vertexPNBTWIidx> & vertices = mesh->vertices;
    std::vector<uint32_t>  & indexes  = mesh->indexes;
    
    int polygonCount            = fbxMesh->GetPolygonCount();
    int conrolPointsCount       = fbxMesh->GetControlPointsCount();
    FbxVector4* pControlPoints  = fbxMesh->GetControlPoints();

    int skinDeformer            = fbxMesh->GetDeformerCount(FbxDeformer::eSkin);
    int blendDeformer           = fbxMesh->GetDeformerCount(FbxDeformer::eBlendShape);
    int vertcashDeformer        = fbxMesh->GetDeformerCount(FbxDeformer::eVertexCache);

    std::multimap<size_t, size_t> vertexIdxMap; // key - control point idx
    std::map<size_t, vertexPNBTWIidx> vertexHashMap;

    FbxStringList UVSetNameList;
    fbxMesh->GetUVSetNames(UVSetNameList);// Get the name of each set of UV coords
    for (int i = 0; i < polygonCount; i++)
    {
        int polygonSize = fbxMesh->GetPolygonSize(i);
        for (int j = 0; j < polygonSize; j++)
        {
            int controlPointIdx = fbxMesh->GetPolygonVertex(i, j);
            if (controlPointIdx > conrolPointsCount)
                continue;

            bool pUnmapped = true;
            FbxVector4 vPos = pControlPoints[controlPointIdx];
            FbxVector4 fbxNormal;
            FbxVector2 fbxTexCoords[8];

            fbxMesh->GetPolygonVertexNormal(i, j, fbxNormal);
            for (int uv = 0; uv < UVSetNameList.GetCount(); ++uv)
            {
                const char * uvname = UVSetNameList.GetStringAt(uv);
                fbxMesh->GetPolygonVertexUV(i, j, uvname, fbxTexCoords[uv], pUnmapped);
            }
            vertexPNBTWIidx vertex;
            vertex.idx = controlPointIdx;
            vertex.position = FBX_VEC4_2_VEC3(vPos);
            vertex.normal = FBX_VEC4_2_VEC3(fbxNormal);
            vertex.texcoord0 = {((float)fbxTexCoords[0][0], (float)fbxTexCoords[0][1])};
            vertex.texcoord1 = {((float)fbxTexCoords[1][0], (float)fbxTexCoords[1][1])};

            #if !REMOVE_DUPLICATES
            vertexHashMap.insert(std::make_pair(vertex.hash(), vertex));
            vertexIdxMap.insert(std::make_pair(vertex.idx, vertices.size()));
            indexes.push_back((uint32_t)vertices.size());
            vertices.push_back(vertex);
            #else
            auto itidx = vertexHashMap.find(vertex.hash());
            if (itidx == vertexHashMap.end())
            {
                uint16_t index = vertines.size();
                vertexHashMap.insert(std::make_pair(vertex.hash(), vertex));
                vertexIdxMap.insert(std::make_pair(vertex.idx, index));

                indexes.push_back(index);
                vertines.push_back(vertex);
            }
            else
            {
                auto indexIt = vertexIdxMap.find(itidx->second.idx);
                if(indexIt != vertexIdxMap.end())
                {
                    indexes.push_back(indexIt->second);
                }
            }
            #endif
        }
    }

    if (skinDeformer > 0)
        load_skin(fbxMesh, mesh);

    if (blendDeformer > 0)
        load_blendshape(fbxMesh, mesh);

    if (skinDeformer == 0)
    {
        int meshOwners = fbxMesh->GetNodeCount();
        FbxMatrix transform = fbxMesh->GetNode()->EvaluateLocalTransform();
        auto localPosition = fbxMesh->GetNode()->EvaluateLocalTranslation();
        auto localRotation = fbxMesh->GetNode()->EvaluateLocalRotation();
        auto localScale    = fbxMesh->GetNode()->EvaluateLocalScaling();

        FbxMatrix globalTransform = fbxMesh->GetNode()->EvaluateGlobalTransform();
        for (size_t i = 0; i < vertices.size(); ++i)
        {
            vec3 p = vertices[i].position;
            FbxVector4 fbxp = globalTransform.MultNormalize(FbxVector4(p.x, p.y, p.z, 0.0));
            vertices[i].position = FBX_VEC4_2_VEC3(fbxp);
        //    vertines[i].position *= 0.01f;
        }/**/
    }

    return mesh;
}



void AssetFbx::buildhierarchy()
{
    for (size_t i = 0; i < m_fbx_node_bones.size(); ++i)
    {
        joint join;
        sprintf(join.name, "%s", m_fbx_node_bones[i]->GetName());
        join.id = static_cast<int>(m_joints.size());
        join.parentid = -1;

        m_joints.push_back(join);
    }

    for (size_t i = 0; i < m_fbx_node_bones.size(); ++i)
    {
        for (int j = 0; j < m_fbx_node_bones[i]->GetChildCount(); j++)
        {
            FbxNode * child = m_fbx_node_bones[i]->GetChild(j);
            auto it = std::find(m_fbx_node_bones.begin(), m_fbx_node_bones.end(), child);
            if (it != m_fbx_node_bones.end())
            {
                size_t  d = std::distance(m_fbx_node_bones.begin(), it);
                m_joints[d].parentid = static_cast<int>(i);
            }
        }
    }
}

void AssetFbx::load_bind_poses()
{
    // set bind poses
    for (size_t i = 0; i < m_fbx_node_bones.size(); ++i)
    {
        auto * node = m_fbx_node_bones[i];
        for (size_t j = 0; j < m_fbx_node_meshes.size(); ++j)
        {
            FbxCluster * cluster = GetDeformerClasterForBone(node, m_fbx_node_meshes[j]);
            if (!cluster)
                continue;

            FbxAMatrix lClusterGlobalInitPosition;
            lClusterGlobalInitPosition = cluster->GetTransformLinkMatrix(lClusterGlobalInitPosition); // bind pose 

            FbxVector4 t = lClusterGlobalInitPosition.GetT();
            FbxVector4 s = lClusterGlobalInitPosition.GetS();
            FbxQuaternion q = lClusterGlobalInitPosition.GetQ();

            m_joints[i].bind_pos = math::make_vec3((float)t[0], (float)t[1], (float)t[2]);
            m_joints[i].bind_rot = quat((float)q[0], (float)q[1], (float)q[2], (float)q[3]);
        }
    }
}

AssetFbx::s_animation * AssetFbx::loadanim()
{
    for (int i = 0; i < m_pScene->GetSrcObjectCount<FbxAnimStack>(); i++)
    {
        FbxTimeSpan pTimeInterval;
        FbxAnimStack* animStack = m_pScene->GetSrcObject<FbxAnimStack>(i);
        m_pScene->SetCurrentAnimationStack(animStack);

        int frameCount = 0;
        for (size_t j = 0; j < m_fbx_node_bones.size(); ++j)
        {
            m_fbx_node_bones[j]->GetAnimationInterval(pTimeInterval, animStack);
            frameCount = fmaxf(frameCount, (int)pTimeInterval.GetStop().GetFrameCount());
        }
        m_frames = frameCount;

        FbxString lOutputString = "\nAnimation Stack Name: ";
        lOutputString += animStack->GetName();

        auto sp = animStack->GetLocalTimeSpan();
        auto stop = sp.GetStop();

        int h, m, s, fr, fld, res;
        auto frames = stop.GetFrameCount();
        auto timelen = stop.GetTime(h, m, s, fr, fld, res/*, FbxTime::eFrames120*/);//eFrames30
        //     FbxString strfr((int)frames);
        lOutputString += "\n frames";
        lOutputString += FbxString((int)frames);
        lOutputString += "\n\n";
        printf(lOutputString);
    }
    return NULL;
}


FbxAnimLayer * AssetFbx::get_animlayer(int id, int * frames_count)
{
    const int lAnimStackCount = m_AnimStackNameArray.GetCount();
    if (id < 0 || id >= lAnimStackCount)
        return nullptr;

    const char * name = m_AnimStackNameArray[id]->Buffer();
    FbxAnimStack * lCurrentAnimationStack = m_pScene->FindMember<FbxAnimStack>(name);

    if (frames_count)
    {
        auto takeInfo = m_pScene->GetTakeInfo(name);
        auto duration = takeInfo->mLocalTimeSpan.GetDuration();
        (*frames_count) = static_cast<int>(duration.GetFrameCount());
    }

    return lCurrentAnimationStack->GetMember<FbxAnimLayer>();
}


void AssetFbx::load_skin(FbxMesh * fbxMesh, s_mesh * mesh)
{
    auto & vertices = mesh->vertices;
    int deformerCount = fbxMesh->GetDeformerCount(FbxDeformer::eSkin);
    std::vector<FbxMatrix> fbxlinkTransformsMatrix(mesh->vertices.size());

    /// load skin data
    for (int deformerIdx = 0; deformerIdx < deformerCount; ++deformerIdx)
    {
        FbxSkin* skin = (FbxSkin*)(fbxMesh->GetDeformer(deformerIdx, FbxDeformer::eSkin));

        int lClusterCount = skin->GetClusterCount();
        for (int clusterIdx = 0; clusterIdx != lClusterCount; ++clusterIdx)
        {
            auto cluster = skin->GetCluster(clusterIdx);
            auto link = cluster->GetLink();
            if (cluster->GetLink() == NULL)
                continue;

            auto it = std::find(m_fbx_node_bones.begin(), m_fbx_node_bones.end(), cluster->GetLink());
            if (it == m_fbx_node_bones.end())
                continue;

            int boneIdx = static_cast<int>(std::distance(m_fbx_node_bones.begin(), it));
            mesh->skeleton.push_back(joint());
            mesh->skeleton.back().id = boneIdx;

            FbxAMatrix lReferenceGlobalInitPosition0;
            FbxMatrix lReferenceGlobalInitPosition = cluster->GetTransformMatrix(lReferenceGlobalInitPosition0);

            int indexCount  = cluster->GetControlPointIndicesCount();

            int* indices    = cluster->GetControlPointIndices();
            double* weights = cluster->GetControlPointWeights();

            for (int k = 0; k < indexCount; ++k)
            {
                int ixcounts = 0;
                int idx = indices[k];
#if 0
                auto vit = vertexIdxMap.equal_range(idx);
                for (auto& rangeIT = vit.first; rangeIT != vit.second; ++rangeIT)
                {
                    size_t vertIdx = rangeIT->second;
                    bool try2replace = true;
                    for (int w = 0; w < 4; ++w)
                    {
                        if (vertines[vertIdx].weights[w] < math::Epsilon)
                        {
                            vertines[vertIdx].weights[w] = (float)weights[k];
                            vertines[vertIdx].indices[w] = (float)boneIdx;
                            try2replace = false;
                            break;
                        }
                    }
                    if (try2replace)
                        replaceVert(vertines[vertIdx], boneIdx, (float)weights[k]);

                    fbxlinkTransformsMatrix[vertIdx] += lReferenceGlobalInitPosition * weights[k];
                }
#else
                for (size_t ii = 0; ii < vertices.size(); ++ii)
                {
                    if (vertices[ii].idx != idx)
                        continue;

                    bool try2replace = true;
                    float* ptr = (float*)&vertices[ii].weights.x;
                    for (int w = 0; w < 4; ++w)
                    {
                        if (ptr[w] < math::Epsilon)
                        {
                            ptr[w] = (float)weights[k];
                            ptr[w] = (float)boneIdx;
                            try2replace = false;
                            break;
                        }
                    }
                    if (try2replace)
                        replaceVert(vertices[ii], boneIdx, (float)weights[k]);

                    fbxlinkTransformsMatrix[ii] += lReferenceGlobalInitPosition * weights[k];
                }
#endif
            }
        }
    }

    for (size_t i = 0; i < vertices.size(); ++i)
    {
        vec3 p = vertices[i].position;
        FbxVector4 fbxp = fbxlinkTransformsMatrix[i].MultNormalize(FbxVector4(p.x, p.y, p.z, 0.0));
        vertices[i].position = FBX_VEC4_2_VEC3(fbxp);

        float wsum = (vertices[i].weights.x + vertices[i].weights.y + vertices[i].weights.z + vertices[i].weights.w);
        if (wsum != 0.0f)
            vertices[i].weights /= wsum;
    }
}


void AssetFbx::load_blendshape(FbxMesh * fbxMesh, s_mesh* mesh)
{
    int lVertexCount = fbxMesh->GetControlPointsCount();

    auto pVertexArray = fbxMesh->GetControlPoints();

    FbxVector4* lSrcVertexArray = pVertexArray;
    FbxVector4* lDstVertexArray = new FbxVector4[lVertexCount];
    memcpy(lDstVertexArray, lSrcVertexArray, lVertexCount * sizeof(FbxVector4));

    int lBlendShapeDeformerCount = fbxMesh->GetDeformerCount(FbxDeformer::eBlendShape);
    for (int lBlendShapeIndex = 0; lBlendShapeIndex < lBlendShapeDeformerCount; ++lBlendShapeIndex)
    {
        FbxBlendShape* lBlendShape = (FbxBlendShape*)fbxMesh->GetDeformer(lBlendShapeIndex, FbxDeformer::eBlendShape);

        int lBlendShapeChannelCount = lBlendShape->GetBlendShapeChannelCount();
        for (int lChannelIndex = 0; lChannelIndex < lBlendShapeChannelCount; ++lChannelIndex)
        {
            FbxBlendShapeChannel* lChannel = lBlendShape->GetBlendShapeChannel(lChannelIndex);
            if (!lChannel) continue;
            auto name = lChannel->GetName();


            auto nodename = fbxMesh->GetNode()->GetName();

            printf("\nblendshape: %s(%s)", nodename, name);

            int frames_count = 0;

            FbxAnimLayer* animLayer = get_animlayer(0, &frames_count);
            FbxAnimCurve* animCurve = fbxMesh->GetShapeChannel(lBlendShapeIndex, lChannelIndex, animLayer);

            int frame_datasize = lVertexCount * lBlendShapeChannelCount * sizeof(vec3);

            if (!animCurve) continue;

            for (int i = 0; i < frames_count; ++i)
            {
                FbxTime animtime;
                animtime.SetFrame(i);
                double lWeight = animCurve->Evaluate(animtime);
                int lShapeCount = lChannel->GetTargetShapeCount();
                double* lFullWeights = lChannel->GetTargetShapeFullWeights();

                int lStartIndex = -1;
                int lEndIndex = -1;
                for (int lShapeIndex = 0; lShapeIndex < lShapeCount; ++lShapeIndex)
                {
                    if (lWeight > 0 && lWeight <= lFullWeights[0])
                    {
                        lEndIndex = 0;
                        break;
                    }
                    if (lWeight > lFullWeights[lShapeIndex] && lWeight < lFullWeights[lShapeIndex + 1])
                    {
                        lStartIndex = lShapeIndex;
                        lEndIndex = lShapeIndex + 1;
                        break;
                    }
                }

                FbxShape * lStartShape = (lStartIndex > -1) ? lChannel->GetTargetShape(lEndIndex) : nullptr;
                FbxShape * lEndShape = (lEndIndex > -1) ? lChannel->GetTargetShape(lEndIndex) : nullptr;
                //The weight percentage falls between base geometry and the first target shape.
                if (lStartIndex == -1 && lEndShape)
                {
                    double lEndWeight = lFullWeights[0];
                    lWeight = (lWeight / lEndWeight) * 100;     // Calculate the real weight.
                    memcpy(lDstVertexArray, lSrcVertexArray, lVertexCount * sizeof(FbxVector4));
                    for (int j = 0; j < lVertexCount; j++)
                    {
                        // Add the influence of the shape vertex to the mesh vertex.
                        FbxVector4 lInfluence = (lEndShape->GetControlPoints()[j] - lSrcVertexArray[j]) * lWeight * 0.01;
                        lDstVertexArray[j] += lInfluence;
                    }
                }

                float bbmin = FBXSDK_FLOAT_MAX;
                float bbmax = FBXSDK_FLOAT_MIN;
                for (int v = 0; v < lVertexCount; v++)
                {
                    for (int d = 0; d < 3; d++)
                    {
                        bbmin = fminf(bbmin, (float)lDstVertexArray[v][d]);
                        bbmax = fmaxf(bbmax, (float)lDstVertexArray[v][d]);
                    }
                }
                printf("\nframe[%d]: %.3f : %.3f ", i, bbmin, bbmax);
            } // 
        } // foreach lChannelIndex
    } //foreach lBlendShapeIndex

    delete [] lDstVertexArray;
}

#if 0
struct s_dbg_mesh
{
    vec3 pos;
    vec3 norm;
    vec2 uv0;
    size_t c;
};


void AssetFbx::draw(iEngine * engine)
{

    if (!m_pScene)
        return;
    RenderSystem *rs = ComponentSystem::get_ecs<RenderSystem>();
    RenderDevice * renderer = engine->render();

    static int currframe = 0;
    static int prevframe = 0;

    m_time += Time::dt();// *0.5f;
    currframe = (int)m_time;
    
    FbxAnimStack* currAnimStack;
    if ( currAnimStack = m_pScene->GetCurrentAnimationStack(), currAnimStack != NULL)
    {
        auto sp = currAnimStack->GetReferenceTimeSpan();
        auto stop = sp.GetStop();
        auto start = sp.GetStart();

        int startFrameCount = (int)start.GetFrameCount();
        int stopFrameCount = (int)stop.GetFrameCount();
        if (stopFrameCount != 0)
        {
            currframe = currframe % stop.GetFrameCount();
        }
    }

    float scale = 1.01f;
    for (size_t i = 0; i < m_bones.size(); ++i)
    {
        auto * node = m_bones[i];

        FbxTime fbxtime = FbxTimeSeconds(m_time);

        if (fbxtime.GetFrameCount() > m_frames)
            m_time = 0.0f;

        //fbxtime.SetFrame(currframe);
        FbxAMatrix globalMatrix = node->EvaluateGlobalTransform(fbxtime);

        auto localPos = node->EvaluateLocalTranslation(fbxtime);
        auto localRot = node->EvaluateLocalRotation(fbxtime);

        FbxVector4 gTranslation = globalMatrix.GetT();
        FbxQuaternion pRotation = globalMatrix.GetQ();

        m_joints[i].animated_pos = vec3(gTranslation[0], gTranslation[1], gTranslation[2]) ;
        m_joints[i].animated_rot = quat(pRotation[0], pRotation[1], pRotation[2], pRotation[3]);
    }

    for (size_t i = 0; i < m_joints.size(); i++)
    {
        int parentId = m_joints[i].parentid;
        
        vec3 p0 = m_joints[i].bind_pos * scale;
        vec3 p1 = ((parentId != -1) ? m_joints[parentId].bind_pos : p0 + vec3::Up*0.1f) * scale;

        vec3 pa0 = m_joints[i].animated_pos * scale;
        vec3 pa1 = ((parentId != -1) ? m_joints[parentId].animated_pos : pa0 + vec3::Up*0.1f) * scale;
  //      bool isEnd = fabs(p0.x) < 0.01 && fabs(p0.y) < 0.01 && fabs(p0.z) < 0.01;
  //      if (isEnd)
  //          continue;

        vec3 up = m_joints[i].bind_rot.rotate_point(vec3::Up);
        vec3 f = m_joints[i].bind_rot.rotate_point(vec3::Forward);

        rs->dbg_drawline(pa0, pa1, color32::Red());
        rs->dbg_drawline(p0, p1, color32::White());
     //   rs->dbg_drawline(pl0, pl1, RGBA(0, 0, 255, 255));
    }
    
    if (m_meshes_new.empty())
        return;

    for (size_t i = 0; i < m_meshes_new.size(); ++i)
    {
        s_mesh* mesh = m_meshes_new[i];

        if (mesh->_vb == NULL && mesh->_ib == NULL)
        {
            VertexDeclaration * vd = renderer->createVertexDeclaration("p3:n3:uv0:c4");
            mesh->_vb = renderer->createVertexBuffer(vd, mesh->m_vertines.size(), true);
            mesh->_ib = renderer->createIndexBuffer(eIndexFormat_uint16, mesh->m_indexes.size(), mesh->m_indexes.data());
        }

        s_dbg_mesh *m = (s_dbg_mesh*)mesh->_vb->map(kAccessMode_Write);
        if (!m){
            mesh->_vb->unmap();
            continue;
        }

        // update animated global pos
        for (size_t jn = 0; jn < m_joints.size(); ++jn)
        {
            m_joints[jn].evaluate_global();
        }

        for (size_t i = 0; i < mesh->m_vertines.size(); i++)
        {
            const vec3 & p = mesh->m_vertines[i].position;
            const vec4 & idx = mesh->m_vertines[i].indices;
            const vec4 & wgx = mesh->m_vertines[i].weights;
            const vec4 & nrm = mesh->m_vertines[i].normal;
            const vec2 & uv0 = mesh->m_vertines[i].texcoord0;

            vec3 v = m_joints.empty() ? p : vec3(0.0f, 0.0f, 0.0f);
            for (int j = 0; j < 4; ++j)
            {
                if (wgx[j] > 0.0f)
                {
                    const joint & jnt = m_joints[int(idx[j])];// joint idx

                    const quat & g_rot = jnt.globalrot;
                    const vec3 & g_pos = jnt.globalpos;

                    // v += matGloal.transform_point(p) * boneWeight;
                    v += (g_rot.rotate_point(p) + g_pos) * wgx[j];
                }
            }/**/

            m[i].pos = v * scale;
            m[i].c = RGBA(uv0.x * 255, uv0.y * 255, 255, 255);
            size_t color = RGBA(255, 255, 255, 255);
        }
        mesh->_vb->unmap();

        renderer->updateVertexBuffer(mesh->_vb, 0, mesh->m_vertines.data(), mesh->m_vertines.size())

        rs->dbg_mesh(mesh->_vb, mesh->_ib);
    }
}
#endif