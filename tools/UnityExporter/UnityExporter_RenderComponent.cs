using UnityEngine;
using UnityEditor;
using System.Collections.Generic;
using System.Linq;

namespace UnityJsonExporter
{
  public static class RenderComponentExporter
  {
    public static void ExportRender(this Exporter exporter, Renderer renderer, SceneNode node)
    {
      var meshFilter = renderer.gameObject.GetComponent<MeshFilter>();

      if(renderer == null || meshFilter == null) return;
      if(meshFilter.sharedMesh == null) return;

      exporter.ExportMesh(meshFilter.sharedMesh);

      node.renderer ??= new();
      node.renderer.mesh = exporter.GetOrCreateMeshGuid(meshFilter.sharedMesh);

      var sourceRenderer = PrefabUtility.GetCorrespondingObjectFromSource(renderer);
      sourceRenderer = sourceRenderer ?? renderer;
      sourceRenderer = renderer;

      if (sourceRenderer.sharedMaterial != null && AssetDatabase.TryGetGUIDAndLocalFileIdentifier(sourceRenderer.sharedMaterial, out var materialGuid, out long _))
        node.renderer.material = materialGuid;

      foreach (var material in sourceRenderer.sharedMaterials)
      {
        if (material == null || !AssetDatabase.TryGetGUIDAndLocalFileIdentifier(material, out var guid, out long localId))
          continue;

        node.renderer.materials.Add(guid);
        exporter.SaveMaterial(material);
      }

      if (renderer.lightmapIndex >= 0 && renderer.lightmapIndex < LightmapSettings.lightmaps.Length)
      {
        var lightMap = LightmapSettings.lightmaps[renderer.lightmapIndex].lightmapColor;
        var lightMapPath = AssetDatabase.GetAssetPath(lightMap);
        var lightMapGuid = AssetDatabase.AssetPathToGUID(lightMapPath);

        node.renderer.lightmap = lightMapGuid;
        node.renderer.lightmapScaleOffset = new XMLFloat4(renderer.lightmapScaleOffset);
      }
    }
  }
}