using System.Text.Json;
using Rat.Expedition.Authoring;
using Rat.Expedition.Core;
using Stride.Core.Mathematics;
using Stride.Engine;
using Stride.Engine.Processors;
using Stride.Engine.Design;
using Stride.Games;
using Stride.Graphics;
using Stride.Input;
using Stride.Rendering;
using Stride.Rendering.Colors;
using Stride.Rendering.Compositing;
using Stride.Rendering.Lights;
using Stride.Rendering.Materials;
using Stride.Rendering.Materials.ComputeColors;
using Stride.Rendering.ProceduralModels;
using Stride.Rendering.Sprites;

namespace Rat.Expedition.Windows;

public sealed class ExpeditionGame : Game
{
    private readonly GameOptions options;
    private ExpeditionProject project = null!;
    private ExpeditionSession session = null!;
    private SceneDefinition definition => session.Scene;
    private ScenePresentation bundle = null!;
    private readonly List<ScenePresentation> retired = [];
    private SpriteSheet sheet = null!;
    private CameraComponent cameraTemplate = null!;
    private readonly List<object> samples = [];
    private Entity hero => bundle.Actors[0];
    private Texture? atlas;
    private SpriteFromSheet provider => bundle.Providers[0];
    private int frames;
    private bool contentReady;
    private SmokeWindowEvidence? smokeWindowEvidence;
    private int drawnFrames;
    private bool deliberateFocusProbe;
    private bool discardInputOnNextUpdate;
    private bool? focusProbePassed;
    private bool? focusZoomProbePassed;
    private int ladderPauseFrame=-1;
    private long ladderPauseTicks;
    private bool? ladderPausePassed;
    private CameraComponent camera => bundle.Camera;
    private Entity cameraEntity => bundle.CameraEntity;
    internal static readonly Vector3 CameraOffset = new(27, 31.5f, 27);
    internal const float SpritePixelsPerUnit = 60f/2.25f;
    internal static readonly float UprightScale = CameraOffset.Length() / new Vector2(CameraOffset.X, CameraOffset.Z).Length();
    private readonly List<object> cameraSamples = [];
    private readonly BodySmokeRoute bodyRoute = new();
    private readonly SessionSmokeRoute sessionRoute;
    private SpriteFont? hudFont;
    private SpriteBatch? hudBatch;
    public Exception? FatalError { get; private set; }

    public ExpeditionGame(GameOptions options)
    {
        this.options = options;
        sessionRoute=new(options.SmokeRoute is "native-candidate-failure" or "native-resource-failure"?"portal-failure":options.SmokeRoute);
        AutoLoadDefaultSettings = false;
        GraphicsDeviceManager.PreferredBackBufferWidth = options.Width;
        GraphicsDeviceManager.PreferredBackBufferHeight = options.Height;
        GraphicsDeviceManager.SynchronizeWithVerticalRetrace = options.SmokeFrames == 0;
        if (options.SmokeFrames > 0)
        {
            WindowMinimumUpdateRate.SetMaxFrequency(60);
            MinimizedMinimumUpdateRate.SetMaxFrequency(60);
            DrawWhileMinimized = true;
        }
        IsMouseVisible = true;
    }

    protected override async Task LoadContent()
    {
        try { await LoadScene(); contentReady = true; if(options.SmokeFrames>0)smokeWindowEvidence=new(); }
        catch (Exception error) { FatalError = error; Exit(); }
    }

    protected override void Initialize()
    {
        if (Settings is not null) { Settings.EffectCompilation = EffectCompilationMode.Local; Settings.RecordUsedEffects = false; }
        base.Initialize();
    }

    protected override void OnDeactivated(object sender, EventArgs args)
    {
        if(options.SmokeFrames>0&&!deliberateFocusProbe){base.OnDeactivated(sender,args);return;}
        // Stride skips Update while inactive; pause/flush the session in the callback.
        session?.Advance(0, new(System.Numerics.Vector2.Zero), false);
        discardInputOnNextUpdate = true;
        base.OnDeactivated(sender, args);
    }

