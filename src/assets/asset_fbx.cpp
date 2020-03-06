//#include <algorithm>

#include "asset_fbx.h"


#define FBX_VEC4_2_VEC3(fbxvec)   vec3((float)fbxvec[0], (float)fbxvec[1], (float)fbxvec[2]);

const char * type2str(FbxNodeAttribute::EType type)
{
    switch (type)
    {
        case FbxNodeAttribute::eUnknown:          return "eUnknown";
        case FbxNodeAttribute::eNull:             return "eNull";
        case FbxNodeAttribute::eMarker:           return "eMarker";
        case FbxNodeAttribute::eSkeleton:         return "eSkeleton";
        case FbxNodeAttribute::eMesh:             return "eMesh";
        case FbxNodeAttribute::eNurbs:            return "eNurbs";
        case FbxNodeAttribute::ePatch:            return "ePatch";
        case FbxNodeAttribute::eCamera:           return "eCamera";
        case FbxNodeAttribute::eCameraStereo:     return "eCameraStereo";
        case FbxNodeAttribute::eCameraSwitcher:   return "eCameraSwitcher";
        case FbxNodeAttribute::eLight:            return "eLight";
        case FbxNodeAttribute::eOpticalReference: return "eOpticalReference";
        case FbxNodeAttribute::eOpticalMarker:    return "eOpticalMarker";
        case FbxNodeAttribute::eNurbsCurve:       return "eNurbsCurve";
        case FbxNodeAttribute::eTrimNurbsSurface: return "eTrimNurbsSurface";
        case FbxNodeAttribute::eBoundary:         return "eBoundary";
        case FbxNodeAttribute::eNurbsSurface:     return "eNurbsSurface";
        case FbxNodeAttribute::eShape:            return "eShape";
        case FbxNodeAttribute::eLODGroup:         return "eLODGroup";
        case FbxNodeAttribute::eSubDiv:           return "eSubDiv";
        case FbxNodeAttribute::eCachedEffect:     return "eCachedEffect";
        case FbxNodeAttribute::eLine:             return "eLine";
    }
    return "";
}

