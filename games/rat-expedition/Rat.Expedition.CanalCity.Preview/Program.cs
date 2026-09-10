using System.Text.Json;
using Stride.Animations;
using Stride.Core.Mathematics;
using Stride.Engine;
using Stride.Games;
using Stride.Graphics;
using Stride.Rendering;
using Stride.Rendering.Compositing;

string evidence=Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"RatExpedition","canal-city-preview");
int width=1280,height=720,smokeFrames=300;
try
{
    for(int i=0;i<args.Length;i++)
    {
        if(i+1>=args.Length)throw new ArgumentException("Expected option value.");
        string key=args[i++],value=args[i];
        switch(key)
        {
            case "--evidence-dir":evidence=Path.GetFullPath(value);break;
            case "--width":width=int.Parse(value);break;
            case "--height":height=int.Parse(value);break;
            case "--smoke-frames":smokeFrames=int.Parse(value);break;
            default:throw new ArgumentException("Unknown option "+key);
        }
    }
    if(width<640||width>3840||height<480||height>2160||smokeFrames<260||smokeFrames>1200)
        throw new ArgumentException("Width 640..3840, height 480..2160, smoke frames 260..1200 required.");
    Directory.CreateDirectory(evidence);
    if(File.Exists(Path.Combine(evidence,"native-preview.json")))throw new IOException("Choose a fresh evidence directory.");
    using var game=new PreviewGame(evidence,width,height,smokeFrames);
    game.Run();
    if(game.Failure is not null)throw game.Failure;
    if(!game.Completed)throw new InvalidDataException("Preview closed before qualification completed.");
    return 0;
}
catch(Exception error)
{
    Directory.CreateDirectory(evidence);
    File.WriteAllText(Path.Combine(evidence,"error.log"),error.ToString());
    return 1;
}

// Isolated native asset qualification. This does not replace the game's A1
// loader, add gameplay colliders or claim Game Studio/MCP reimport acceptance.
sealed class PreviewGame(string evidence,int width,int height,int smokeFrames):Game
{
    static readonly string[] ModelIds=["bridge_arch","canal_wall","door_standard","lantern_amber","railing_high","railing_iron","scale_rat_adult","scale_rat_high","scale_rat_medium","scale_rat_small","stairs_medium","stairs_paired"];
    static readonly JsonSerializerOptions JsonOptions=new(){WriteIndented=true};
    readonly List<object> poses=[];
    readonly Dictionary<string,Quaternion> rotations=[];
    readonly Dictionary<string,Matrix> leafMatrices=[];
    readonly Dictionary<string,Matrix> frameMatrices=[];
    Scene scene=null!;
    Entity camera=null!;
    ModelComponent door=null!;
    AnimationComponent animation=null!;
    PlayingAnimation playing=null!;
    int frames,hingeIndex,leafIndex,frameIndex;
    bool ready;
    object[] models=[];
    public Exception? Failure{get;private set;}
    public bool Completed{get;private set;}

