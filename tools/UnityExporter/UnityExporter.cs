using System;
using System.Collections.Generic;
using System.Diagnostics.CodeAnalysis;
using System.IO;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Xml.Serialization;
using System.Text;
using UnityEditor;
using UnityEngine;
using UnityEngine.SceneManagement;
using Unity.Mathematics;
using UnityEngine.Rendering;
using Newtonsoft.Json;

// ReSharper disable InconsistentNaming
// ReSharper disable StringLiteralTypo
// ReSharper disable CheckNamespace
#pragma warning disable CS8618 // Non-nullable field must contain a non-null value when exiting constructor. Consider adding the 'required' modifier or declaring as nullable.

namespace UnityJsonExporter
{
  public struct MeshHeader
  {
    public uint magick;

    public int vertexCount;
    public int vertexStride;

    public int indexCount;
    public int indexStride;

    public int submeshCount;

    public float3 BboxMin;
    public float3 BboxMax;
  }

  public struct MeshVertex
  {
    public float4 pos, normal, tangent, textureСoords;
  }

  [Serializable]
  public class BaseMeta
  {
    public string guid = "";
  }
  
  public class PlatformTextureSettings
  {
      public int maxSize;
  }

  [Serializable] public class MeshMeta : BaseMeta { public bool rw; }
  [Serializable] public class TextureMeta : BaseMeta { public int maxTextureSize; public int compression; }
  [Serializable] public class ShaderTextureSlot { public string guid = ""; public string name = ""; public Vector4 scaleOffset; }
  [Serializable] public class ShaderScalarSlot { public string key = ""; public float value; }
  [Serializable] public class ShaderVectorSlot { public string key = ""; public XMLFloat4 value; }

  [Serializable]
  public class SerializablePair<K, V>
  {
    public SerializablePair(K key, V value) { this.key = key; this.value = value; }

    public K key;
    public V value;
  }

  [Serializable]
  [SuppressMessage("Build Size", "PL0001:Value types as type arguments in generics")]
  public class MaterialAsset
  {
    public string shader = "";      // guid of the shader (the engine reads it as "name or guid"), same guid as in the scene "shaders" list
    public string shaderName = "";  // human readable, the engine ignores it
    public List<ShaderTextureSlot> textures = new List<ShaderTextureSlot>();
    public List<string> properties = new List<string>();
    public List<SerializablePair<string, Vector4>> vectors = new List<SerializablePair<string, Vector4>>();
    public List<SerializablePair<string, float>> scalars = new List<SerializablePair<string, float>>();
  }

  [Serializable]
  public struct XMLFloat3
  {
    public XMLFloat3(float _x, float _y, float _z) { x = _x; y = _y; z = _z; }

    public static explicit operator XMLFloat3(float3 v) => new XMLFloat3(v.x, v.y, v.z);
    public static implicit operator XMLFloat3(Vector3 v) => new XMLFloat3(v.x, v.y, v.z);

    [XmlAttribute] public float x, y, z;
  }

  [Serializable]
  public struct XMLFloat4
  {
    public XMLFloat4(float _x, float _y, float _z, float _w) { x = _x; y = _y; z = _z; w = _w; }
    public XMLFloat4(Vector4 _v) { x = _v.x; y = _v.y; z = _v.z; w = _v.w; }
    public static explicit operator XMLFloat4(float4 v) => new XMLFloat4(v.x, v.y, v.z, v.w);
    public static explicit operator XMLFloat4(Vector4 v) => new XMLFloat4(v.x, v.y, v.z, v.w);
    public static implicit operator XMLFloat4(Quaternion v) => new XMLFloat4(v.x, v.y, v.z, v.w);

    [XmlAttribute] public float x, y, z, w;
  }

  [Serializable] public class TransformComponent
  {
    public XMLFloat3 position;
    public XMLFloat4 rotation;
    public XMLFloat3 scale;
  }

  [Serializable] public class RendererComponent
  {
    public string mesh = "";
    public string material = "";

    public List<string> materials = new List<string>(); // material guids

    public string lightmap = "";
    public XMLFloat4 lightmapScaleOffset;
  }