template <class vertexType>
void checkForReplace(vertexType & vertex, int boneIndex, float weight)
{
    for (int i = 0; i < 4; ++i)
    {
        if (vertex.weights[i] < weight)
        {
            vertex.weights[i] = weight;
            vertex.indices[i] = (float)boneIndex;
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

AssetFbx::AssetFbx()
    :m_pScene(NULL)
    ,m_pFbxSdkManager(NULL)
{
    m_frames = 0;
    m_time = 0.0f;
}

void AssetFbx::load(const char * filePath)
{
//    Measure  ms("\nnewfbx::load");
    assert(filePath);

    m_pFbxSdkManager = FbxManager::Create();
    FbxImporter* importer = FbxImporter::Create(m_pFbxSdkManager, "");

    if (!importer->Initialize(filePath, -1, m_pFbxSdkManager->GetIOSettings()))
    {
        FbxStatus status = importer->GetStatus();
        FBXSDK_printf("\nFailed importer: %s, - %s\n", filePath, status.GetErrorString());
        return; // failed
    }

    m_pScene = FbxScene::Create(m_pFbxSdkManager, "temp_scene");

    importer->Import(m_pScene);
    importer->Destroy();

    FbxGeometryConverter lGeomConverter(m_pFbxSdkManager);
    lGeomConverter.Triangulate(m_pScene, true);
    lGeomConverter.SplitMeshesPerMaterial(m_pScene, true);

    m_pScene->FillAnimStackNameArray(m_AnimStackNameArray);


    FbxNode* pRootNode = m_pScene->GetRootNode();
    scan(pRootNode);
    printf("\n");


    buildhierarchy();

    for (size_t i = 0; i < m_meshes.size(); ++i)
    {
        s_mesh * mesh = loadmesh(m_meshes[i]);
        if (mesh != NULL)
        {
            m_meshes_new.push_back(mesh);
        }
    }

    loadbinds();
    loadanim();
}

void AssetFbx::unload()
{
    m_frames = 0;
    m_time = 0.0f;
    m_meshes_new.clear();
    m_joints.clear();

    m_meshes.clear();
    m_bones.clear();
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
            m_meshes.push_back(static_cast<FbxMesh*>(attribute));
        if (type == FbxNodeAttribute::eSkeleton)
            m_bones.push_back(node);
    }
    printf("%s  %s \n", node->GetName(), attributes.c_str());
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

    s_mesh * mesh = new s_mesh;
    mesh->name = fbxMesh->GetName();

    std::vector<vertexPNBTWIidx> & vertines = mesh->m_vertines;
    std::vector<unsigned short>  & indexes = mesh->m_indexes;
    
    int polygonCount = fbxMesh->GetPolygonCount();
    int conrolPointsCount = fbxMesh->GetControlPointsCount();
    FbxVector4* pControlPoints = fbxMesh->GetControlPoints();

    int skinDeformer = fbxMesh->GetDeformerCount(FbxDeformer::eSkin);
    int blendDeformer = fbxMesh->GetDeformerCount(FbxDeformer::eBlendShape);
    int vertcashDeformer = fbxMesh->GetDeformerCount(FbxDeformer::eVertexCache);

    std::multimap<size_t, size_t> vertexIdxMap; // key - control point idx
    std::map<size_t, vertexPNBTWIidx> vertexHashMap;
    //std::signbit

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
            vertex.texcoord0 = vec2((float)fbxTexCoords[0][0], (float)fbxTexCoords[0][1]);
            vertex.texcoord1 = vec2((float)fbxTexCoords[1][0], (float)fbxTexCoords[1][1]);

            auto itidx = vertexHashMap.find(vertex.hash());
            if (itidx == vertexHashMap.end())
            {
                vertexHashMap.insert(std::make_pair(vertex.hash(), vertex));
                vertexIdxMap.insert(std::make_pair(vertex.idx, vertines.size()));

                indexes.push_back(vertines.size());
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
        }
    }

    // load materials
    int fbxMaterialCount = fbxMesh->GetNode()->GetMaterialCount();
    for (int i = 0; i < fbxMaterialCount; ++i)
    {
        FbxSurfaceMaterial * material = fbxMesh->GetNode()->GetMaterial(i);

        auto l = material->ShadingModel;
    }

    if (blendDeformer > 0)
        load_blendhapeinfo(fbxMesh);


    int deformerCount = fbxMesh->GetDeformerCount(FbxDeformer::eSkin);
    if (deformerCount == 0)
        return mesh;
    
    FbxMatrix  * fbxlinkTransformsMatrix = new FbxMatrix[vertines.size()];
    memset(fbxlinkTransformsMatrix, 0, sizeof(FbxMatrix)*vertines.size());

    /// load skin data
    for (int deformerIdx = 0; deformerIdx < deformerCount; ++deformerIdx)
    {
        FbxSkin * pFbxSkin = (FbxSkin*)(fbxMesh->GetDeformer(deformerIdx, FbxDeformer::eSkin));

        int lClusterCount = pFbxSkin->GetClusterCount();
        for (int clusterIdx = 0; clusterIdx != lClusterCount; ++clusterIdx)
        {
            FbxCluster* pFbxCluster = pFbxSkin->GetCluster(clusterIdx);
            if (pFbxCluster->GetLink() == NULL)
                continue;

            auto it = std::find(m_bones.begin(), m_bones.end(), pFbxCluster->GetLink());
            if (it == m_bones.end())
                continue;

            int boneIdx = std::distance(m_bones.begin(), it);
            mesh->m_skeleton.push_back(joint());

            FbxAMatrix lReferenceGlobalInitPosition0;
            FbxMatrix lReferenceGlobalInitPosition = pFbxCluster->GetTransformMatrix(lReferenceGlobalInitPosition0);

            int indexCount = pFbxCluster->GetControlPointIndicesCount();
            int* indices = pFbxCluster->GetControlPointIndices();
            double* weights = pFbxCluster->GetControlPointWeights();
            for (int k = 0; k < indexCount; ++k)
            {
                int ixcounts = 0;
                int idx = indices[k];
#if 1
                auto vit = vertexIdxMap.equal_range(idx);
                for (auto rangeIT = vit.first; rangeIT != vit.second; ++rangeIT)
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
                        checkForReplace(vertines[vertIdx], boneIdx, (float)weights[k]);

                    fbxlinkTransformsMatrix[vertIdx] += lReferenceGlobalInitPosition * weights[k];
                }
#else
                for (size_t ii = 0; ii < vertines.size(); ++ii)
                {
                    if (vertines[ii].idx == idx)
                    {
                        bool try2replace = true;
                        for (int w = 0; w < 4; ++w)
                        {
                            if (vertines[ii].weights[w] < EPSILON)
                            {
                                vertines[ii].weights[w] = (float)weights[k];
                                vertines[ii].indices[w] = (float)boneIdx;
                                try2replace = false;
                                break;
                            }
                        }
                        if (try2replace)
                            checkForReplace(vertines[ii], boneIdx, (float)weights[k]);

                        fbxlinkTransformsMatrix[ii] += lReferenceGlobalInitPosition * weights[k];
                    }
                }
#endif
            }
        }
    }


//    m_aabb.clear();
    for (size_t i = 0; i < vertines.size(); ++i)
    {
        vec3 p = vertines[i].position;
        FbxVector4 fbxp = fbxlinkTransformsMatrix[i].MultNormalize(FbxVector4(p.x, p.y, p.z, 0.0));
        vertines[i].position = FBX_VEC4_2_VEC3(fbxp);

        float wsum = (vertines[i].weights.x + vertines[i].weights.y + vertines[i].weights.z + vertines[i].weights.w);
        if (wsum != 0.0f)
            vertines[i].weights /= wsum;
    }
    delete[] fbxlinkTransformsMatrix;

    return mesh;
}



