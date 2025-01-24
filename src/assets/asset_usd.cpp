#include "asset_usd.h"

#ifdef USD_ENABLED

#define BOOST_LIB_TOOLSET "vc142"
#define BOOST_WHATEVER_NO_LIB
#define BOOST_ALL_NO_LIB 
//#pragma comment(lib, "boost_python311-vc143-mt-x64-1_82.lib")
#pragma comment(lib, "boost_python311-vc143-mt-gd-x64-1_82.lib")

#define NOMINMAX
#define TBB_USE_ASSERT 0
#define TBB_USE_THREADING_TOOLS 0

#include <pxr/pxr.h>
#include <pxr/usd/sdf/api.h>
#include <pxr/usd/sdf/pool.h>
#include <pxr/usd/sdf/abstractData.h>
#include <pxr/usd/sdf/fileFormat.h>
#include <pxr/base/tf/declarePtrs.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>

#include <pxr/base/gf/vec3d.h>
#include <pxr/base/gf/quatd.h>


#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/variantSets.h>
#include <pxr/usd/usd/variantSets.h>
#include <pxr/usd/usd/payloads.h>
#include <pxr/usd/usd/references.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usd/primCompositionQuery.h>

#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/subset.h>

#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ar/defaultResolver.h"
#include "pxr/usd/ar/resolverContextBinder.h"



//#pragma comment(lib,"usd_m.lib")
#pragma comment(lib,"usd_sdf.lib")
#pragma comment(lib,"usd_usd.lib")
#pragma comment(lib,"usd_usdGeom.lib")

#pragma comment(lib,"usd_ar.lib")   // asst resolver
#pragma comment(lib,"usd_vt.lib")   
#pragma comment(lib,"usd_tf.lib")
#pragma comment(lib,"usd_gf.lib")
#pragma comment(lib,"usd_pcp.lib")
#pragma comment(lib,"usd_arch.lib")


static pxr::TfToken uv_token = pxr::TfToken("primvars:st", pxr::TfToken::Immortal);
static pxr::TfToken uv_idx_token = pxr::TfToken("primvars:st:indices", pxr::TfToken::Immortal);

static pxr::TfToken xform_rotate_token = pxr::TfToken("xformOp:rotateXYZ", pxr::TfToken::Immortal);
static pxr::TfToken xform_scale_token = pxr::TfToken("xformOp:scale", pxr::TfToken::Immortal);
static pxr::TfToken xform_tra_token = pxr::TfToken("xformOp:translate", pxr::TfToken::Immortal);



/*
void usd_load_reansform(pxr::UsdPrim* prim)
{
    pxr::UsdGeomXformable xform(*prim);

    pxr::GfMatrix4d localTransform, worldTransform;
    bool lt;
    xform.GetLocalTransformation(&localTransform, &lt);
    worldTransform = xform.ComputeLocalToWorldTransform(pxr::UsdTimeCode::Default());

    pxr::GfVec3d pos = localTransform.ExtractTranslation();
    pxr::GfQuatd rot = localTransform.ExtractRotationQuat();

    double* ptr = pos.data();
   // printf("\n pos(%.3f,%.3f,%.3f)", ptr[0], ptr[1], ptr[2]);
   // pxr::GfRotation rot = transform.ExtractRotation();
}

void usd_print_prim_info(pxr::UsdPrim * prim)
{
    auto path       = prim->GetPath();
    auto variants   = prim->GetVariantSets();
    auto payloads   = prim->GetPayloads();
    auto refs       = prim->GetReferences();
    bool ismodel    = prim->IsModel();
    bool isloaded   = prim->IsLoaded();

    auto type       = prim->GetTypeName().GetText();
    auto path_c     = path.GetText();


    bool isgeom = prim->IsA<pxr::UsdGeomMesh>();
    if(prim->IsA<pxr::UsdGeomMesh>())
    {
        //load_mesh(prim);
       load_mesh(prim);
    }

    bool isxform = prim->IsA<pxr::UsdGeomXformable>();
    if(prim->IsA<pxr::UsdGeomXformable>())
    {
        usd_load_reansform(prim);
    }

    if(isgeom && !isxform)
        printf("");

    auto primStack = prim->GetPrimStack();

    for(size_t i = 0;i < primStack.size(); ++i)
    {
        auto spec = primStack[i].GetSpec();
        auto payloadPrependedItems = spec.GetPayloadList().GetPrependedItems();
        auto referencePrependedItems = spec.GetReferenceList().GetPrependedItems();

        for (size_t j = 0; j < referencePrependedItems.size(); ++j)
        {
            pxr::SdfReference reference = referencePrependedItems[j];
         //   refs_assets.insert(reference.GetAssetPath());

            auto ppp = reference.GetPrimPath().GetText();
            auto prim2 = prim->GetStage()->GetPrimAtPath(reference.GetPrimPath());
            
            //   printf("\n reference %s", reference.GetAssetPath().c_str());
        }

        for (size_t j = 0; j < payloadPrependedItems.size(); ++j)
        {
            pxr::SdfPayload payload = payloadPrependedItems[j];
        //    payload_assets.insert(payload.GetAssetPath());
        //    printf("\n\tpayload: %s", payload.GetAssetPath().c_str());
        }
    }

    auto names = variants.GetNames();
    for(size_t i = 0; i < names.size(); ++i)
    {
        auto set = prim->GetVariantSet(names[i]);
        auto varianNames = set.GetVariantNames();
        for (size_t j = 0; j < varianNames.size(); ++j)
        {
            auto editContext = set.GetVariantEditContext();
            auto editTarget = set.GetVariantEditTarget();
            auto active = set.GetVariantSelection();
            auto pams = editTarget.GetMapFunction().GetString();

          //  auto spec0 = editTarget.GetSpecForScenePath(pxr::SdfPath("world/mp_wz_island/mp_wz_island_paths/mp_wz_island_geo/map_phosphate_mine"));
         //   auto spec1 = editTarget.GetSpecForScenePath(pxr::SdfPath("world/mp_wz_island/mp_wz_island_paths/mp_wz_island_geo/map_phosphate_mine{districtLod=proxy}"));
     //       pxr::SdfSpec spec = spec0;
     //       printf("%s", pams.c_str());
        //    set.BlockVariantSelection();
         //   set.SetVariantSelection(varianNames[0]);
      //  prim->s
        }
    }
}*/


