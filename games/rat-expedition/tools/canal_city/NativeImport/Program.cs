using System.Text.Json;
using System.Security.Cryptography;
using Stride.Assets.Models;
using Stride.Assets.Materials;
using Stride.Assets.Textures;
using Stride.Core.Assets;
using Stride.Core.Diagnostics;
using Stride.Core.IO;

if(args.Length!=2)throw new ArgumentException("NativeImport <CanalCity resources> <new report.json>");
string root=Path.GetFullPath(args[0]),report=Path.GetFullPath(args[1]);
if(File.Exists(report))throw new IOException("Choose a new import report path.");
var logger=new LoggerResult();
var importer=new ThreeDAssetImporter();
// Geometry/hierarchy come from the real importer. Textures and PBR materials
// are authored separately from materials.json, not guessed from FBX shaders.
var parameters=new AssetImporterParameters(typeof(ModelAsset),typeof(SkeletonAsset),typeof(AnimationAsset)){Logger=logger};
var files=Directory.GetFiles(Path.Combine(root,"models"),"*.fbx").Concat(Directory.GetFiles(Path.Combine(root,"animations"),"*.fbx")).Order().ToArray();
var output=new List<object>();
foreach(string file in files)
{
    var info=importer.GetEntityInfo(new UFile(file),logger,parameters)??throw new InvalidDataException("No entity information: "+file);
    var imported=importer.Import(new UFile(file),parameters).ToList();
    var skeleton=imported.Select(x=>x.Asset).OfType<SkeletonAsset>().Single();
    var animations=imported.Select(x=>x.Asset).OfType<AnimationAsset>().ToArray();
    output.Add(new {file=Path.GetRelativePath(root,file).Replace('\\','/'),sha256=Convert.ToHexStringLower(SHA256.HashData(File.ReadAllBytes(file))),materials=info.Materials.Keys.ToArray(),meshes=info.Models.Select(x=>new {x.MeshName,x.MaterialName,x.NodeName}),nodes=skeleton.Nodes.Select(x=>new{x.Name,x.Depth,x.Preserve}),animations=animations.Select(x=>new{x.AnimationStack,start=x.AnimationTimeMinimum.TotalSeconds,end=x.AnimationTimeMaximum.TotalSeconds})});
}
if(logger.HasErrors)throw new InvalidDataException(logger.ToText());
Directory.CreateDirectory(Path.GetDirectoryName(report)!);
File.WriteAllText(report,JsonSerializer.Serialize(new {qualification="Stride ThreeDAssetImporter.GetEntityInfo and Import",version=typeof(ThreeDAssetImporter).Assembly.GetName().Version?.ToString(),files=output},new JsonSerializerOptions{WriteIndented=true}));
Console.WriteLine(report);