  [Serializable] public class LightComponent
  {
    public Color32 color;
    public XMLFloat3 dir;
    public float radius;
    public int mode; //point/spot/dir
    public float intensity;

    public int shadowMode;
  }

  [Serializable] public class ColliderComponent
  {
    public int type;           // 0=box, 1=sphere, 2=capsule, 3=mesh
    public bool isTrigger;

    public XMLFloat3 center;
    public XMLFloat3 size;       // box
    public float radius;       // sphere, capsule
    public float height;       // capsule
    public int direction;      // capsule (0=X, 1=Y, 2=Z)

    public string meshGuid;    // для MeshCollider
    public bool convex;
  }

  [Serializable] public class NodeLODS
  {
    public float screenRelativeTransitionHeight;
    public float aspect;
    public List<string> meshGuids = new();
    public List<string> materialGuids = new();
  }

  [Serializable]
  public class LODGroupComponent
  {
    public float fadeTransitionWidth = 0f;
    public bool animateCrossFading = true;
    public List<NodeLODS> lods = new();
  }

  [Serializable]
  public class SceneNode
  {
    [XmlAttribute("name")] public string name = "";
    [XmlAttribute("guid")] public string guid = "";

    // JSON (engine scene_json) is a flat list: "parent" is the guid of the parent node, none = root.
    // The tree (childs) is only used while exporting and by the XML export.
    [XmlIgnore] public string parent;

    public TransformComponent transform = new TransformComponent();
    public RendererComponent renderer { get; set; }
    public LightComponent light { get; set; }
    public LODGroupComponent lodGroup { get; set; }
    public ColliderComponent collider { get; set; }
    public TerrainComponent terrain { get; set; }

    // json: "prefab": { "guid": "<guid>", "overrides": [ { path, node } ], "added": [ { parent, nodes } ] }
    public PrefabInstanceComponent prefab { get; set; }
    public bool ShouldSerializePrefab() => prefab != null;

    // set only for disabled nodes (prefab content / overrides); scene export skips inactive objects
    [XmlAttribute("inactive"), JsonIgnore] public bool inactive;
    public bool ShouldSerializeinactive() => inactive;

    // json: node flags, 0 = disabled (the engine default is enabled, so the key is written only for disabled nodes)
    [XmlIgnore, JsonProperty("flags")] public int? flags => inactive ? 0 : (int?) null;

    [JsonIgnore] public List<SceneNode> childs; // = new List<SceneNode>();

    public bool ShouldSerializeRenderer() => renderer != null;
    public bool ShouldSerializeLight() => light != null;
    public bool ShouldSerializeLodGroup() => lodGroup != null;
    public bool ShouldSerializeCollider() => collider != null;
    public bool ShouldSerializeTerrain() => terrain != null;

    public bool ShouldSerializeChilds() => childs != null && childs.Count > 0;
  };

  [Serializable]
  public class SceneExport
  {
    // json: resources used by the scene (guids) and the flat list of nodes, parents before their childs
    // (order = the order of the counts in the engine's scene_header_t)
    public List<string> meshes = new List<string>();
    public List<string> textures = new List<string>();
    public List<string> shaders = new List<string>();
    public List<string> materials = new List<string>();
    [XmlIgnore] public List<SceneNode> nodes = new List<SceneNode>();

    // XML: the tree
    [JsonIgnore] public List<SceneNode> childs = new List<SceneNode>();
  }

  public partial class Exporter
  {
    private const string SavedPathKey = "UnityExportPath";
    private const uint MeshMagic = 1213416781; // 'M''E''S''H' fourCC
    private const int ProgressUpdateStep = 32;

    private static readonly string[] ExportSubDirs = { "mesh_src", "meshes", "textures", "lightmaps", "materials", "prefabs" };

    private static string DefaultExportPath => Path.GetFullPath(Application.dataPath + "/../!Export/");

    private static string GetSavedExportPath()
    {
      var path = EditorPrefs.GetString(SavedPathKey, "");
      return string.IsNullOrEmpty(path) ? DefaultExportPath : path;
    }