    protected override async Task LoadContent()
    {
        try
        {
            await base.LoadContent();
            scene=Content.Load<Scene>("CanalCity/PreviewScene");
            SceneSystem.SceneInstance=new SceneInstance(Services,scene);
            SceneSystem.GraphicsCompositor=Content.Load<GraphicsCompositor>("CanalCity/PreviewCompositor");
            camera=scene.Entities.Single(e=>e.Name=="Camera");
            Aim(camera,new Vector3(-21,18,30),new Vector3(0,2,0));
            scene.Entities.Single(e=>e.Name=="Key").Transform.Rotation=Quaternion.RotationYawPitchRoll(-.65f,-.8f,0);
            scene.Entities.Single(e=>e.Name=="Fill").Transform.Rotation=Quaternion.RotationYawPitchRoll(2.4f,-.4f,0);
            GraphicsDeviceManager.PreferredBackBufferWidth=width;
            GraphicsDeviceManager.PreferredBackBufferHeight=height;
            GraphicsDeviceManager.ApplyChanges();
            Window.Title="Dremma | native CanalCity preview qualification";
            WindowMinimumUpdateRate.SetMaxFrequency(60);MinimizedMinimumUpdateRate.SetMaxFrequency(60);DrawWhileMinimized=true;
            door=scene.Entities.Single(e=>e.Name=="door_standard").Get<ModelComponent>();
            animation=door.Entity.Get<AnimationComponent>();
            if(animation.Animations.Count!=2)throw new InvalidDataException("Two native door clips required.");
            foreach(var clip in animation.Animations.Values)
                if(Math.Abs(clip.Duration.TotalSeconds-2)>.001)throw new InvalidDataException("Door duration must be two seconds.");
            hingeIndex=Array.FindIndex(door.Skeleton.Nodes,n=>n.Name=="door_hinge.001");
            leafIndex=Array.FindIndex(door.Skeleton.Nodes,n=>n.Name=="door_standard_leaf");
            frameIndex=Array.FindIndex(door.Skeleton.Nodes,n=>n.Name=="door_standard_frame");
            if(hingeIndex<0||leafIndex<0||frameIndex<0)throw new InvalidDataException("Importer-observed door nodes were lost.");
            if(door.Model.Meshes.Where(m=>m.NodeIndex==leafIndex).Count()==0)throw new InvalidDataException("Leaf mesh has no skeleton node binding.");
            foreach(string id in ModelIds)
            {
                var prefab=Content.Load<Prefab>("CanalCity/"+id+"_prefab");
                if(prefab.Entities.Count!=1||prefab.Entities[0].Get<ModelComponent>()?.Model is null)
                    throw new InvalidDataException("Native prefab missing model: "+id);
            }
            SetPose("open",0);
            ready=true;
        }
        catch(Exception error){Failure=error;Exit();}
    }