void AssetUsd::set_export_path(const char* path)
{
    m_export_path = path;
    m_export_path_mesh = m_export_path + "/" + "meshes/";

    if (!std::filesystem::exists(path))
        std::filesystem::create_directories(path);

    if (!std::filesystem::exists(m_export_path_mesh))
        std::filesystem::create_directories(m_export_path_mesh);
}

void AssetUsd::scan_dir(const char* path)
{
    std::filesystem::path tmp(path);
    auto dir = tmp.parent_path();

    if (!std::filesystem::exists(dir))
        return;

    std::filesystem::recursive_directory_iterator  it(dir);// = { dir };
    for (; it != std::filesystem::end(it); it++)
    {
        auto path = it->path();
        if (it->is_directory())
            m_pathes.push_back(path);
    }
}

std::string AssetUsd::resolve_asset_path(const std::string& path)
{
    for(auto p : m_pathes)
    {
        auto tmp = p.u8string() + path;
        if( std::filesystem::exists(tmp) )
            return tmp;
    }

    return "";
}

void AssetUsd::load(const char* path)
{
  //  pxr::TfDebug::EnableAll<pxr::TF_DEBUG_CODES>();
//    pxr::TfDebug::Enable(pxr::TF_LOG_STACK_TRACE_ON_ERROR);
 //   pxr::TfDebug::Enable(pxr::TF_LOG_STACK_TRACE_ON_WARNING);
 //   pxr::TfDebug::Enable(pxr::TF_ERROR_MARK_TRACKING);
 //   pxr::TfDebug::Enable(pxr::TF_PRINT_ALL_POSTED_ERRORS_TO_STDERR);
 //   pxr::TfDebug::EnableAll<TF_DEBUG_CODES>();
    scan_dir(path);

    std::string ppp = path;
  //  auto stag = pxr::UsdStage::Open(std::string_view(ppp));
 //   auto stage = pxr::UsdStage::Open("D:/work/github/activision/caldera/caldera.usda", pxr::UsdStage::LoadAll);
  //  auto stage = pxr::UsdStage::Open("D:/work/github/activision/caldera/caldera.usda", pxr::UsdStage::LoadNone);
 //   auto stage = pxr::UsdStage::Open("D:/work/github/activision/caldera/assets/xmodel/props/cp_dlc3/lm_town_foliage_palm_date_med_half_crown_curved.gdt.usd", pxr::UsdStage::LoadAll);
    auto stage = pxr::UsdStage::Open("D:/work/github/activision/caldera/map_source/prefabs/br/wz_vg/mp_wz_island/residential/beach_cottage_02.usd", pxr::UsdStage::LoadAll);
/*
    auto ladable = stage->FindLoadable();
    if(!ladable.empty())
    {
        auto l = *ladable.begin();
        auto stage2 = stage->Load(l);

        parse_prim_info(&stage2, true);
    }

    for(auto l : ladable)
    {
        printf("\n%s", l.GetText());
    }*/

    //auto subprim = stage->Load("../data/blockout_ship_bridge_funnel_tower_01.usd");
   // auto stage = pxr::UsdStage::Open("D:/work/github/activision/USD_Content_v1-1/USD_Mini_Car_Kit/assets/vehicles/vehicleVariants.usda", pxr::UsdStage::LoadNone);

//    auto prim = stage->GetPrimAtPath(pxr::SdfPath("/world/mp_wz_island/mp_wz_island_paths/mp_wz_island_geo/map_phosphate_mine"));
//    usd_print_prim_info(&prim);

    auto range = stage->TraverseAll();
    for (auto it = range.begin(); it != range.end(); it++)
    {
        pxr::UsdPrim * prim = &(*it);
        parse_prim_info(prim);
    }
    stage->Unload();

    if(!m_payload_assets.empty())
    {
        printf("\npayloads:");
        for(auto p : m_payload_assets)
            printf("\n    %s", p.c_str());
    }

    if(!m_reference_assets.empty())
    {
        printf("\nreferences:");
        for (auto r : m_reference_assets)
            printf("\n    %s", r.c_str());
    }
}