    [MenuItem("Exporter/Export scene", false, 450)]
    private static void Export() => RunExport(GetSavedExportPath(), false);

    [MenuItem("Exporter/Export scene to...", false, 451)]
    private static void ExportTo()
    {
      var path = EditorUtility.OpenFolderPanel("Export folder", GetSavedExportPath(), "");
      if (string.IsNullOrEmpty(path))
        return;

      EditorPrefs.SetString(SavedPathKey, path);
      RunExport(path, false);
    }

    [MenuItem("Exporter/Clean export scene", false, 452)]
    private static void ExportClean()
    {
      var path = GetSavedExportPath();
      if (!EditorUtility.DisplayDialog("Clean export",
            $"Delete exported meshes/textures/lightmaps/materials in\n{path}\nand export again?", "Clean & export", "Cancel"))
        return;

      RunExport(path, true);
    }

    [MenuItem("Exporter/Export scene (inline prefabs)", false, 453)]
    private static void ExportInlinePrefabs() => RunExport(GetSavedExportPath(), false, true);

    private static void RunExport(string path, bool clean, bool inlinePrefabs = false)
    {
      try
      {
        if (clean)
        {
          foreach (var sub in ExportSubDirs)
          {
            var dir = Path.Combine(path, sub);
            if (Directory.Exists(dir))
              Directory.Delete(dir, true);
          }
        }

        new Exporter(path) { _inlinePrefabs = inlinePrefabs }.ExportImpl();
      }
      catch (Exception e)
      {
        Debug.LogException(e);
      }
      finally
      {
        EditorUtility.ClearProgressBar();
      }
    }

    private readonly string _rootExportPath;
    private readonly string _rootMeshExportPath;
    private readonly string _rootRawMeshExportPath;
    private readonly string _rootTextureExportPath;
    private readonly string _rootLightmapExportPath;
    private readonly string _rootMaterialExportPath;

    private int _progressTotal;
    private int _progressDone;

    private Dictionary<Mesh, string> _exportedMeshGuids = new();
    private Dictionary<Texture, string> _exportedTextureGuids = new();
    private Dictionary<Material, string> _exportedMaterialGuids = new();
    private Dictionary<Shader, string> _exportedShaderGuids = new();

    private static void CreateDirIfNotExist(string path)
    {
      if (!Directory.Exists(path))
        Directory.CreateDirectory(path);
    }

    /// <summary>
    /// Builds destination path that mirrors the asset's source folder structure:
    /// Assets/Foo/Bar/x.png -> {rootDir}/Foo/Bar/{fileName}
    /// Assets outside of "Assets/" (Packages/, built-in) keep their first segment or go to rootDir.
    /// </summary>
    private static string GetMirroredPath(string rootDir, UnityEngine.Object asset, string fileName)
      => GetMirroredPath(rootDir, asset != null ? AssetDatabase.GetAssetPath(asset) : "", fileName);

    private static string GetMirroredPath(string rootDir, string assetPath, string fileName)
    {
      var relDir = "";
      if (!string.IsNullOrEmpty(assetPath))
      {
        assetPath = assetPath.Replace('\\', '/');
        var dir = Path.GetDirectoryName(assetPath)?.Replace('\\', '/') ?? "";
        const string assetsPrefix = "Assets";
        if (dir == assetsPrefix)
          dir = "";
        else if (dir.StartsWith(assetsPrefix + "/"))
          dir = dir.Substring(assetsPrefix.Length + 1);
        else if (dir.StartsWith("Library") || dir.StartsWith("Resources/unity_builtin"))
          dir = ""; // built-in resources

        relDir = dir;
      }

      var fullDir = string.IsNullOrEmpty(relDir)
        ? rootDir.TrimEnd('/', '\\')
        : Path.Combine(rootDir.TrimEnd('/', '\\'), relDir).Replace('\\', '/');

      CreateDirIfNotExist(fullDir);
      return $"{fullDir}/{fileName}";
    }