    protected override void OnActivated(object sender, EventArgs args)
    {
        if(options.SmokeFrames>0&&!deliberateFocusProbe){base.OnActivated(sender,args);return;}
        ResetElapsedTime();
        discardInputOnNextUpdate = true;
        base.OnActivated(sender, args);
    }

    private async Task LoadScene()
    {
        await base.LoadContent();
        project = new NativeProjectLoader(Services,options.NativeProject).Load();
        if(options.SmokeFrames>0&&options.SmokeRoute=="native-candidate-failure")
        {
            // The initial world uses the production assets. A later real portal
            // requests a separately compiled invalid native candidate, exercising
            // loader/validation failure before any presentation replacement.
            var initial=project;int reads=0;
            project=ExpeditionProject.FromSource(initial.StartScene,initial.Scenes.Values,id=>
                ++reads==1?initial.LoadCandidate(id):new NativeProjectLoader(Services,"QA/bad-portal-target").Load().LoadCandidate(id));
        }
        if(options.SmokeFrames>0)
            File.WriteAllText(Path.Combine(options.EvidenceDirectory,"native-world.json"),JsonSerializer.Serialize(new {
                source="compiled native assets",asset=options.NativeProject,project.StartScene,scenes=project.Scenes.Values,
                names=project.Scenes.Values.ToDictionary(s=>s.Id,s=>s.DisplayNames),
                wallProbes=project.Scenes.Values.ToDictionary(s=>s.Id,s=>new {
                    oldPositionFree=s.CreateWorld().HasClearance(new(0,0,0),TraversalMotor.Radius,TraversalMotor.StandingHeight),
                    shiftedPositionFree=s.CreateWorld().HasClearance(new(0,0,-3.375f),TraversalMotor.Radius,TraversalMotor.StandingHeight),
                    oldSweep=s.CreateWorld().SweepFraction(new(-6.75f,0,0),new(6.75f,0,0),TraversalMotor.Radius,TraversalMotor.StandingHeight),
                    shiftedSweep=s.CreateWorld().SweepFraction(new(-6.75f,0,-3.375f),new(6.75f,0,-3.375f),TraversalMotor.Radius,TraversalMotor.StandingHeight)})
            },new JsonSerializerOptions{WriteIndented=true}));
        Window.Title = "Rat Expedition | WASD | Ctrl | E / Enter | Esc: pause | Wheel: zoom";
        var fontSystem = (Stride.Graphics.Font.FontSystem)Font;
        fontSystem.RuntimeFonts.RegisterFont("RatNoto",Path.Combine(options.ContentDirectory,"fonts","NotoSans-Regular.ttf"));
        hudFont = fontSystem.LoadRuntimeFont("RatNoto",20f) ?? throw new InvalidDataException("Cannot load RatNoto font.");
        hudBatch = new SpriteBatch(GraphicsDevice);
        cameraTemplate = new CameraComponent { Projection = CameraProjectionMode.Orthographic, OrthographicSize = options.CameraSize };
        SceneSystem.GraphicsCompositor = GraphicsCompositorHelper.CreateDefault(false, camera: cameraTemplate, clearColor: new Color4(.065f,.08f,.10f,1));
        string png = Path.Combine(options.ContentDirectory, "sprites", "rat.png");
        using (var stream = File.OpenRead(png)) atlas = Texture.Load(GraphicsDevice, stream);
        if (atlas.Width != 64 || atlas.Height != 192) throw new InvalidDataException("Rat PNG must be the 64x192 eight-frame atlas.");
        sheet = new SpriteSheet();
        for (int row = 0; row < 4; row++)
        for (int column = 0; column < 2; column++)
            sheet.Sprites.Add(new Sprite(atlas) { Name = $"rat-{row}-{column}", Region = new RectangleF(column * 32, row * 48, 32, 48), Center = new Vector2(16, 43), PixelsPerUnit = new Vector2(SpritePixelsPerUnit), IsTransparent = true });
        session = new ExpeditionSession(project, PrepareScene);
        FollowHero();
    }

