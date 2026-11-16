using System;
using System.Collections.Generic;
using System.IO;
using System.Xml.Serialization;
using Newtonsoft.Json;
using UnityEditor;
using UnityEngine;

// ReSharper disable InconsistentNaming
// ReSharper disable CheckNamespace

namespace UnityJsonExporter
{
  /// <summary>
  /// Per-instance data of an object that overrides something of its prefab (or has per-instance lightmap data).
  /// node.transform is relative to the prefab instance root, node has no childs.
  /// json: { "path": "A/B", "node": { ...node keys to patch... } }
  /// </summary>
  [Serializable]
  public class PrefabNodeOverride
  {
    [XmlAttribute("path")] public string path = ""; // hierarchy path from the instance root, "" = root itself
    public SceneNode node;
  }

  /// <summary> GameObject added to the instance (does not exist in the prefab asset). </summary>
  [Serializable]
  public class PrefabAddedNode
  {
    [XmlAttribute("parent"), JsonProperty("parent")] public string parentPath = ""; // hierarchy path of the parent from the instance root

    [JsonIgnore] public SceneNode node;                       // XML: the tree
    [XmlIgnore] public List<SceneNode> nodes = new();         // json: the same subtree as a flat list, the first one is its root
  }

  [Serializable]
  public class PrefabInstanceComponent
  {
    [JsonProperty("guid")] public string prefab = ""; // guid of the exported .prefab file

    public List<PrefabNodeOverride> overrides = new();
    public List<PrefabAddedNode> added = new();

    public bool ShouldSerializeoverrides() => overrides != null && overrides.Count > 0;
    public bool ShouldSerializeadded() => added != null && added.Count > 0;
  }

  /// <summary>
  /// Content of an exported .prefab file. Node transforms are in prefab space (root at identity).
  /// </summary>
  [Serializable]
  public class PrefabExport
  {
    public string guid = "";
    public string name = "";

    [JsonIgnore] public SceneNode root;
    [XmlIgnore] public List<SceneNode> nodes = new();   // flat list, the first node is the root of the prefab
  }

  public partial class Exporter
  {
    private readonly string _rootPrefabExportPath;
    private readonly HashSet<string> _exportedPrefabs = new();

    // true: prefab instances are embedded into the scene as plain nodes (set before ExportImpl)
    private bool _inlinePrefabs;

    // state of the prefab that is being exported right now
    private GameObject _prefabContentsRoot;
    private string _currentPrefabGuid = "";
    private bool _includeInactive;

    // when set, global transforms are written relative to this transform (prefab instance root)
    private Transform _relativeTo;

    /// <summary>
    /// If go is a root of a regular prefab instance: exports the prefab asset (once) and fills node.prefab.
    /// Model prefabs (fbx) are not handled here, they are exported as an ordinary hierarchy.
    /// </summary>
    private bool TryExportPrefabInstance(GameObject go, SceneNode node)
    {
      // inline mode: instances are exported as an ordinary hierarchy, no .prefab files and no references
      if (_inlinePrefabs)
        return false;

      if (!PrefabUtility.IsAnyPrefabInstanceRoot(go))
        return false;

      var assetType = PrefabUtility.GetPrefabAssetType(go);
      if (assetType != PrefabAssetType.Regular && assetType != PrefabAssetType.Variant)
        return false;

      var source = PrefabUtility.GetCorrespondingObjectFromSource(go);
      var assetPath = source != null ? AssetDatabase.GetAssetPath(source) : "";
      if (string.IsNullOrEmpty(assetPath))
        return false;

      var guid = ExportPrefab(assetPath);
      if (string.IsNullOrEmpty(guid))
        return false;

      node.prefab = new PrefabInstanceComponent { prefab = guid };
      CollectInstanceData(go, node.prefab);
      return true;
    }