void load_imageable(void* prim_ptr)
{
    pxr::UsdPrim* prim = (pxr::UsdPrim*)(prim_ptr);
    pxr::UsdGeomImageable imageable(*prim);
}

void AssetUsd::parse_prim_info(void* prim_ptr, bool load)
{
    pxr::UsdPrim* prim = (pxr::UsdPrim*)(prim_ptr);

    auto stage      = prim->GetStage();

    auto path       = prim->GetPath();
    auto variants   = prim->GetVariantSets();
    auto primStack  = prim->GetPrimStack();

    auto type_str  = prim->GetTypeName().GetText();
    auto path_str  = path.GetText();
    auto path_str0 = path.GetAbsoluteRootOrPrimPath().GetText();

    printf("\n%s ", path_str0);

    bool hasVariant     = prim->HasVariantSets();
    auto names          = variants.GetNames();
    auto activeVariant  = std::string("");
    for (size_t i = 0; i < names.size(); ++i)
    {
        auto set = prim->GetVariantSet(names[i]);
        auto varianNames = set.GetVariantNames();
        activeVariant = set.GetVariantSelection();
      /*  for (size_t j = 0; j < varianNames.size(); ++j)
        {
            auto editContext = set.GetVariantEditContext();
            auto editTarget = set.GetVariantEditTarget();
            auto activeVariant = set.GetVariantSelection();
        }*/
    }

    if (prim->IsA<pxr::UsdGeomMesh>())
    {
        printf("|GeomMesh");
        load_mesh(prim);
    }

    if (prim->IsA<pxr::UsdGeomSubset>())
    {
        printf("|GeomSubset");
        //  load_mesh(prim);
    }

    if (prim->IsA<pxr::UsdGeomXformable>())
    {
        printf("|Xform");
        load_transform(prim);
    }

    for (size_t i = 0; i < primStack.size(); ++i)
    {
        auto spec = primStack[i].GetSpec();
        auto payloadPrependedItems = spec.GetPayloadList().GetPrependedItems();
        auto referencePrependedItems = spec.GetReferenceList().GetPrependedItems();

        for (size_t j = 0; j < referencePrependedItems.size(); ++j)
        {
            pxr::SdfReference reference = referencePrependedItems[j];
            m_reference_assets.insert(reference.GetAssetPath());
        }

        for (size_t j = 0; j < payloadPrependedItems.size(); ++j)
        {
            pxr::SdfPayload payload = payloadPrependedItems[j];

            if(m_payload_assets.find(payload.GetAssetPath()) != m_payload_assets.end())
                continue;

            m_payload_assets.insert(payload.GetAssetPath());

            auto ctx = prim->GetStage()->GetPathResolverContext();
            auto def = ctx.Get<pxr::ArDefaultResolverContext>();
            pxr::ArResolverContextBinder binder(ctx);
            auto &resolver = pxr::ArGetResolver();
            auto fullpath = resolve_asset_path(payload.GetAssetPath());
            auto payloadPrimPath = payload.GetPrimPath().GetText();

            bool solid = activeVariant.find("lod") != -1;
            printf("\n     payload %s", payload.GetAssetPath().c_str());
            printf("\x1B[31m ");
            auto payload_stage = stage->Open(fullpath);
            for (auto c : payload_stage->TraverseAll())
                parse_prim_info(&c);

            payload_stage->Unload();
            printf("\x1b[37m ");
        }
    }
}