    /// <summary>
    /// Mesh file name: "{name}.{fileId}.mesh". One fbx may contain several sub-meshes with equal names,
    /// fileId (local id inside the asset) tells them apart. Falls back to the mesh guid for non-asset meshes.
    /// </summary>
    private static string GetMeshFileName(Mesh mesh, string guid)
    {
      var id = AssetDatabase.TryGetGUIDAndLocalFileIdentifier(mesh, out string _, out long localId)
        ? localId.ToString()
        : guid;
      return $"{mesh.name}.{id}.mesh";
    }

    /// <summary>
    /// True if dst exists and is not older than the source asset (or its .meta, where import settings live).
    /// Sources without a file on disk (generated / built-in) are considered up to date once exported.
    /// </summary>
    private static bool IsUpToDate(string srcAssetPath, string dstPath)
    {
      if (!File.Exists(dstPath))
        return false;
      if (string.IsNullOrEmpty(srcAssetPath) || !File.Exists(srcAssetPath))
        return true;

      var dstTime = File.GetLastWriteTimeUtc(dstPath);
      var srcTime = File.GetLastWriteTimeUtc(srcAssetPath);
      var metaPath = srcAssetPath + ".meta";
      if (File.Exists(metaPath))
      {
        var metaTime = File.GetLastWriteTimeUtc(metaPath);
        if (metaTime > srcTime)
          srcTime = metaTime;
      }

      return dstTime >= srcTime;
    }

    private void UpdateProgress(string title, string name)
    {
      if (_progressTotal <= 0)
        return;
      if (++_progressDone % ProgressUpdateStep != 0)
        return;

      EditorUtility.DisplayProgressBar(title, name, Mathf.Clamp01(_progressDone / (float) _progressTotal));
    }

    private Exporter(string path)
    {
      _rootExportPath = path; //Application.dataPath + "/../!Export/";

      _rootMeshExportPath = $"{_rootExportPath}/mesh_src/";
      _rootRawMeshExportPath = $"{_rootExportPath}/meshes/";
      _rootTextureExportPath = $"{_rootExportPath}/textures/";
      _rootLightmapExportPath = $"{_rootExportPath}/lightmaps/";
      _rootMaterialExportPath = $"{_rootExportPath}/materials/";

      CreateDirIfNotExist(_rootExportPath);
      CreateDirIfNotExist(_rootMeshExportPath);
      CreateDirIfNotExist(_rootRawMeshExportPath);
      CreateDirIfNotExist(_rootLightmapExportPath);
      CreateDirIfNotExist(_rootTextureExportPath);
      CreateDirIfNotExist(_rootMaterialExportPath);

      _rootPrefabExportPath = $"{_rootExportPath}/prefabs/";
      CreateDirIfNotExist(_rootPrefabExportPath);
    }

    public void ExportImpl()
    {
      var activeScene = SceneManager.GetActiveScene();
      var exportScene = new SceneExport();

      _progressTotal = 0;
      _progressDone = 0;
      for (var i = 0; i < SceneManager.sceneCount; ++i)
        foreach (var gob in SceneManager.GetSceneAt(i).GetRootGameObjects())
          if (gob.activeSelf)
            _progressTotal += gob.GetComponentsInChildren<Transform>(false).Length;

      for (var i = 0; i < SceneManager.sceneCount; ++i)
      {
        var scene = SceneManager.GetSceneAt(i);
        foreach (var gob in scene.GetRootGameObjects())
        {
          if (!gob.activeSelf)
            continue;

          var node = ExportGameObject(gob, true);
          exportScene.childs.Add(node);
        }
      }

      EditorUtility.DisplayProgressBar("Export scene", "Lightmaps...", 1f);
      SaveLightMaps();

      foreach (var root in exportScene.childs)
        Flatten(root, null, exportScene.nodes);

      // the asset lists of the scene header (engine: scene_header_t), the same four lists as in the binary scene
      AddGuids(exportScene.meshes, _exportedMeshGuids.Values);
      AddGuids(exportScene.textures, _exportedTextureGuids.Values);
      AddGuids(exportScene.shaders, _exportedShaderGuids.Values);
      AddGuids(exportScene.materials, _exportedMaterialGuids.Values);

      //  var jsonData = JsonUtility.ToJson(exportScene, true);
      var settings = new JsonSerializerSettings { NullValueHandling = NullValueHandling.Ignore, Formatting = Formatting.Indented };
      var jsonData = JsonConvert.SerializeObject(exportScene, settings);

      File.WriteAllText($"{_rootExportPath}/{activeScene.name}.json", jsonData);

      XmlSerializer serializer = new XmlSerializer(typeof(SceneExport));
      using (FileStream stream = new FileStream($"{_rootExportPath}/{activeScene.name}.xml", FileMode.Create))
      {
        serializer.Serialize(stream, exportScene);
      }
    }

