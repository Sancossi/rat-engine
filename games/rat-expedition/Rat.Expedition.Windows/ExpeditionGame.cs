using System.Text.Json;
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
    private readonly SceneDefinition definition;
    private readonly TraversalMotor motor;
    private readonly List<object> samples = [];
    private Entity? hero;
    private Texture? atlas;
    private SpriteFromSheet? provider;
    private int frames;
    private bool contentReady;
    private bool discardInputOnNextUpdate;
    private bool? focusProbePassed;
    private bool? focusZoomProbePassed;
    private CameraComponent camera = null!;
    private Entity cameraEntity = null!;
    private static readonly Vector3 CameraOffset = new(12, 14, 12);
    private static readonly float UprightScale = CameraOffset.Length() / new Vector2(CameraOffset.X, CameraOffset.Z).Length();
    private readonly List<object> cameraSamples = [];
    private readonly BodySmokeRoute bodyRoute = new();
    private SpriteFont? hudFont;
    private SpriteBatch? hudBatch;
    public Exception? FatalError { get; private set; }

    public ExpeditionGame(GameOptions options, SceneDefinition definition)
    {
        this.options = options;
        this.definition = definition;
        motor = new(definition);
        AutoLoadDefaultSettings = false;
        GraphicsDeviceManager.PreferredBackBufferWidth = options.Width;
        GraphicsDeviceManager.PreferredBackBufferHeight = options.Height;
        GraphicsDeviceManager.SynchronizeWithVerticalRetrace = true;
        IsMouseVisible = true;
    }

    protected override async Task LoadContent()
    {
        try { await LoadScene(); contentReady = true; }
        catch (Exception error) { FatalError = error; Exit(); }
    }

    protected override void Initialize()
    {
        if (Settings is not null) { Settings.EffectCompilation = EffectCompilationMode.Local; Settings.RecordUsedEffects = false; }
        base.Initialize();
    }

    protected override void OnDeactivated(object sender, EventArgs args)
    {
        // Stride skips Update entirely while inactive, so the callback must flush the motor.
        motor?.Advance(0, new(System.Numerics.Vector2.Zero), false);
        discardInputOnNextUpdate = true;
        base.OnDeactivated(sender, args);
    }

    protected override void OnActivated(object sender, EventArgs args)
    {
        ResetElapsedTime();
        discardInputOnNextUpdate = true;
        base.OnActivated(sender, args);
    }

    private async Task LoadScene()
    {
        await base.LoadContent();
        Window.Title = "Rat Expedition | WASD: move | Ctrl: crouch | E: ladder | Wheel: zoom";
        var fontSystem = (Stride.Graphics.Font.FontSystem)Font;
        fontSystem.RuntimeFonts.RegisterFont("RatNoto",Path.Combine(options.ContentDirectory,"fonts","NotoSans-Regular.ttf"));
        hudFont = fontSystem.LoadRuntimeFont("RatNoto",20f) ?? throw new InvalidDataException("Cannot load RatNoto font.");
        hudBatch = new SpriteBatch(GraphicsDevice);
        var scene = new Scene();
        camera = new CameraComponent { Projection = CameraProjectionMode.Orthographic, OrthographicSize = options.CameraSize, NearClipPlane = 0.1f, FarClipPlane = 100 };
        cameraEntity = new Entity("Following orthographic camera") { camera };
        FollowHero();
        var view = Matrix.LookAtRH(CameraOffset, Vector3.Zero, Vector3.UnitY);
        cameraEntity.Transform.Rotation = Quaternion.RotationMatrix(Matrix.Invert(view));
        scene.Entities.Add(cameraEntity);
        SceneSystem.GraphicsCompositor = GraphicsCompositorHelper.CreateDefault(false, camera: camera, clearColor: new Color4(0.065f, 0.08f, 0.10f, 1));
        scene.Entities.Add(new Entity("Ambient") { new LightComponent { Type = new LightAmbient { Color = new ColorRgbProvider(Color.White) }, Intensity = 0.55f } });
        var sunlight = new Entity("Courtyard light") { new LightComponent { Type = new LightDirectional { Color = new ColorRgbProvider(Color.White) }, Intensity = 0.65f } };
        sunlight.Transform.RotationEulerXYZ = new(-MathUtil.PiOverFour, -MathUtil.PiOverFour, 0);
        scene.Entities.Add(sunlight);
        foreach (var box in definition.AllSolids.Concat(LadderDecorations()))
        {
            var color = box.Id.StartsWith("ladder-decoration-") ? new Color4(.65f,.39f,.15f,1) :
                box == definition.Floor ? new Color4(0.24f, 0.29f, 0.28f, 1) : new Color4(0.40f, 0.43f, 0.44f, 1);
            var material = Material.New(GraphicsDevice, new MaterialDescriptor { Attributes = { Diffuse = new MaterialDiffuseMapFeature(new ComputeColor(color)), DiffuseModel = new MaterialDiffuseLambertModelFeature() } }, Content);
            var primitive = new CubeProceduralModel { Size = new(box.Max.X - box.Min.X, box.Max.Y - box.Min.Y, box.Max.Z - box.Min.Z) };
            primitive.SetMaterial("Material", material);
            var entity = new Entity(box.Id) { new ModelComponent(primitive.Generate(Services)) };
            entity.Transform.Position = new(box.Min.X + (box.Max.X - box.Min.X) / 2,
                box.Min.Y + (box.Max.Y - box.Min.Y) / 2, box.Min.Z + (box.Max.Z - box.Min.Z) / 2);
            scene.Entities.Add(entity);
        }

        string png = Path.Combine(options.ContentDirectory, "sprites", "rat.png");
        using (var stream = File.OpenRead(png)) atlas = Texture.Load(GraphicsDevice, stream);
        if (atlas.Width != 64 || atlas.Height != 192) throw new InvalidDataException("Rat PNG must be the 64x192 eight-frame atlas.");
        var sheet = new SpriteSheet();
        for (int row = 0; row < 4; row++)
        for (int column = 0; column < 2; column++)
            sheet.Sprites.Add(new Sprite(atlas) { Name = $"rat-{row}-{column}", Region = new RectangleF(column * 32, row * 48, 32, 48), Center = new Vector2(16, 43), PixelsPerUnit = new Vector2(60), IsTransparent = true });
        provider = new SpriteFromSheet { Sheet = sheet };
        // Upright yaw-facing quad fits inside the footprint. Full camera billboarding would
        // lean the head into a wall even when the leader's feet are safely in front of it.
        hero = new Entity("Rat leader") { new SpriteComponent { SpriteProvider = provider, SpriteType = SpriteType.Sprite, Sampler = SpriteSampler.PointClamp, IgnoreDepth = false, PremultipliedAlpha = false, IsAlphaCutoff = true } };
        hero.Transform.Rotation = Quaternion.RotationY(MathUtil.PiOverFour);
        // Correct only the vertical foreshortening caused by keeping the quad upright.
        hero.Transform.Scale = new(1, UprightScale, 1);
        hero.Transform.Position = new(motor.Position.X, motor.Position.Y, motor.Position.Z);
        scene.Entities.Add(hero);
        SceneSystem.SceneInstance = new SceneInstance(Services, scene);
    }

    private void FollowHero()
    {
        static float ClampAim(float value, float min, float max)
        {
            float inset = Math.Min(1.5f, (max - min) / 2);
            return Math.Clamp(value, min + inset, max - inset);
        }
        cameraEntity.Transform.Position = CameraOffset + new Vector3(
            ClampAim(motor.Position.X, definition.Floor.Min.X, definition.Floor.Max.X), motor.Position.Y + .6f,
            ClampAim(motor.Position.Z, definition.Floor.Min.Z, definition.Floor.Max.Z));
    }

    private IEnumerable<WorldBox> LadderDecorations()
    {
        // Rungs/rails mark the traversable ladder corridor; they are not solid blockers.
        foreach(var ladder in definition.Ladders)
        {
            float visualZ = ladder.Bottom.Z - .25f; // Behind the upright hero, toward this field scene's platform.
            foreach(float side in new[]{-.3f,.3f})
                yield return new($"ladder-decoration-{ladder.Id}-{side}",new(ladder.Bottom.X+side-.025f,ladder.Bottom.Y,visualZ-.025f),new(ladder.Top.X+side+.025f,ladder.Top.Y+.25f,visualZ+.025f));
            for(float y=ladder.Bottom.Y+.15f;y<=ladder.Top.Y;y+=.2f)
                yield return new($"ladder-decoration-{ladder.Id}-rung-{y}",new(ladder.Bottom.X-.3f,y,visualZ-.025f),new(ladder.Top.X+.3f,y+.035f,visualZ+.025f));
        }
    }

    protected override void Draw(GameTime gameTime)
    {
        base.Draw(gameTime);
        if(!contentReady || hudBatch is null || hudFont is null) return;
        GraphicsContext.CommandList.SetRenderTargetAndViewport(GraphicsDevice.Presenter.DepthStencilBuffer,GraphicsDevice.Presenter.BackBuffer);
        hudBatch.Begin(GraphicsContext);
        string text = motor.Snapshot.Hint;
        hudBatch.DrawString(hudFont,text,new Vector2(25,25),Color.Black);
        hudBatch.DrawString(hudFont,text,new Vector2(24,24),Color.White);
        hudBatch.End();
    }

    protected override void Destroy()
    {
        hudBatch?.Dispose(); hudFont?.Dispose(); atlas?.Dispose();
        base.Destroy();
    }

    protected override void Update(GameTime gameTime)
    {
        if (!contentReady) { base.Update(gameTime); return; }
        var beforeFocus = motor.Position;
        float zoomBeforeFocus = camera.OrthographicSize;
        long ticksBeforeFocus = motor.Ticks;
        bool probeFocus = options.SmokeFrames > 120 && frames == 120;
        if (probeFocus) { OnDeactivated(this, EventArgs.Empty); OnActivated(this, EventArgs.Empty); }
        var input = new System.Numerics.Vector2(
            (Input.IsKeyDown(Keys.D) ? 1 : 0) - (Input.IsKeyDown(Keys.A) ? 1 : 0),
            (Input.IsKeyDown(Keys.W) ? 1 : 0) - (Input.IsKeyDown(Keys.S) ? 1 : 0));
        if (options.SmokeFrames > 0)
            input = frames < 90 ? new(0, 1) : frames < 240 ? new(1, 0) : frames < 300 ? new(0, 1) : frames < 360 ? new(-1, 0) : System.Numerics.Vector2.Zero;
        if (options.SmokeFrames > 0 && options.SmokeRoute == "edges")
            input = frames < 180 ? new(0, -1) : frames < 420 ? new(1, -1) : frames < 720 ? new(1, 1) : frames < 1080 ? new(-1, 1) : new(-1, -1);
        if (options.SmokeFrames > 0 && options.SmokeRoute == "zoom") input = System.Numerics.Vector2.Zero;
        var command = new TraversalInput(input,Input.IsKeyDown(Keys.LeftCtrl)||Input.IsKeyDown(Keys.RightCtrl),Input.IsKeyDown(Keys.E)||Input.IsKeyDown(Keys.Enter));
        if(options.SmokeFrames>0) command = options.SmokeRoute=="body" ? bodyRoute.Next(motor.Snapshot) : new(input);
        double elapsed = options.SmokeFrames > 0 ? (options.SmokeRoute is "edges" or "body" ? 1.0 / 30 : 1.0 / 60) : gameTime.Elapsed.TotalSeconds;
        float wheel = Input.MouseWheelDelta;
        if (options.SmokeFrames > 0) wheel = options.SmokeRoute == "zoom" ? frames switch { 30 or 120 or 150 => 100, 90 or 210 => -100, 270 => 4, _ => 0 } : 0;
        bool discard = discardInputOnNextUpdate;
        if (discard) { elapsed = 0; command = command with {Move=System.Numerics.Vector2.Zero}; wheel = 0; discardInputOnNextUpdate = false; }
        if (IsActive || options.SmokeFrames > 0) camera.OrthographicSize = Math.Clamp(camera.OrthographicSize - wheel * .5f, 4.5f, 7f);
        motor.Advance(elapsed, command, !discard && (options.SmokeFrames > 0 || IsActive));
        if (probeFocus) focusProbePassed = motor.Position == beforeFocus && motor.Ticks == ticksBeforeFocus;
        if (probeFocus) focusZoomProbePassed = camera.OrthographicSize == zoomBeforeFocus;
        if (hero is not null) { hero.Transform.Position = new(motor.Position.X, motor.Position.Y, motor.Position.Z); hero.Transform.Scale = new(1,UprightScale*(motor.Stance==BodyStance.Crouched?.5f:1),1); }
        FollowHero();
        input = command.Move;
        if (provider is not null && input.LengthSquared() > 0) provider.CurrentFrame = frames / 12 % 2 + (Math.Abs(input.X) > Math.Abs(input.Y) ? (input.X > 0 ? 4 : 2) : input.Y > 0 ? 6 : 0);
        if (options.SmokeFrames > 0 && frames % 30 == 0) samples.Add(new { frame = frames, x = motor.Position.X, y = motor.Position.Y, z = motor.Position.Z, motor.Ticks, snapshot=motor.Snapshot });
        base.Update(gameTime);
    }

    protected override void EndDraw(bool present)
    {
        if (!contentReady) { base.EndDraw(present); return; }
        frames++;
        if(options.SmokeFrames>0 && options.SmokeRoute=="body" && bodyRoute.Capture is string milestone)
        {
            using var capture = File.Create(Path.Combine(options.EvidenceDirectory,$"body-{milestone}.png"));
            GraphicsDevice.Presenter.BackBuffer.Save(GraphicsContext.CommandList,capture,ImageFileType.Png);
            var snapshot=motor.Snapshot;
            bodyRoute.Milestones.Add(new {name=milestone,frame=frames,x=snapshot.Position.X,y=snapshot.Position.Y,z=snapshot.Position.Z,state=snapshot.State.ToString(),height=snapshot.BodyHeight,snapshot.StandBlocked,snapshot.Hint});
            bodyRoute.Capture=null;
        }
        if (options.SmokeFrames > 0 && frames % 30 == 0)
        {
            var feet = Vector3.TransformCoordinate(new Vector3(motor.Position.X, motor.Position.Y, motor.Position.Z), camera.ViewProjectionMatrix);
            // Full 32x48 quad bounds are conservative, including transparent margins.
            var corners = new[] { new Vector3(-16f / 60, -5f / 60, 0), new Vector3(16f / 60, -5f / 60, 0),
                new Vector3(-16f / 60, 43f / 60, 0), new Vector3(16f / 60, 43f / 60, 0) }
                .Select(p => Vector3.TransformCoordinate(Vector3.TransformCoordinate(p, hero!.Transform.WorldMatrix), camera.ViewProjectionMatrix)).ToArray();
            cameraSamples.Add(new { frame = frames, size = camera.OrthographicSize, footX = (feet.X + 1) / 2, footY = (1 - feet.Y) / 2,
                quadLeft = (1 + corners.Min(p => p.X)) / 2, quadRight = (1 + corners.Max(p => p.X)) / 2,
                quadTop = (1 - corners.Max(p => p.Y)) / 2, quadBottom = (1 - corners.Min(p => p.Y)) / 2,
                spriteHeightFraction = (corners.Max(p => p.Y) - corners.Min(p => p.Y)) / 2, worldX = motor.Position.X, worldZ = motor.Position.Z });
        }
        if (options.SmokeFrames > 0 && (frames == 30 || frames == 180 || frames == options.SmokeFrames || (options.SmokeRoute == "edges" && (frames % 360 == 0 || frames == 420))))
        {
            using var stream = File.Create(Path.Combine(options.EvidenceDirectory, $"frame-{frames:D4}.png"));
            GraphicsDevice.Presenter.BackBuffer.Save(GraphicsContext.CommandList, stream, ImageFileType.Png);
        }
        if (options.SmokeFrames > 0 && (frames >= options.SmokeFrames || options.SmokeRoute=="body" && bodyRoute.Complete))
        {
            var manifestPath = Path.Combine(AppContext.BaseDirectory, "build-manifest.json");
            JsonElement? build = File.Exists(manifestPath) ? JsonSerializer.Deserialize<JsonElement>(File.ReadAllText(manifestPath)) : null;
            File.WriteAllText(Path.Combine(options.EvidenceDirectory, "run.json"), JsonSerializer.Serialize(new
            {
                scene = definition.Id, width = GraphicsDevice.Presenter.BackBuffer.Width, height = GraphicsDevice.Presenter.BackBuffer.Height,
                adapter = GraphicsDevice.Adapter.Description, backend = GraphicsDevice.Platform.ToString(), frames,
                runtime = Environment.Version.ToString(), stride = typeof(Game).Assembly.GetName().Version?.ToString(), samples,
                camera = new { size = camera.OrthographicSize, minSize = 4.5f, maxSize = 7f, uprightScale = UprightScale, pitchDegrees = MathF.Atan2(14, MathF.Sqrt(288)) * 180 / MathF.PI, aimHeight = .6f, edgeInset = 1.5f }, cameraSamples, route = options.SmokeRoute,
                finalPosition = new { x = motor.Position.X, y = motor.Position.Y, z = motor.Position.Z }, build,
                bodyComplete=bodyRoute.Complete, bodyMilestones=bodyRoute.Milestones, finalSnapshot=motor.Snapshot,
                focusProbePassed, focusZoomProbePassed, focusProbe = "Direct invocation of actual focus callbacks during the automated route; zoom route injects wheel deltas through the same clamp/discard path, not OS input",
                sourceBaseline = "e2c786a45f69917bf233793f6a097b150e2fe264", check = "automated GPU route, not manual playtest"
            }, new JsonSerializerOptions { WriteIndented = true, IncludeFields = true, Converters = { new System.Text.Json.Serialization.JsonStringEnumConverter() } }));
            Exit();
        }
        base.EndDraw(present);
    }
}
