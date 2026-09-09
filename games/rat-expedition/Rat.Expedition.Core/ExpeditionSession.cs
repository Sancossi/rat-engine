using System.Numerics;

namespace Rat.Expedition.Core;

public readonly record struct SessionInput(Vector2 Move,bool CrouchHeld=false,bool InteractHeld=false,bool PauseHeld=false);
public enum SessionMode { Explore, Paused }
public enum SceneChangeReason { Initial, Portal, Recovery }
public sealed record SceneChangeCandidate(SceneDefinition Scene,TraversalSnapshot Leader,SceneChangeReason Reason);
public sealed record SessionSnapshot(SessionMode Mode,long Ticks,long WorldRevision,TraversalSnapshot Leader,Vector3 SafePoint,string Hint);

// Prepare must construct every fallible render/resource object before returning.
// Activate is a non-throwing pointer/ownership swap, and Dispose must not throw.
// Dispose releases unactivated candidates; activated resource ownership belongs to
// the presentation adapter. No graphics types or callbacks enter the motor.
public interface IPreparedSceneChange : IDisposable {void Activate();}

public sealed class ExpeditionSession
{
    private readonly ExpeditionProject project;
    private readonly Func<SceneChangeCandidate,IPreparedSceneChange>? prepare;
    private TraversalMotor motor;
    private double accumulator;
    private bool pendingInteract,interactWasHeld,pauseWasHeld;
    public SceneDefinition Scene {get;private set;}
    public SessionMode Mode {get;private set;}=SessionMode.Explore;
    public long Ticks {get;private set;}
    public long WorldRevision {get;private set;}
    public Vector3 SafePoint {get;private set;}
    public string? LastError {get;private set;}
    public TraversalSnapshot Leader=>motor.Snapshot;
    public SessionSnapshot Snapshot=>new(Mode,Ticks,WorldRevision,Leader,SafePoint,
        Mode==SessionMode.Paused?"Пауза — Esc / Enter: продолжить":LastError??Leader.Hint);

    public ExpeditionSession(ExpeditionProject project,Func<SceneChangeCandidate,IPreparedSceneChange>? prepare=null)
    {
        this.project=project;this.prepare=prepare;
        Scene=project.LoadCandidate(project.StartScene); motor=new(Scene); SafePoint=motor.Position;
        using var candidate=prepare?.Invoke(new(Scene,motor.Snapshot,SceneChangeReason.Initial));
        candidate?.Activate();
    }

    public void Advance(double elapsed,SessionInput input,bool focused=true)
    {
        if(!double.IsFinite(elapsed)||elapsed<0||!float.IsFinite(input.Move.X)||!float.IsFinite(input.Move.Y))
            throw new ArgumentOutOfRangeException(nameof(elapsed),"Time/input must be finite and time nonnegative.");
        bool pausePressed=input.PauseHeld&&!pauseWasHeld;
        bool interactPressed=input.InteractHeld&&!interactWasHeld;
        pauseWasHeld=input.PauseHeld;interactWasHeld=input.InteractHeld;
        if(!focused){Mode=SessionMode.Paused;Flush();return;}
        if(Mode==SessionMode.Paused)
        {
            if(pausePressed||interactPressed){Mode=SessionMode.Explore;Flush();}
            return;
        }
        if(pausePressed)
        {
            Mode=SessionMode.Paused;Flush();
            // Escape observes the current key state: a subsequently fresh Enter may
            // resume. Focus loss remains conservative because events can be missed.
            interactWasHeld=input.InteractHeld;return;
        }
        pendingInteract|=interactPressed;
        accumulator=Math.Min(accumulator+elapsed,TraversalMotor.StepSeconds*TraversalMotor.MaximumCatchUpSteps);
        while(accumulator+1e-12>=TraversalMotor.StepSeconds)
        {
            bool interact=pendingInteract;pendingInteract=false;
            var previousMode=motor.Mode;
            var portal=motor.Tick(new(input.Move,input.CrouchHeld,input.InteractHeld),interact);
            Ticks++; accumulator-=TraversalMotor.StepSeconds;
            if(portal is not null)
            {
                TryPortal(portal);Flush();break;
            }
            if(motor.Position.Y<Scene.RecoveryThreshold)
            {
                TryReplace(Scene,new(SafePoint.X,SafePoint.Y,SafePoint.Z),SceneChangeReason.Recovery);
                Flush();break;
            }
            if(motor.StandingSafe)SafePoint=motor.Position;
            if(previousMode==TraversalMode.Climbing&&motor.Mode!=TraversalMode.Climbing){accumulator=0;break;}
        }
    }
    private void Flush(){accumulator=0;pendingInteract=false;interactWasHeld=true;pauseWasHeld=true;}
    private void TryPortal(PortalDefinition portal)
    {
        try
        {
            var target=project.LoadCandidate(portal.TargetScene);
            TryReplace(target,target.GetSpawn(portal.TargetSpawn),SceneChangeReason.Portal);
        }
        catch(Exception error){LastError=$"Переход отклонён: {error.Message}";}
    }
    private bool TryReplace(SceneDefinition target,Point3 spawn,SceneChangeReason reason)
    {
        try
        {
            var nextMotor=new TraversalMotor(target,spawn);
            using var candidate=prepare?.Invoke(new(target,nextMotor.Snapshot,reason));
            candidate?.Activate();
            Scene=target;motor=nextMotor;SafePoint=motor.Position;WorldRevision++;LastError=null;
            return true;
        }
        catch(Exception error){LastError=$"Смена сцены отклонена: {error.Message}";return false;}
    }
}