    /// <summary>
    /// Tree -> flat list: a node, then its childs (depth first, so siblings keep their order and a parent is always before
    /// its childs). Sets SceneNode.parent to the guid of the parent.
    /// </summary>
    // distinct, non empty guids (an asset without a guid, e.g. a built-in one, can't be referenced anyway)
    private static void AddGuids(List<string> list, IEnumerable<string> guids)
    {
      var seen = new HashSet<string>(list);
      foreach (var guid in guids)
      {
        if (!string.IsNullOrEmpty(guid) && seen.Add(guid))
          list.Add(guid);
      }
    }

    /// <summary>
    /// Guid of a shader asset. Built-in shaders live in no file of the project (their "asset path" is a shared built-in
    /// resource, the guid is the same for all of them), so those get a stable guid made from the shader name.
    /// </summary>
    private static string GetShaderGuid(Shader shader)
    {
      var path = AssetDatabase.GetAssetPath(shader);
      var isProjectAsset = path.StartsWith("Assets/") || path.StartsWith("Packages/");
      var guid = isProjectAsset ? AssetDatabase.AssetPathToGUID(path) : "";
      return string.IsNullOrEmpty(guid) ? CreateCombinedGuid("shader:" + shader.name) : guid;
    }

    internal static void Flatten(SceneNode node, string parentGuid, List<SceneNode> output)
    {
      node.parent = string.IsNullOrEmpty(parentGuid) ? null : parentGuid;
      output.Add(node);

      if (node.childs == null)
        return;

      foreach (var child in node.childs)
        Flatten(child, node.guid, output);
    }

    private SceneNode ExportGameObject(GameObject root, bool useGlobalTransform, bool childTraverse = true, bool allowPrefabRef = true)
    {
      if (root == null)
        return new();

      var node = new SceneNode();
      UpdateProgress("Export scene", root.name);
      node.guid = GetNodeGuid(root);
      node.name = root.name;
      node.inactive = !root.activeSelf;

      if (useGlobalTransform && _relativeTo != null)
      {
        // transform in the space of the prefab instance root
        node.transform.position = _relativeTo.InverseTransformPoint(root.transform.position);
        node.transform.rotation = Quaternion.Inverse(_relativeTo.rotation) * root.transform.rotation;
      }
      else
      {
        node.transform.position = useGlobalTransform ? root.transform.position : root.transform.localPosition;
        node.transform.rotation = useGlobalTransform ? root.transform.rotation : root.transform.localRotation;
      }
      node.transform.scale = root.transform.localScale;

      // prefab instance in a scene (or nested in a prefab): store only a reference + per-instance data
      if (allowPrefabRef && TryExportPrefabInstance(root, node))
        return node;

      var components = root.GetComponents(typeof(Component));
      foreach (var component in components)
      {
        switch (component)
        {
         // case MeshFilter meshFilter: ExportMeshFilterComponent(meshFilter, node); break;
          case Renderer renderer: this.ExportRender(renderer, node);break;// ExportRendererComponent(renderer, node); break;
          case Light light: this.ExportLight(light, node); break;
          case LODGroup lodGroup: this.ExportLODGroup(lodGroup, node); break;
          case Collider collider: this.ExportCollider(collider, node); break;
        }
      }

    /*  var lodGroup0 = root.GetComponent<LODGroup>();
      if (lodGroup0 != null)
      {
        var r = lodGroup0.GetLODs()[0].renderers[0];
        this.ExportRender(r, node);
      }
      else/**/
      if (childTraverse)
      {
        for (var i = 0; i < root.transform.childCount; ++i)
        {
          node.childs ??= new();
          var gob = root.transform.GetChild(i);
          if (!gob.gameObject.activeSelf && !_includeInactive)
            continue;

          var childNode = ExportGameObject(gob.gameObject, useGlobalTransform);
          node.childs.Add(childNode);
        }
      }

      return node;
    }