    private IPreparedSceneChange PrepareScene(SceneChangeCandidate candidate)
    {
        if(bundle is not null) cameraTemplate.OrthographicSize = camera.OrthographicSize;
        retired.EnsureCapacity(retired.Count+1);
        string visualProject=options.SmokeRoute=="native-resource-failure"&&candidate.Reason==SceneChangeReason.Portal?"QA/bad-native-binding":options.NativeProject;
        var next = ScenePresentation.Prepare(this,candidate,sheet,cameraTemplate,visualProject);
        if(options.SmokeFrames>0&&options.SmokeRoute=="portal-failure"&&candidate.Reason==SceneChangeReason.Portal)
        {
            next.Dispose();throw new InvalidDataException("Diagnostic renderer candidate rejected after resource preparation.");
        }
        return new PreparedChange(() => {
            if(bundle is not null) retired.Add(bundle);
            bundle = next; SceneSystem.SceneInstance = next.Instance;
        }, next);
    }
    private sealed class PreparedChange(Action activate,ScenePresentation candidate):IPreparedSceneChange
    {
        private bool activated;
        public void Activate(){activate();activated=true;}
        public void Dispose(){if(!activated)candidate.Dispose();}
    }

    private void FollowHero()
    {
        static float ClampAim(float value, float min, float max)
        {
            float inset = Math.Min(3.375f, (max - min) / 2);
            return Math.Clamp(value, min + inset, max - inset);
        }
        cameraEntity.Transform.Position = CameraOffset + new Vector3(
            ClampAim(session.Leader.Position.X, definition.Floor.Min.X, definition.Floor.Max.X), session.Leader.Position.Y + 1.35f,
            ClampAim(session.Leader.Position.Z, definition.Floor.Min.Z, definition.Floor.Max.Z));
    }

    private void UpdatePartyAndOcclusion(double elapsed)
    {
        cameraEntity.Transform.UpdateWorldMatrix();
        camera.Update((float)GraphicsDevice.Presenter.BackBuffer.Width/GraphicsDevice.Presenter.BackBuffer.Height);
        var leader=session.Leader;
        var rays=new List<SightSegment>();
        foreach(float height in new[]{.03f,leader.BodyHeight*.5f,leader.BodyHeight*.95f})
        {
            var target=new Vector3(leader.Position.X,leader.Position.Y+height,leader.Position.Z);
            var projected=Vector3.TransformCoordinate(target,camera.ViewProjectionMatrix);
            var inverse=Matrix.Invert(camera.ViewProjectionMatrix);
            var near=Vector3.TransformCoordinate(new Vector3(projected.X,projected.Y,0),inverse);
            var far=Vector3.TransformCoordinate(new Vector3(projected.X,projected.Y,1),inverse);
            var ray=Vector3.Normalize(far-near);
            var end=near+ray*Vector3.Dot(target-near,ray);
            rays.Add(new(new(near.X,near.Y,near.Z),new(end.X,end.Y,end.Z)));
        }
        bundle.Occlusion.Update(leader,rays,session.Mode==SessionMode.Explore?elapsed:0);
        bundle.ApplyOcclusion(definition,bundle.Occlusion.Hidden);
        var companions=session.Trail.Companions;
        for(int i=0;i<companions.Count;i++)
        {
            var pose=companions[i];var actor=bundle.Actors[i+1];var before=actor.Transform.Position;
            actor.Transform.Position=new(pose.Position.X,pose.Position.Y,pose.Position.Z);
            actor.Transform.Scale=new(1,UprightScale*(pose.Stance==BodyStance.Crouched?.5f:1),1);
            actor.Get<SpriteComponent>().Enabled=!bundle.Occlusion.HidesCompanion(pose);
            var delta=actor.Transform.Position-before;
            if(delta.LengthSquared()>.000001f)
            {
                var move=new System.Numerics.Vector2(delta.X,delta.Z);
                float x=System.Numerics.Vector2.Dot(move,TraversalMotor.CameraRight),y=System.Numerics.Vector2.Dot(move,TraversalMotor.CameraForward);
                bundle.Providers[i+1].CurrentFrame=frames/12%2+(Math.Abs(x)>Math.Abs(y)?(x>0?4:2):y>0?6:0);
            }
        }
    }