    static void Aim(Entity entity,Vector3 position,Vector3 target)
    {
        entity.Transform.Position=position;
        var world=Matrix.Invert(Matrix.LookAtRH(position,target,Vector3.UnitY));
        entity.Transform.Rotation=Quaternion.RotationMatrix(world);
    }
    void SetPose(string clip,double seconds)
    {
        animation.PlayingAnimations.Clear();
        playing=animation.Play(clip);
        playing.RepeatMode=AnimationRepeatMode.PlayOnceHold;
        playing.TimeFactor=0;
        playing.CurrentTime=TimeSpan.FromSeconds(seconds);
    }
    protected override void Update(GameTime gameTime)
    {
        try
        {
            if(ready)
            {
                switch(frames)
                {
                    case 80:Aim(camera,new Vector3(-2.7f,3.0f,9.5f),new Vector3(1,1.5f,3.5f));SetPose("open",0);break;
                    case 110:SetPose("open",1);break;
                    case 140:SetPose("open",2);break;
                    case 170:SetPose("close",0);break;
                    case 200:SetPose("close",1);break;
                    case 230:SetPose("close",2);break;
                }
            }
            base.Update(gameTime);
        }
        catch(Exception error){Failure=error;Exit();}
    }
    protected override void EndDraw(bool present)
    {
        try
        {
            if(ready)
            {
                frames++;
                if(frames==60){Capture("overview");ObserveModels();}
                string? sample=frames switch{100=>"door-closed",130=>"door-middle",160=>"door-open",190=>"close-start",220=>"close-middle",250=>"close-end",_=>null};
                if(sample is not null){Capture(sample);ObservePose(sample);}
                if(frames==260)Qualify();
                if(frames>=smokeFrames)Exit();
            }
            base.EndDraw(present);
        }
        catch(Exception error){Failure=error;Exit();}
    }
    void Capture(string name)
    {
        var buffer=GraphicsDevice.Presenter.BackBuffer;
        if(buffer.Width!=width||buffer.Height!=height)throw new InvalidDataException("Unexpected backbuffer resolution.");
        using var stream=File.Create(Path.Combine(evidence,name+".png"));
        buffer.Save(GraphicsContext.CommandList,stream,ImageFileType.Png);
    }
    static float[] Vec(Vector3 v)=>[v.X,v.Y,v.Z];
    static float[] Quat(Quaternion q)=>[q.X,q.Y,q.Z,q.W];
    static float[] Mat(Matrix m)=>[m.M11,m.M12,m.M13,m.M14,m.M21,m.M22,m.M23,m.M24,m.M31,m.M32,m.M33,m.M34,m.M41,m.M42,m.M43,m.M44];
    void ObserveModels()
    {
        var rows=new List<object>();
        foreach(string id in ModelIds)
        {
            var model=scene.Entities.Single(e=>e.Name==id).Get<ModelComponent>();
            if(model.Model.Meshes.Count==0||model.GetMaterialCount()==0||model.Model.Materials.Any(m=>m.Material is null))throw new InvalidDataException("Missing native mesh/material: "+id);
            var size=model.BoundingBox.Maximum-model.BoundingBox.Minimum;
            if(!float.IsFinite(size.Length())||size.X<=0||size.Y<=0||size.Z<=0)throw new InvalidDataException("Invalid native bounds: "+id);
            float expected=id switch{"scale_rat_small"=>1.2f,"scale_rat_medium"=>1.8f,"scale_rat_adult"=>2.8f,"scale_rat_high"=>4,"bridge_arch"=>5,"canal_wall"=>4.5f,"lantern_amber"=>.75f,_=>size.Y};
            if(Math.Abs(size.Y-expected)>.01)throw new InvalidDataException($"Wrong native metre/up-axis for {id}: {size.Y}, expected {expected}");
            rows.Add(new{id,meshes=model.Model.Meshes.Count,materials=model.GetMaterialCount(),dimensionsMetres=Vec(size),boundsMin=Vec(model.BoundingBox.Minimum),boundsMax=Vec(model.BoundingBox.Maximum),nodes=model.Skeleton.Nodes.Select((n,i)=>new{index=i,n.Name,n.ParentIndex}),meshNodes=model.Model.Meshes.Select(m=>m.NodeIndex).Distinct()});
        }
        models=rows.ToArray();
    }
    void ObservePose(string name)
    {
        var transform=door.Skeleton.NodeTransformations[hingeIndex];
        rotations[name]=transform.Transform.Rotation;
        leafMatrices[name]=door.Skeleton.NodeTransformations[leafIndex].WorldMatrix;
        frameMatrices[name]=door.Skeleton.NodeTransformations[frameIndex].WorldMatrix;
        poses.Add(new{name,clip=playing.Name,timeSeconds=playing.CurrentTime.TotalSeconds,durationSeconds=playing.Clip.Duration.TotalSeconds,hingeIndex,localRotation=Quat(transform.Transform.Rotation),localTranslation=Vec(transform.Transform.Position),worldHinge=Mat(transform.WorldMatrix),worldLeaf=Mat(leafMatrices[name]),worldFrame=Mat(frameMatrices[name])});
    }
    void Qualify()
    {
        double Difference(Quaternion a,Quaternion b)=>2*Math.Acos(Math.Clamp(Math.Abs(Quaternion.Dot(a,b)),0,1))*180/Math.PI;
        var neutral=rotations["door-closed"];
        var expected=new Dictionary<string,double>{{"door-closed",0},{"door-middle",50},{"door-open",100},{"close-start",100},{"close-middle",50},{"close-end",0}};
        var angles=rotations.ToDictionary(x=>x.Key,x=>Difference(neutral,x.Value));
        foreach(var pair in expected)
            if(Math.Abs(angles[pair.Key]-pair.Value)>.1)throw new InvalidDataException($"Native pose {pair.Key}: {angles[pair.Key]} degrees, expected {pair.Value}");
        if(Mat(leafMatrices["door-closed"]).Zip(Mat(leafMatrices["door-open"]),(a,b)=>Math.Abs(a-b)).Max()<.1)throw new InvalidDataException("Leaf mesh node did not move.");
        foreach(var matrix in frameMatrices.Values)
            if(Mat(matrix).Zip(Mat(frameMatrices["door-closed"]),(a,b)=>Math.Abs(a-b)).Max()>.0001)throw new InvalidDataException("Fixed masonry frame moved with door.");
        File.WriteAllText(Path.Combine(evidence,"native-preview.json"),JsonSerializer.Serialize(new{status="passed",qualification="isolated compiled native preview, not A1/game/editor acceptance",resolution=new{width,height},modelCount=models.Length,prefabCount=12,clipCount=2,models,poses,relativeHingeAnglesDegrees=angles,stride=typeof(Game).Assembly.GetName().Version?.ToString(),graphics=GraphicsDevice.Platform.ToString()},JsonOptions));
        Completed=true;
    }
}