    private string GetNodeGuid(GameObject go)
    {
      // objects of a loaded prefab have no usable GlobalObjectId: derive the guid from prefab guid + hierarchy path
      if (_prefabContentsRoot != null && go != null)
      {
        var path = AnimationUtility.CalculateTransformPath(go.transform, _prefabContentsRoot.transform);
        return CreateCombinedGuid($"{_currentPrefabGuid}:{path}");
      }

      return GetStableGameObjectGuid(go);
    }

    private string GetStableGameObjectGuid(GameObject go)
    {
      if (go == null)
        return string.Empty;

      GlobalObjectId globalId = GlobalObjectId.GetGlobalObjectIdSlow(go);
      return CreateCombinedGuid(globalId.ToString());
    }

    public static string CreateCombinedGuid(string data)
    {
      using MD5 md5 = MD5.Create();
      var inputBytes = Encoding.UTF8.GetBytes(data);
      var hashBytes = md5.ComputeHash(inputBytes);

      var deterministicGuid = new Guid(hashBytes);
      return deterministicGuid.ToString().Replace("-", string.Empty);
    }

    public string GetOrCreateMeshGuid(Mesh mesh)
    {
      if (mesh == null)
        return "";
      var globalObjectId = GlobalObjectId.GetGlobalObjectIdSlow(mesh);
      return CreateCombinedGuid(globalObjectId.ToString());
    }


    private void SaveMesh(Mesh mesh, string guid, string dstPath)
    {
      var metaDstPath = $"{dstPath}.jmeta";

      guid = GetOrCreateMeshGuid(mesh);

      SaveRawMesh(mesh, dstPath);

      var meta = new BaseMeta { guid = guid };
      var jsonData = JsonUtility.ToJson(meta, true);
      File.WriteAllText(metaDstPath, jsonData);
    }

    public void ExportMesh(Mesh mesh)
    {
      if (mesh == null) return;
      if (mesh.name.Contains("TextMeshPro")) return;
      if (_exportedMeshGuids.ContainsKey(mesh)) return;

      var guid = GetOrCreateMeshGuid(mesh);
      _exportedMeshGuids.TryAdd(mesh, guid);

      var meshPath = GetMirroredPath(_rootMeshExportPath, mesh, GetMeshFileName(mesh, guid));
      var metaPath = $"{meshPath}.jmeta";

      if (IsUpToDate(AssetDatabase.GetAssetPath(mesh), meshPath))
        return;

      if (!mesh.isReadable)
        Debug.LogWarning($"Mesh '{mesh.name}' is not readable (enable Read/Write in the import settings), export may fail", mesh);

      try
      {
        SaveRawMesh(mesh, meshPath);
      }
      catch (Exception e)
      {
        Debug.LogError($"Failed to export mesh '{mesh.name}': {e.Message}", mesh);
        return;
      }

      var meta = new BaseMeta { guid = guid };
      var jsonData = JsonUtility.ToJson(meta, true);
      File.WriteAllText(metaPath, jsonData);
    }

    public void ExportTexture(Texture texture)
    {
      if(_exportedTextureGuids.ContainsKey(texture))
        return;

      var path = AssetDatabase.GetAssetPath(texture);
      var guid = AssetDatabase.AssetPathToGUID(path);
      var fileName = Path.GetFileName(path);
      var dstPathOrigin = GetMirroredPath(_rootTextureExportPath, texture, fileName);
      var importer = AssetImporter.GetAtPath(path) as TextureImporter;
      if (importer == null)
        return;

      _exportedTextureGuids.TryAdd(texture, guid);
      if (File.Exists(path) && !IsUpToDate(path, dstPathOrigin))
        File.Copy(path, dstPathOrigin, true);

      var isCompressed = (importer.textureCompression == TextureImporterCompression.Uncompressed) ? 0 : 1;
      var meta = new TextureMeta { guid = guid, compression = isCompressed, maxTextureSize = importer.maxTextureSize };
      var jsonData = JsonUtility.ToJson(meta, true);
      File.WriteAllText($"{dstPathOrigin}.jmeta", jsonData);
    }