    /// <summary>
    /// Exports a prefab asset as separate file prefabs/{asset folders}/{name}.prefab (+ .jmeta).
    /// Nested prefab instances inside become references too. Variants are exported flattened.
    /// Returns prefab guid, or "" on failure.
    /// </summary>
    public string ExportPrefab(string assetPath)
    {
      var guid = AssetDatabase.AssetPathToGUID(assetPath);
      if (string.IsNullOrEmpty(guid))
        return "";

      // also guards against cycles
      if (!_exportedPrefabs.Add(guid))
        return guid;

      GameObject contents = null;
      var prevRoot = _prefabContentsRoot;
      var prevGuid = _currentPrefabGuid;
      var prevInclude = _includeInactive;
      var prevRelative = _relativeTo;

      try
      {
        contents = PrefabUtility.LoadPrefabContents(assetPath);

        // the instance transform comes from the scene, prefab content is stored relative to its root
        contents.transform.SetPositionAndRotation(Vector3.zero, Quaternion.identity);
        contents.transform.localScale = Vector3.one;

        _prefabContentsRoot = contents;
        _currentPrefabGuid = guid;
        _includeInactive = true;
        _relativeTo = null;

        var root = ExportGameObject(contents, true, true, false);

        var fileName = $"{Path.GetFileNameWithoutExtension(assetPath)}.prefab";
        var dstPath = GetMirroredPath(_rootPrefabExportPath, assetPath, fileName);

        var prefabExport = new PrefabExport { guid = guid, name = contents.name, root = root };
        Flatten(root, null, prefabExport.nodes);
        var settings = new JsonSerializerSettings { NullValueHandling = NullValueHandling.Ignore, Formatting = Formatting.Indented };
        File.WriteAllText(dstPath, JsonConvert.SerializeObject(prefabExport, settings));
        File.WriteAllText($"{dstPath}.jmeta", JsonUtility.ToJson(new BaseMeta { guid = guid }, true));
      }
      catch (Exception e)
      {
        Debug.LogError($"Failed to export prefab '{assetPath}': {e}");
        _exportedPrefabs.Remove(guid);
        return "";
      }
      finally
      {
        _prefabContentsRoot = prevRoot;
        _currentPrefabGuid = prevGuid;
        _includeInactive = prevInclude;
        _relativeTo = prevRelative;

        if (contents != null)
          PrefabUtility.UnloadPrefabContents(contents);
      }

      return guid;
    }

    /// <summary>
    /// Collects what differs between the instance and its prefab asset:
    /// property overrides, per-instance lightmap data, added GameObjects.
    /// </summary>
    private void CollectInstanceData(GameObject instanceRoot, PrefabInstanceComponent component)
    {
      var rootTransform = instanceRoot.transform;

      var overridden = new HashSet<GameObject>();
      foreach (var objectOverride in PrefabUtility.GetObjectOverrides(instanceRoot, false))
      {
        var target = objectOverride.instanceObject;
        var go = target as GameObject ?? (target as Component)?.gameObject;
        if (go != null)
          overridden.Add(go);
      }

      // baked lightmaps are per instance, prefab asset does not contain them
      foreach (var renderer in instanceRoot.GetComponentsInChildren<Renderer>(true))
        if (renderer.lightmapIndex >= 0)
          overridden.Add(renderer.gameObject);

      var prevRelative = _relativeTo;
      _relativeTo = rootTransform;
      try
      {
        // GetComponentsInChildren keeps hierarchy order, so overrides are written parents first
        foreach (var t in instanceRoot.GetComponentsInChildren<Transform>(true))
        {
          if (!overridden.Contains(t.gameObject))
            continue;

          var node = ExportGameObject(t.gameObject, true, false, false);
          component.overrides.Add(new PrefabNodeOverride
          {
            path = AnimationUtility.CalculateTransformPath(t, rootTransform),
            node = node
          });
        }

        foreach (var added in PrefabUtility.GetAddedGameObjects(instanceRoot))
        {
          var go = added.instanceGameObject;
          if (go == null || go.transform.parent == null)
            continue;

          var addedNode = new PrefabAddedNode
          {
            parentPath = AnimationUtility.CalculateTransformPath(go.transform.parent, rootTransform),
            node = ExportGameObject(go, true)
          };
          Flatten(addedNode.node, null, addedNode.nodes);
          component.added.Add(addedNode);
        }
      }
      finally
      {
        _relativeTo = prevRelative;
      }
    }
  }
}