    protected override void Draw(GameTime gameTime)
    {
        base.Draw(gameTime);
        if(!contentReady || hudBatch is null || hudFont is null) return;
        drawnFrames++;
        GraphicsContext.CommandList.SetRenderTargetAndViewport(GraphicsDevice.Presenter.DepthStencilBuffer,GraphicsDevice.Presenter.BackBuffer);
        hudBatch.Begin(GraphicsContext,depthStencilState:DepthStencilStates.None);
        string text = session.Snapshot.Hint;
        hudBatch.DrawString(hudFont,text,new Vector2(25,25),Color.Black);
        hudBatch.DrawString(hudFont,text,new Vector2(24,24),Color.White);
        hudBatch.End();
    }

    protected override void Destroy()
    {
        SceneSystem.SceneInstance = null;
        if(bundle is not null)bundle.Dispose();
        foreach(var old in retired)old.Dispose();retired.Clear();
        hudBatch?.Dispose(); hudFont?.Dispose(); atlas?.Dispose();
        base.Destroy();
    }

    protected override void Update(GameTime gameTime)
    {
        if (!contentReady) { base.Update(gameTime); return; }
        var beforeFocus = session.Leader.Position;
        float zoomBeforeFocus = camera.OrthographicSize;
        long ticksBeforeFocus = session.Ticks;
        bool probeFocus = options.SmokeFrames > 120 && frames == 120;
        if (probeFocus)
        {
            deliberateFocusProbe=true;
            try { OnDeactivated(this, EventArgs.Empty); OnActivated(this, EventArgs.Empty); }
            finally { deliberateFocusProbe=false; }
        }
        var input = new System.Numerics.Vector2(
            (Input.IsKeyDown(Keys.D) ? 1 : 0) - (Input.IsKeyDown(Keys.A) ? 1 : 0),
            (Input.IsKeyDown(Keys.W) ? 1 : 0) - (Input.IsKeyDown(Keys.S) ? 1 : 0));
        if (options.SmokeFrames > 0)
            input = frames < 90 ? new(0, 1) : frames < 240 ? new(1, 0) : frames < 300 ? new(0, 1) : frames < 360 ? new(-1, 0) : System.Numerics.Vector2.Zero;
        if (options.SmokeFrames > 0 && options.SmokeRoute == "edges")
            input = frames < 405 ? new(0, -1) : frames < 945 ? new(1, -1) : frames < 1620 ? new(1, 1) : frames < 2430 ? new(-1, 1) : new(-1, -1);
        if (options.SmokeFrames > 0 && options.SmokeRoute == "zoom") input = System.Numerics.Vector2.Zero;
        var command = new TraversalInput(input,Input.IsKeyDown(Keys.LeftCtrl)||Input.IsKeyDown(Keys.RightCtrl),Input.IsKeyDown(Keys.E)||Input.IsKeyDown(Keys.Enter));
        if(options.SmokeFrames>0) command = options.SmokeRoute=="body" && session.Mode==SessionMode.Explore ? bodyRoute.Next(session.Leader) : new(input);
        double elapsed = options.SmokeFrames > 0 ? (options.SmokeRoute is "walls" or "zoom" or "window-states" ? 1.0 / 60 : 1.0 / 30) : gameTime.Elapsed.TotalSeconds;
        float wheel = Input.MouseWheelDelta;
        if (options.SmokeFrames > 0) wheel = options.SmokeRoute == "zoom" ? frames switch { 30 or 120 or 150 => 100, 90 or 210 => -100, 270 => 4, _ => 0 } : 0;
        bool discard = discardInputOnNextUpdate;
        if (discard) { elapsed = 0; command = command with {Move=System.Numerics.Vector2.Zero}; wheel = 0; discardInputOnNextUpdate = false; }
        if (IsActive || options.SmokeFrames > 0) camera.OrthographicSize = Math.Clamp(camera.OrthographicSize - wheel * 1.125f, 10.125f, 15.75f);
        var sessionCommand = new SessionInput(command.Move,command.CrouchHeld,command.InteractHeld,Input.IsKeyDown(Keys.Escape));
        if(options.SmokeFrames>0 && options.SmokeRoute is "layered" or "mixed" or "portals" or "portal-failure" or "native-candidate-failure" or "native-resource-failure" or "recovery" && !discard && session.Mode==SessionMode.Explore)
            sessionCommand=sessionRoute.Next(session);
        if(options.SmokeFrames>0 && frames is 121 or 123)sessionCommand=new(System.Numerics.Vector2.Zero);
        if(options.SmokeFrames>0 && frames==122)sessionCommand=new(System.Numerics.Vector2.Zero,PauseHeld:true);
        if(options.SmokeFrames>0&&options.SmokeRoute=="body"&&session.Leader.Mode==TraversalMode.Climbing&&session.Leader.Position.Y>1.6875f&&ladderPauseFrame<0)
        {ladderPauseFrame=0;ladderPauseTicks=session.Ticks;}
        if(ladderPauseFrame is >=0 and <16)
            sessionCommand=new(new(0,1),InteractHeld:true,PauseHeld:ladderPauseFrame is 0 or 14);
        session.Advance(elapsed,sessionCommand,options.SmokeFrames>0||IsActive);
        if(ladderPauseFrame is >=0 and <16)
        {
            if(ladderPauseFrame==0)sessionRoute.Capture="ladder-pause-start";
            if(ladderPauseFrame==12){ladderPausePassed=session.Ticks==ladderPauseTicks&&session.Mode==SessionMode.Paused;sessionRoute.Capture="ladder-paused";}
            if(ladderPauseFrame==15)sessionRoute.Capture="ladder-resumed";
            ladderPauseFrame++;
        }
        foreach(var old in retired)old.Dispose();retired.Clear();
        if (probeFocus) focusProbePassed = session.Leader.Position == beforeFocus && session.Ticks == ticksBeforeFocus && session.Mode==SessionMode.Paused;
        if (probeFocus) focusZoomProbePassed = camera.OrthographicSize == zoomBeforeFocus;
        if (hero is not null) { hero.Transform.Position = new(session.Leader.Position.X, session.Leader.Position.Y, session.Leader.Position.Z); hero.Transform.Scale = new(1,UprightScale*(session.Leader.Stance==BodyStance.Crouched?.5f:1),1); }
        FollowHero();
        UpdatePartyAndOcclusion((session.Ticks-ticksBeforeFocus)*TraversalMotor.StepSeconds);
        if(options.SmokeFrames>0)sessionRoute.Observe(session,bundle.Occlusion);
        input = sessionCommand.Move;
        if (session.Mode==SessionMode.Explore && input.LengthSquared() > 0) provider.CurrentFrame = frames / 12 % 2 + (Math.Abs(input.X) > Math.Abs(input.Y) ? (input.X > 0 ? 4 : 2) : input.Y > 0 ? 6 : 0);
        if (options.SmokeFrames > 0 && frames % 30 == 0) samples.Add(new { frame = frames, x = session.Leader.Position.X, y = session.Leader.Position.Y, z = session.Leader.Position.Z, session.Ticks, snapshot=session.Leader });
        base.Update(gameTime);
    }