    public void SaveMaterial(Material material)
    {
      if (_exportedMaterialGuids.ContainsKey(material))
        return;

      var path = AssetDatabase.GetAssetPath(material);
      var guid = AssetDatabase.AssetPathToGUID(path);

      var materialAsset = new MaterialAsset();

      if (material.shader != null)
      {
        var shaderGuid = GetShaderGuid(material.shader);
        materialAsset.shader = shaderGuid;
        materialAsset.shaderName = material.shader.name;
        _exportedShaderGuids.TryAdd(material.shader, shaderGuid);
      }

      var textureNames = material.GetTexturePropertyNames();
      foreach (var textureName in textureNames)
      {
        var texture = material.GetTexture(textureName);
        if (texture == null)
          continue;
      
        var texturePath = AssetDatabase.GetAssetPath(texture);
        var textureGuid = AssetDatabase.AssetPathToGUID(texturePath);

        ExportTexture(texture);

        var scale = material.GetTextureScale(textureName);
        var offset = material.GetTextureOffset(textureName);
        var scaleOffset = new Vector4(offset.x, offset.y, scale.x, scale.y);
        materialAsset.textures.Add(new() {guid = textureGuid, name = textureName, scaleOffset = scaleOffset});
      }

      foreach (var propertyName in material.GetPropertyNames(MaterialPropertyType.Float))
        materialAsset.scalars.Add(new(propertyName, material.GetFloat(propertyName)));

      foreach (var propertyName in material.GetPropertyNames(MaterialPropertyType.Vector))
        materialAsset.vectors.Add(new(propertyName, material.GetVector(propertyName)));

      var fileName = string.IsNullOrEmpty(path) ? material.name : Path.GetFileNameWithoutExtension(path);
      var dstPath = GetMirroredPath(_rootMaterialExportPath, material, $"{fileName}.material");

      _exportedMaterialGuids.TryAdd(material, guid);

      if (IsUpToDate(path, dstPath))
        return;

      // save material
      var jsonData = JsonUtility.ToJson(materialAsset, true);
      File.WriteAllText(dstPath, jsonData);

      // save meta
      var meta = new BaseMeta { guid = guid };
      var metaJsonData = JsonUtility.ToJson(meta, true);
      File.WriteAllText($"{dstPath}.jmeta", metaJsonData);
    }

    private void SaveLightMaps()
    {
      for (var i = 0; i < LightmapSettings.lightmaps.Length; i++)
      {
        var lightMap = LightmapSettings.lightmaps[i].lightmapColor;
        var lightMask = LightmapSettings.lightmaps[i].shadowMask;
        if (lightMap == null)
          continue;

        var lightMapPath = AssetDatabase.GetAssetPath(lightMap);
        var lightMapGUID = AssetDatabase.AssetPathToGUID(lightMapPath);

        var lightMaskPath = AssetDatabase.GetAssetPath(lightMask);
        var lightMaskGUID = AssetDatabase.AssetPathToGUID(lightMaskPath);

        _exportedTextureGuids.TryAdd(lightMap, lightMapGUID);   // lightmaps are textures of the scene too
        var fileName = Path.GetFileName(lightMapPath);

        var dstPath = GetMirroredPath(_rootLightmapExportPath, lightMap, fileName);
        if (!IsUpToDate(lightMapPath, dstPath))
          File.Copy(lightMapPath, dstPath, true);

        File.WriteAllText($"{dstPath}.jmeta", JsonUtility.ToJson(new BaseMeta { guid = lightMapGUID }, true));
      }
    }

