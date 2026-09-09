using System.Windows.Threading;

namespace Rat.StrideMcp;

internal static class UiRequestQueue
{
    public static async Task<T> Run<T>(Dispatcher dispatcher,Func<Task<T>> action,CancellationToken cancellation)
    {
        var queued=dispatcher.InvokeAsync(()=>{
            cancellation.ThrowIfCancellationRequested();
            return action();
        },DispatcherPriority.Normal,cancellation);
        // Cancellation can abort queued work. A synchronous native mutation that
        // already started must finish and report its outcome, not be abandoned.
        return await await queued.Task;
    }
}