    protected override void EndDraw(bool present)
    {
        // Minimized/hidden smoke still draws/captures the backbuffer; an occluded
        // desktop swap-chain Present must not become a second timing policy.
        if(options.SmokeFrames>0&&(Window.IsMinimized||!Window.Visible))present=false;
        if (!contentReady) { base.EndDraw(present); return; }
        frames++;
        if(options.SmokeFrames>0&&frames%30==0)smokeWindowEvidence!.Record(this,frames,drawnFrames,session.Ticks,options.EvidenceDirectory);
        if(options.SmokeFrames>0 && sessionRoute.Capture is string sessionMilestone)
        {
            using var capture=File.Create(Path.Combine(options.EvidenceDirectory,$"session-{sessionMilestone}.png"));
            GraphicsDevice.Presenter.BackBuffer.Save(GraphicsContext.CommandList,capture,ImageFileType.Png);
            sessionRoute.Milestones.Add(new {name=sessionMilestone,frame=frames,session=session.Snapshot,companions=session.Trail.Companions,
                hidden=bundle.Occlusion.Hidden.Select(id=>definition.DisplayNames.GetValueOrDefault(id,id)).ToArray(),actorVisible=bundle.Actors.Select(a=>a.Get<SpriteComponent>().Enabled).ToArray(),leaderAnimationFrame=provider.CurrentFrame,ownedBuffers=bundle.BufferCount,
                nativeVisuals=bundle.NativeVisualCount,nativeMeshes=bundle.NativeMeshCount,nativeMaterialSlots=bundle.NativeMaterialSlotCount,activeNativeLeases=NativeVisualLease.ActiveCount});
            sessionRoute.Capture=null;
        }
        if(options.SmokeFrames>0 && options.SmokeRoute=="body" && bodyRoute.Capture is string milestone)
        {
            using var capture = File.Create(Path.Combine(options.EvidenceDirectory,$"body-{milestone}.png"));
            GraphicsDevice.Presenter.BackBuffer.Save(GraphicsContext.CommandList,capture,ImageFileType.Png);
            var snapshot=session.Leader;
            bodyRoute.Milestones.Add(new {name=milestone,frame=frames,x=snapshot.Position.X,y=snapshot.Position.Y,z=snapshot.Position.Z,state=snapshot.State.ToString(),height=snapshot.BodyHeight,snapshot.StandBlocked,snapshot.Hint});
            bodyRoute.Capture=null;
        }
        if (options.SmokeFrames > 0 && frames % 30 == 0)
        {
            var feet = Vector3.TransformCoordinate(new Vector3(session.Leader.Position.X, session.Leader.Position.Y, session.Leader.Position.Z), camera.ViewProjectionMatrix);
            // Full 32x48 quad bounds are conservative, including transparent margins.
            var corners = new[] { new Vector3(-16f / SpritePixelsPerUnit, -5f / SpritePixelsPerUnit, 0), new Vector3(16f / SpritePixelsPerUnit, -5f / SpritePixelsPerUnit, 0),
                new Vector3(-16f / SpritePixelsPerUnit, 43f / SpritePixelsPerUnit, 0), new Vector3(16f / SpritePixelsPerUnit, 43f / SpritePixelsPerUnit, 0) }
                .Select(p => Vector3.TransformCoordinate(Vector3.TransformCoordinate(p, hero!.Transform.WorldMatrix), camera.ViewProjectionMatrix)).ToArray();
            cameraSamples.Add(new { frame = frames, size = camera.OrthographicSize, footX = (feet.X + 1) / 2, footY = (1 - feet.Y) / 2,
                quadLeft = (1 + corners.Min(p => p.X)) / 2, quadRight = (1 + corners.Max(p => p.X)) / 2,
                quadTop = (1 - corners.Max(p => p.Y)) / 2, quadBottom = (1 - corners.Min(p => p.Y)) / 2,
                spriteHeightFraction = (corners.Max(p => p.Y) - corners.Min(p => p.Y)) / 2, worldX = session.Leader.Position.X, worldZ = session.Leader.Position.Z });
        }
        if (options.SmokeFrames > 0 && (frames == 30 || frames == 180 || frames == options.SmokeFrames || (options.SmokeRoute=="window-states"&&frames%60==0) || (options.SmokeRoute == "edges" && (frames % 810 == 0 || frames == 945))))
        {
            using var stream = File.Create(Path.Combine(options.EvidenceDirectory, $"frame-{frames:D4}.png"));
            GraphicsDevice.Presenter.BackBuffer.Save(GraphicsContext.CommandList, stream, ImageFileType.Png);
        }
        if (options.SmokeFrames > 0 && (frames >= options.SmokeFrames || options.SmokeRoute=="body" && bodyRoute.Complete || sessionRoute.Complete))
        {
            var manifestPath = Path.Combine(AppContext.BaseDirectory, "build-manifest.json");
            JsonElement? build = File.Exists(manifestPath) ? JsonSerializer.Deserialize<JsonElement>(File.ReadAllText(manifestPath)) : null;
            File.WriteAllText(Path.Combine(options.EvidenceDirectory, "run.json"), JsonSerializer.Serialize(new
            {
                scene = definition.DisplayNames.GetValueOrDefault(definition.Id,definition.Id), sceneId=definition.Id, width = GraphicsDevice.Presenter.BackBuffer.Width, height = GraphicsDevice.Presenter.BackBuffer.Height,
                adapter = GraphicsDevice.Adapter.Description, backend = GraphicsDevice.Platform.ToString(), frames,
                smokeTiming = new { limitHz=60, drawWhileMinimized=DrawWhileMinimized, vsync=GraphicsDeviceManager.SynchronizeWithVerticalRetrace, samples=smokeWindowEvidence?.Samples },
                runtime = Environment.Version.ToString(), stride = typeof(Game).Assembly.GetName().Version?.ToString(), samples,
                camera = new { size = camera.OrthographicSize, minSize = 10.125f, maxSize = 15.75f, uprightScale = UprightScale, pitchDegrees = MathF.Atan2(31.5f, MathF.Sqrt(1458)) * 180 / MathF.PI, aimHeight = 1.35f, edgeInset = 3.375f }, cameraSamples, route = options.SmokeRoute,
                finalPosition = new { x = session.Leader.Position.X, y = session.Leader.Position.Y, z = session.Leader.Position.Z }, build,
                bodyComplete=bodyRoute.Complete, bodyMilestones=bodyRoute.Milestones, finalSnapshot=session.Leader, session=session.Snapshot, companions=session.Trail.Companions, hiddenGroups=bundle.Occlusion.Hidden, ownedBuffers=bundle.BufferCount,
                nativeVisuals=bundle.NativeVisualCount,nativeMeshes=bundle.NativeMeshCount,nativeMaterialSlots=bundle.NativeMaterialSlotCount,activeNativeLeases=NativeVisualLease.ActiveCount,
                sessionComplete=sessionRoute.Complete,sessionMilestones=sessionRoute.Milestones,portalLegs=sessionRoute.Legs,
                ladderPausePassed,
                focusProbePassed, focusZoomProbePassed, focusProbe = "Direct invocation of actual focus callbacks during the automated route; zoom route injects wheel deltas through the same clamp/discard path, not OS input",
                sourceBaseline = "c0b9065d6e902b45d4d3a5c656318c70df53a6f3", check = "automated GPU route, not manual playtest"
            }, new JsonSerializerOptions { WriteIndented = true, IncludeFields = true, Converters = { new System.Text.Json.Serialization.JsonStringEnumConverter() } }));
            Exit();
        }
        base.EndDraw(present);
    }
}