    private static void SaveRawMesh(Mesh mesh, string dstPath)
    {
      var vertexes = new MeshVertex[mesh.vertexCount];
      var use16BitIndex = mesh.indexFormat == IndexFormat.UInt16;

      var submeshIndexCounts = new List<int>();

      var us16 = new List<ushort>();
      var us32 = new List<int>();

      int startIndex = 0;
      for (var i = 0; i < mesh.subMeshCount; ++i)
      {
        submeshIndexCounts.Add(startIndex);
        submeshIndexCounts.Add(mesh.GetSubMesh(i).indexCount);
        startIndex += mesh.GetSubMesh(i).indexCount;
        if (use16BitIndex)
        {
          var tmp = new List<ushort>();
          mesh.GetIndices(tmp, i);
          us16.AddRange(tmp);
        }
        else
        {
          var tmp = new List<int>();
          mesh.GetIndices(tmp, i);
          us32.AddRange(tmp);
        }
      }

      var submeshes = submeshIndexCounts.ToArray();
      var indexes16 = us16.ToArray();
      var indexes32 = us32.ToArray();

      var vertexCount = mesh.vertexCount;

      var meshVertexes = mesh.vertices;
      for (var i = 0; i < vertexCount; ++i)
        vertexes[i].pos = math.float4(meshVertexes[i], 1.0f);

      // arrays are fetched only for existing attributes: every Mesh.xxx getter allocates a copy
      if (mesh.HasVertexAttribute(VertexAttribute.Normal))
      {
        var meshNormals = mesh.normals;
        for (var i = 0; i < vertexCount; ++i)
          vertexes[i].normal = math.float4(meshNormals[i], 1.0f);
      }

      if (mesh.HasVertexAttribute(VertexAttribute.Tangent))
      {
        var meshTangents = mesh.tangents;
        for (var i = 0; i < vertexCount; ++i)
          vertexes[i].tangent = meshTangents[i];
      }

      if (mesh.HasVertexAttribute(VertexAttribute.TexCoord0))
      {
        var meshTextureCoords0 = mesh.uv;
        for (var i = 0; i < vertexCount; ++i)
          vertexes[i].textureСoords.xy = meshTextureCoords0[i];
      }

      if (mesh.HasVertexAttribute(VertexAttribute.TexCoord1))
      {
        var meshTextureCoords1 = mesh.uv2;
        for (var i = 0; i < vertexCount; ++i)
          vertexes[i].textureСoords.zw = meshTextureCoords1[i];
      }

      var header = new MeshHeader
      {
        magick = MeshMagic,
        submeshCount = mesh.subMeshCount,
        vertexCount = vertexes.Length,
        vertexStride = Marshal.SizeOf(default(MeshVertex)),
        indexCount = math.max(indexes16.Length, indexes32.Length),
        indexStride = (mesh.indexFormat == IndexFormat.UInt16) ? sizeof(short) : sizeof(int),
        BboxMin = mesh.bounds.min,
        BboxMax = mesh.bounds.max,
      };

      using var stream = new FileStream(dstPath, FileMode.Create, FileAccess.Write, FileShare.None);
      using var writer = new BinaryWriter(stream);
      writer.Write(GetBytes(header));
      writer.Write(GetBytes(vertexes));
      writer.Write(use16BitIndex ? GetBytes(indexes16) : GetBytes(indexes32));
      writer.Write(GetBytes(submeshes));
    }

    private static byte[] GetBytes<T>(T structAry) where T : struct
    {
      var size = Marshal.SizeOf(default(T));
      var arr = new byte[size];

      var ptr = Marshal.AllocHGlobal(size);
      Marshal.StructureToPtr(structAry, ptr, true);
      Marshal.Copy(ptr, arr, 0, size);
      Marshal.FreeHGlobal(ptr);
      return arr;
    }

    private static byte[] GetBytes<T>(IReadOnlyList<T> structAry) where T : struct
    {
      var len = structAry.Count;
      var size = Marshal.SizeOf(default(T));
      var arr = new byte[size * len];

      var ptr = Marshal.AllocHGlobal(size);
      for (var i = 0; i < len; ++i)
      {
        Marshal.StructureToPtr(structAry[i], ptr, true);
        Marshal.Copy(ptr, arr, i * size, size);
      }

      Marshal.FreeHGlobal(ptr);
      return arr;
    }
  }
}