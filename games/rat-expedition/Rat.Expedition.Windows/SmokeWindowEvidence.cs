using System.Diagnostics;
using System.Runtime.InteropServices;

namespace Rat.Expedition.Windows;

// Read-only OS observations: the external verifier changes only its own game HWND.
internal sealed class SmokeWindowEvidence
{
    private readonly Stopwatch clock = Stopwatch.StartNew();
    public List<object> Samples { get; } = [];
    public void Record(ExpeditionGame game, int frame, int draws, long ticks, string directory)
    {
        var nativeHandle = game.Window.NativeWindow.Handle;
        var handle = GetAncestor(nativeHandle, 2); // GA_ROOT: WinForms may expose a render child.
        var sample = new { frame, draws, ticks, seconds = clock.Elapsed.TotalSeconds,
            hwnd = handle.ToInt64(), nativeHandle = nativeHandle.ToInt64(), windowBackend=game.Window.NativeWindow.Context.ToString(), isIconic = IsIconic(handle), visible = IsWindowVisible(handle),
            foreground = GetForegroundWindow() == handle, game.IsActive,
            windowFocused = game.Window.Focused, windowMinimized = game.Window.IsMinimized };
        Samples.Add(sample);
        File.WriteAllText(Path.Combine(directory, "smoke-progress.json"), System.Text.Json.JsonSerializer.Serialize(sample));
    }
    [DllImport("user32.dll")] private static extern bool IsIconic(IntPtr window);
    [DllImport("user32.dll")] private static extern bool IsWindowVisible(IntPtr window);
    [DllImport("user32.dll")] private static extern IntPtr GetForegroundWindow();
    [DllImport("user32.dll")] private static extern IntPtr GetAncestor(IntPtr window,uint flags);
}
