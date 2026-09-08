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

    private async Task LoadScene()
    {
        await base.LoadContent();
        Window.Title = "Rat Expedition — courtyard | WASD: move";
        var scene = new Scene();
        var camera = new CameraComponent { Projection = CameraProjectionMode.Orthographic, OrthographicSize = 14, NearClipPlane = 0.1f, FarClipPlane = 100 };
        var cameraEntity = new Entity("Fixed orthographic camera") { camera };
        cameraEntity.Transform.Position = new(12, 14, 12);
        var view = Matrix.LookAtRH(cameraEntity.Transform.Position, Vector3.Zero, Vector3.UnitY);
        cameraEntity.Transform.Rotation = Quaternion.RotationMatrix(Matrix.Invert(view));
        scene.Entities.Add(cameraEntity);
        SceneSystem.GraphicsCompositor = GraphicsCompositorHelper.CreateDefault(false, camera: camera, clearColor: new Color4(0.065f, 0.08f, 0.10f, 1));
        scene.Entities.Add(new Entity("Ambient") { new LightComponent { Type = new LightAmbient { Color = new ColorRgbProvider(Color.White) }, Intensity = 0.55f } });
        var sunlight = new Entity("Courtyard light") { new LightComponent { Type = new LightDirectional { Color = new ColorRgbProvider(Color.White) }, Intensity = 0.65f } };
        sunlight.Transform.RotationEulerXYZ = new(-MathUtil.PiOverFour, -MathUtil.PiOverFour, 0);
        scene.Entities.Add(sunlight);
        foreach (var box in definition.Walls.Prepend(definition.Floor))
        {
            var color = box == definition.Floor ? new Color4(0.24f, 0.29f, 0.28f, 1) : new Color4(0.40f, 0.43f, 0.44f, 1);
            var material = Material.New(GraphicsDevice, new MaterialDescriptor { Attributes = { Diffuse = new MaterialDiffuseMapFeature(new ComputeColor(color)), DiffuseModel = new MaterialDiffuseLambertModelFeature() } }, Content);
            var primitive = new CubeProceduralModel { Size = new(box.Max.X - box.Min.X, box.Max.Y - box.Min.Y, box.Max.Z - box.Min.Z) };
            primitive.SetMaterial("Material", material);
            var entity = new Entity(box.Id) { new ModelComponent(primitive.Generate(Services)) };
            entity.Transform.Position = new((box.Min.X + box.Max.X) / 2, (box.Min.Y + box.Max.Y) / 2, (box.Min.Z + box.Max.Z) / 2);
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
        hero.Transform.Position = new(motor.Position.X, motor.Position.Y, motor.Position.Z);
        scene.Entities.Add(hero);
        SceneSystem.SceneInstance = new SceneInstance(Services, scene);
    }

    protected override void Update(GameTime gameTime)
    {
        if (!contentReady) { base.Update(gameTime); return; }
        var input = new System.Numerics.Vector2(
            (Input.IsKeyDown(Keys.D) ? 1 : 0) - (Input.IsKeyDown(Keys.A) ? 1 : 0),
            (Input.IsKeyDown(Keys.W) ? 1 : 0) - (Input.IsKeyDown(Keys.S) ? 1 : 0));
        if (options.SmokeFrames > 0)
            input = frames < 90 ? new(0, 1) : frames < 240 ? new(1, 0) : frames < 300 ? new(0, 1) : frames < 360 ? new(-1, 0) : System.Numerics.Vector2.Zero;
        motor.Advance(options.SmokeFrames > 0 ? 1.0 / 60 : gameTime.Elapsed.TotalSeconds, input, options.SmokeFrames > 0 || IsActive);
        if (hero is not null) hero.Transform.Position = new(motor.Position.X, motor.Position.Y, motor.Position.Z);
        if (provider is not null && input.LengthSquared() > 0) provider.CurrentFrame = frames / 12 % 2 + (Math.Abs(input.X) > Math.Abs(input.Y) ? (input.X > 0 ? 4 : 2) : input.Y > 0 ? 6 : 0);
        if (options.SmokeFrames > 0 && frames % 30 == 0) samples.Add(new { frame = frames, x = motor.Position.X, y = motor.Position.Y, z = motor.Position.Z, motor.Ticks });
        base.Update(gameTime);
    }

    protected override void EndDraw(bool present)
    {
        if (!contentReady) { base.EndDraw(present); return; }
        frames++;
        if (options.SmokeFrames > 0 && (frames == 30 || frames == 180 || frames == options.SmokeFrames))
        {
            using var stream = File.Create(Path.Combine(options.EvidenceDirectory, $"frame-{frames:D4}.png"));
            GraphicsDevice.Presenter.BackBuffer.Save(GraphicsContext.CommandList, stream, ImageFileType.Png);
        }
        if (options.SmokeFrames > 0 && frames >= options.SmokeFrames)
        {
            var manifestPath = Path.Combine(AppContext.BaseDirectory, "build-manifest.json");
            JsonElement? build = File.Exists(manifestPath) ? JsonSerializer.Deserialize<JsonElement>(File.ReadAllText(manifestPath)) : null;
            File.WriteAllText(Path.Combine(options.EvidenceDirectory, "run.json"), JsonSerializer.Serialize(new
            {
                scene = definition.Id, width = GraphicsDevice.Presenter.BackBuffer.Width, height = GraphicsDevice.Presenter.BackBuffer.Height,
                adapter = GraphicsDevice.Adapter.Description, backend = GraphicsDevice.Platform.ToString(), frames,
                runtime = Environment.Version.ToString(), stride = typeof(Game).Assembly.GetName().Version?.ToString(), samples,
                finalPosition = new { x = motor.Position.X, y = motor.Position.Y, z = motor.Position.Z }, build,
                sourceBaseline = "e2c786a45f69917bf233793f6a097b150e2fe264", check = "automated GPU route, not manual playtest"
            }, new JsonSerializerOptions { WriteIndented = true }));
            Exit();
        }
        base.EndDraw(present);
    }
}