#pragma pack(push, 1)
struct mesh_header_t
{
    uint32_t   magick;
    uint32_t   submesh_count;

    int32_t    vertex_stride;
    int32_t    index_stride;

    int32_t    vertex_count;
    int32_t    index_count;

    float      bbox_max[4];
    float      bbox_min[4];
};
#pragma pack(pop)

struct vertex
{
    vec4    position;
    vec4    normal;
    vec4    tangent;
    vec4    uv;
};

void AssetUsd::load_mesh(void* prim_ptr)
{
    pxr::UsdPrim *prim = (pxr::UsdPrim*)(prim_ptr);
    pxr::UsdGeomMesh geometry(*prim);

    //prim->GetAttributeAtPath("primvars:st");

    auto name = strrchr(prim->GetPath().GetText(), '/') + 1;

    if(m_meshes.find(name) != m_meshes.end())
        return;

    m_meshes.insert(name);

    auto path = prim->GetPath();
    auto variants = prim->GetVariantSets();
    auto payloads = prim->GetPayloads();
    auto refs = prim->GetReferences();

    pxr::VtArray<pxr::GfVec3f> points;
    pxr::VtArray<pxr::GfVec3f> normals;
    pxr::VtArray<pxr::GfVec2f> textures;
 
    pxr::VtArray<int>     faceVertexCounts;
    pxr::VtArray<int>     faceVertexIndices;
    pxr::VtArray<int>     uvIndices;

    geometry.GetFaceVertexCountsAttr().Get(&faceVertexCounts);
    geometry.GetFaceVertexIndicesAttr().Get(&faceVertexIndices);
    geometry.GetPointsAttr().Get(&points);
    geometry.GetNormalsAttr().Get(&normals);

    auto uv = geometry.GetPrim().GetAttribute(uv_token);
    auto uvidx = geometry.GetPrim().GetAttribute(uv_idx_token);
    if(uv.IsValid()) uv.Get(&textures);
    if(uvidx.IsValid()) uvidx.Get(&uvIndices);

    if (strstr(name, "polySurf") != nullptr)
        printf("");

    std::vector faces(faceVertexCounts.begin(), faceVertexCounts.end());
    std::vector idx(faceVertexIndices.begin(), faceVertexIndices.end());

  //  pxr::UsdGeomPrimvar primvar = pxr::UsdGeomPrimvar(usdAttr);
 //   auto primvar = geometry.GetPrimvar(pxr::TfToken("primvars:st"));

    std::vector<vertex> vertexes(points.size());
    std::vector<uint16_t> indexes(faceVertexIndices.size());

    for (int i = 0; i < points.size(); ++i)
    {
        float* ptr = points[i].data();
        vertexes[i].position = math::make_vec4(ptr[0], ptr[1], ptr[2], 1.0f) * 0.0254f;
    }

    mesh_header_t header = {};

    header.vertex_stride = sizeof(vertex);
    header.index_stride = sizeof(uint16_t);

    header.vertex_count = vertexes.size();
    header.index_count = indexes.size();

    std::string mesh_path = m_export_path_mesh + "/" + name;
    FILE * file = fopen(mesh_path.c_str(), "wb+");
    if(file != nullptr)
    {
        fwrite(&header, sizeof(header), 1, file);
        fwrite(vertexes.data(), vertexes.size(), 1, file);
        fwrite(indexes.data(), indexes.size(), 1, file);
        fclose(file);
    }

}


void AssetUsd::load_transform(void* prim_ptr)
{
    pxr::UsdPrim* prim = (pxr::UsdPrim*)(prim_ptr);
    pxr::UsdGeomXformable xform(*prim);

    pxr::GfMatrix4d localTransform, worldTransform;
    bool lt;
    xform.GetLocalTransformation(&localTransform, &lt);
    worldTransform = xform.ComputeLocalToWorldTransform(pxr::UsdTimeCode::Default());

    pxr::GfVec3d pos = localTransform.ExtractTranslation();
    pxr::GfQuatd rot = localTransform.ExtractRotationQuat();

    double* ptr = pos.data();
}

#endif