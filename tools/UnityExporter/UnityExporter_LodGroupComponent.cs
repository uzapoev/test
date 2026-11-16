using UnityEngine;
using UnityEditor;
using System.Collections.Generic;
using System.Linq;

namespace UnityJsonExporter
{
  public static class LODExporter
  {
    public static void ExportLODGroup(this Exporter exporter, LODGroup lodGroup, SceneNode node)
    {
      if (lodGroup == null)
        return;

      var nodeLodGroup = new LODGroupComponent()
      {
        fadeTransitionWidth = lodGroup.size, animateCrossFading = lodGroup.animateCrossFading
      };

      var lods = lodGroup.GetLODs();
      bool isSimpleLOD = true;

      foreach (var lod in lods)
      {
        var nodeLod = new NodeLODS { screenRelativeTransitionHeight = lod.screenRelativeTransitionHeight };

        foreach (var renderer in lod.renderers)
        {
          if (renderer == null)
            continue;

          if (!IsSimpleLODRenderer(renderer.gameObject))
          {
            isSimpleLOD = false;
            break;
          }

          ExportRendererForLOD(exporter, renderer, nodeLod);
        }

        if (!isSimpleLOD)
          break;

        nodeLodGroup.lods.Add(nodeLod);
      }

      node.lodGroup = nodeLodGroup;

      if (isSimpleLOD)
      {
        node.childs = null!;
      }
    }


    private static bool IsSimpleLODRenderer(GameObject go)
    {
      if (go.transform.childCount > 0)
        return false;

      var components = go.GetComponents<Component>();

      foreach (var comp in components)
      {
        switch (comp)
        {
          case Transform _:
          case MeshFilter _:
          case MeshRenderer _:
          case SkinnedMeshRenderer _:
            continue;

          default:
            return false;
        }
      }

      return true;
    }

    private static void ExportRendererForLOD(Exporter exporter, Renderer renderer, NodeLODS nodeLod)
    {
      if (renderer is MeshRenderer meshRenderer)
      {
        var meshFilter = renderer.GetComponent<MeshFilter>();
        if (meshFilter?.sharedMesh == null)
          return;

        exporter.ExportMesh(meshFilter.sharedMesh);     // the mesh file + its place in the "meshes" list of the scene
        var meshGuid = exporter.GetOrCreateMeshGuid(meshFilter.sharedMesh);
        nodeLod.meshGuids.Add(meshGuid);

        var mats = new List<string>();
        foreach (var mat in meshRenderer.sharedMaterials)
        {
          if (mat == null)
            continue;

          var matGuid = GetMaterialGuid(mat);
          mats.Add(matGuid);
          exporter.SaveMaterial(mat); //
        }

        nodeLod.materialGuids = mats;
        //nodeLod.materialGuids.Add(mats);
      }
      else if (renderer is SkinnedMeshRenderer skinned)
      {
        if (skinned.sharedMesh != null)
        {
          exporter.ExportMesh(skinned.sharedMesh);
          var meshGuid = exporter.GetOrCreateMeshGuid(skinned.sharedMesh);
          nodeLod.meshGuids.Add(meshGuid);

          var mats = new List<string>();
          foreach (var mat in skinned.sharedMaterials)
          {
            if (mat == null)
              continue;

            mats.Add(GetMaterialGuid(mat));
            exporter.SaveMaterial(mat);
          }
          nodeLod.materialGuids = mats;
        }
      }
    }

    private static string GetMaterialGuid(Material mat)
    {
      if (mat == null)
        return "";

      AssetDatabase.TryGetGUIDAndLocalFileIdentifier(mat, out string guid, out _);
      return guid;
    }
  }
}