void AssetFbx::buildhierarchy()
{
    for (size_t i = 0; i < m_bones.size(); ++i)
    {
        joint join;
        sprintf(join.name, "%s", m_bones[i]->GetName());
        join.id = m_joints.size();
        join.parentid = -1;

        m_joints.push_back(join);
    }

    for (size_t i = 0; i < m_bones.size(); ++i)
    {
        for (int j = 0; j < m_bones[i]->GetChildCount(); j++)
        {
            FbxNode * child = m_bones[i]->GetChild(j);
            auto it = std::find(m_bones.begin(), m_bones.end(), child);
            if (it != m_bones.end())
            {
                size_t  d = std::distance(m_bones.begin(), it);
                m_joints[d].parentid = i;
            }
        }
    }
}

void AssetFbx::loadbinds()
{
    // set bind poses
    for (size_t i = 0; i < m_bones.size(); ++i)
    {
        auto * node = m_bones[i];
        for (size_t j = 0; j < m_meshes.size(); ++j)
        {
            FbxCluster * cluster = GetDeformerClasterForBone(node, m_meshes[j]);
            if (!cluster)
                continue;

            FbxAMatrix lClusterGlobalInitPosition;
            lClusterGlobalInitPosition = cluster->GetTransformLinkMatrix(lClusterGlobalInitPosition); // bind pose 

            FbxVector4 t = lClusterGlobalInitPosition.GetT();
            FbxVector4 s = lClusterGlobalInitPosition.GetS();
            FbxQuaternion q = lClusterGlobalInitPosition.GetQ();

            m_joints[i].bind_pos = vec3((float)t[0], (float)t[1], (float)t[2]);
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
        for (size_t j = 0; j < m_bones.size(); ++j)
        {
            m_bones[j]->GetAnimationInterval(pTimeInterval, animStack);
            frameCount = math::max(frameCount, (int)pTimeInterval.GetStop().GetFrameCount());
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
        (*frames_count) = duration.GetFrameCount();
    }

    return lCurrentAnimationStack->GetMember<FbxAnimLayer>();
}


void AssetFbx::load_skininfo(FbxMesh * fbxMesh)
{
}


void AssetFbx::load_blendhapeinfo(FbxMesh * fbxMesh)
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
                        bbmin = math::min(bbmin, (float)lDstVertexArray[v][d]);
                        bbmax = math::max(bbmax, (float)lDstVertexArray[v][d]);
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