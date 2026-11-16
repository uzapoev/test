using System;
using UnityEngine;
using UnityEditor;
using System.IO;
using System.Collections.Generic;
using Object = UnityEngine.Object;

namespace UnityJsonExporter
{
  [Serializable]
  public class TerrainComponent
  {
    public Vector3 terrainSize;
    public int heightmapResolution;
    public int alphamapResolution;
    public int baseMapResolution;

    public string terrainMeshGuid = "";
    public string heightmapGuid = "";
    public List<string> layerGuids = new List<string>(); // Terrain Layers (splat textures)

    public float heightmapScaleY;
    public bool drawHeightmap = true;
    public bool drawTreesAndFoliage = true;
  }

  public static class TerrainComponentExporter
  {
    public static void ExportTerrain(this Exporter exporter, Terrain terrain, SceneNode node)
    {
      if (terrain == null || terrain.terrainData == null)
        return;

      var terrainData = terrain.terrainData;
      var terrainComponent = new TerrainComponent
      {
        terrainSize = terrainData.size,
        heightmapResolution = terrainData.heightmapResolution,
        alphamapResolution = terrainData.alphamapResolution,
        baseMapResolution = terrainData.baseMapResolution,
        heightmapScaleY = terrainData.heightmapScale.y,
        drawHeightmap = terrain.drawHeightmap,
        drawTreesAndFoliage = terrain.drawTreesAndFoliage
      };

      // 1. Heightmap (RAW)
      string heightmapGuid = ExportHeightmap(exporter, terrainData, terrain.name);
      terrainComponent.heightmapGuid = heightmapGuid;

      // 2. Terrain Layers
      ExportTerrainLayers(exporter, terrainData, terrainComponent);

      // 3. МЕШ ТЕРРЕЙНА ДЛЯ ТЕСТІВ ← НОВЕ
      string terrainMeshGuid = ExportTerrainAsMesh(exporter, terrain, terrain.name);
      terrainComponent.terrainMeshGuid = terrainMeshGuid;

      node.terrain = terrainComponent;
    }

    // === НОВИЙ МЕТОД ===
    private static string ExportTerrainAsMesh(Exporter exporter, Terrain terrain, string terrainName)
    {
      return "";
      /*
      if (terrain == null)
        return "";

      string meshName = $"{terrainName}_terrain_mesh";
      string dstPath = Path.Combine(exporter._rootRawMeshExportPath, $"{meshName}.mesh");

      if (File.Exists(dstPath))
        return meshName;

      Mesh terrainMesh = GenerateTerrainMesh(terrain.terrainData);

      if (terrainMesh == null)
      {
        Debug.LogWarning($"Failed to generate mesh for terrain: {terrainName}");
        return "";
      }

      exporter.SaveMesh(terrainMesh, meshName, dstPath);
      Object.DestroyImmediate(terrainMesh);

      return meshName;*/
    }

    private static Mesh GenerateTerrainMesh(TerrainData terrainData)
    {
      int resolution = terrainData.heightmapResolution;
      Vector3 size = terrainData.size;

      Mesh mesh = new Mesh();
      mesh.name = "Generated_Terrain_Mesh";

      Vector3[] vertices = new Vector3[resolution * resolution];
      Vector2[] uv = new Vector2[resolution * resolution];
      int[] triangles = new int[(resolution - 1) * (resolution - 1) * 6];

      float[,] heights = terrainData.GetHeights(0, 0, resolution, resolution);

      int index = 0;
      for (int z = 0; z < resolution; z++)
      {
        for (int x = 0; x < resolution; x++)
        {
          float height = heights[z, x];

          vertices[index] = new Vector3(x * size.x / (resolution - 1),
            height * size.y,
            z * size.z / (resolution - 1));

          uv[index] = new Vector2((float) x / (resolution - 1),
            (float) z / (resolution - 1));

          index++;
        }
      }

      index = 0;
      for (int z = 0; z < resolution - 1; z++)
      {
        for (int x = 0; x < resolution - 1; x++)
        {
          int a = z * resolution + x;
          int b = a + 1;
          int c = a + resolution;
          int d = c + 1;

          triangles[index++] = a;
          triangles[index++] = c;
          triangles[index++] = b;

          triangles[index++] = b;
          triangles[index++] = c;
          triangles[index++] = d;
        }
      }

      mesh.vertices = vertices;
      mesh.uv = uv;
      mesh.triangles = triangles;

      mesh.RecalculateNormals();
      mesh.RecalculateTangents();
      mesh.RecalculateBounds();

      return mesh;
    }

    private static string ExportHeightmap(Exporter exporter, TerrainData terrainData, string terrainName)
    {
      return "";
      /*
      string filename = $"{terrainName}_heightmap.raw";
      string path = Path.Combine(exporter.GetRootTerrainPath(), filename);

      if (File.Exists(path))
        return filename;

      var heights = terrainData.GetHeights(0, 0, terrainData.heightmapResolution, terrainData.heightmapResolution);
      byte[] rawBytes = new byte[heights.Length * 2];

      int i = 0;
      for (int y = 0; y < heights.GetLength(0); y++)
      for (int x = 0; x < heights.GetLength(1); x++)
      {
        ushort value = (ushort) (heights[y, x] * ushort.MaxValue);
        rawBytes[i++] = (byte) (value & 0xFF);
        rawBytes[i++] = (byte) ((value >> 8) & 0xFF);
      }

      File.WriteAllBytes(path, rawBytes);
      return filename;*/
    }

    private static void ExportTerrainLayers(Exporter exporter, TerrainData terrainData, TerrainComponent tc)
    {
      foreach (var layer in terrainData.terrainLayers)
      {
        if (layer?.diffuseTexture == null)
          continue;

        string guid = AssetDatabase.AssetPathToGUID(AssetDatabase.GetAssetPath(layer));
        if (!string.IsNullOrEmpty(guid))
        {
          tc.layerGuids.Add(guid);
          exporter.ExportTexture(layer.diffuseTexture);
        }
      }
    }
  }
}