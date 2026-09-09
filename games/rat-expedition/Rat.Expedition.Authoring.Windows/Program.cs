using System.Text.Json;
using Rat.Expedition.Authoring;
using Stride.Engine;
using Stride.Graphics;

string evidence = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "RatExpedition", "authoring-qualification");
int smokeFrames = 0;
try
{
    for(int i=0;i<args.Length;i++)
    {
        if(i+1>=args.Length)throw new ArgumentException("Expected option value.");
        string key=args[i++],value=args[i];
        if(key=="--evidence-dir")evidence=Path.GetFullPath(value);
        else if(key=="--smoke-frames")smokeFrames=int.Parse(value);
        else throw new ArgumentException($"Unknown option {key}");
    }
    if(smokeFrames!=0&&(smokeFrames<60||smokeFrames>600))throw new ArgumentException("Smoke frames must be 0 or 60..600.");
    Directory.CreateDirectory(evidence);
    using var game=new QualificationGame(evidence,smokeFrames);
    game.Run();
    if(game.Failure is not null)throw game.Failure;
    return 0;
}
catch(Exception error)
{
    Directory.CreateDirectory(evidence);
    File.WriteAllText(Path.Combine(evidence,"error.log"),error.ToString());
    return 1;
}

// This bounded launcher qualifies native content. The existing P1 executable
// remains independent until the reviewed A1.2 scene/Core migration.
sealed class QualificationGame(string evidence,int smokeFrames) : Game
{
    private bool ready;
    private int frames;
    public Exception? Failure {get;private set;}
    protected override async Task LoadContent()
    {
        try
        {
            await base.LoadContent();
            var scene=Content.Load<Scene>("QualificationScene");
            var entity=scene.Entities.Single(e=>e.Get<ExpeditionIdentityComponent>() is not null);
            var identity=entity.Get<ExpeditionIdentityComponent>();
            if(string.IsNullOrWhiteSpace(identity.GameId)||string.IsNullOrWhiteSpace(identity.DisplayLabel))
                throw new InvalidDataException("QualificationScene identity requires GameId and DisplayLabel.");
            if(!ReferenceEquals(SceneSystem.SceneInstance.RootScene,scene))
                throw new InvalidDataException("GameSettings and Content.Load must resolve the same native scene.");
            Window.Title=$"Rat native authoring | {identity.GameId} | {identity.DisplayLabel}";
            File.WriteAllText(Path.Combine(evidence,"loaded-asset.json"),JsonSerializer.Serialize(new {
                asset="QualificationScene",entity.Id,entity.Name,identity.GameId,identity.DisplayLabel,
                position=new {entity.Transform.Position.X,entity.Transform.Position.Y,entity.Transform.Position.Z},
                source="compiled native scene via Content.Load",stride=typeof(Game).Assembly.GetName().Version?.ToString()
            },new JsonSerializerOptions {WriteIndented=true}));
            if(smokeFrames>0)
            {
                WindowMinimumUpdateRate.SetMaxFrequency(60);MinimizedMinimumUpdateRate.SetMaxFrequency(60);
                DrawWhileMinimized=true;
            }
            ready=true;
        }
        catch(Exception error){Failure=error;Exit();}
    }
    protected override void EndDraw(bool present)
    {
        if(ready&&++frames==60)
        {
            using var stream=File.Create(Path.Combine(evidence,"native-scene.png"));
            GraphicsDevice.Presenter.BackBuffer.Save(GraphicsContext.CommandList,stream,ImageFileType.Png);
        }
        if(ready&&smokeFrames>0&&frames>=smokeFrames)Exit();
        base.EndDraw(present);
    }
}
