using Rat.Expedition.Core;
using Rat.Expedition.Windows;

return Entry.Run(args);

namespace Rat.Expedition.Windows
{
    public static class Entry
    {
        public static int Run(string[] args)
        {
            GameOptions? options = null;
            try
            {
                options = GameOptions.Parse(args);
                AppContext.SetSwitch("Stride.Engine.RemoteEffectCompilerEnabled", false);
                Directory.CreateDirectory(options.EvidenceDirectory);
                using var game = new ExpeditionGame(options);
                game.Run();
                if (game.FatalError is not null) throw game.FatalError;
                return 0;
            }
            catch (Exception error)
            {
                var message = $"Rat Expedition startup/run failed: {error}";
                Console.Error.WriteLine(message);
                if (options is not null)
                    File.WriteAllText(Path.Combine(options.EvidenceDirectory, "error.log"), message);
                return 1;
            }
        }
    }

    public sealed record GameOptions(int Width, int Height, int SmokeFrames, string EvidenceDirectory, string ContentDirectory, float CameraSize, string SmokeRoute, string NativeProject)
    {
        public static GameOptions Parse(string[] args)
        {
            int width = 1280, height = 720, frames = 0;
            float cameraSize = 11.25f;
            string route = "walls";
            string nativeProject = "ExpeditionProject";
            string evidence = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "RatExpedition", "logs");
            string content = Path.Combine(AppContext.BaseDirectory, "Content");
            for (int i = 0; i < args.Length; i++)
            {
                if (i + 1 >= args.Length) throw new ArgumentException($"Missing value for {args[i]}");
                string key = args[i++], value = args[i];
                switch (key)
                {
                    case "--width": width = int.Parse(value); break;
                    case "--height": height = int.Parse(value); break;
                    case "--smoke-frames": frames = int.Parse(value); break;
                    case "--evidence-dir": evidence = Path.GetFullPath(value); break;
                    case "--content-dir": content = Path.GetFullPath(value); break;
                    case "--camera-size": cameraSize = float.Parse(value, System.Globalization.CultureInfo.InvariantCulture); break;
                    case "--smoke-route": route = value; break;
                    case "--native-project": nativeProject = value; break;
                    default: throw new ArgumentException($"Unknown argument {key}");
                }
            }
            if (width < 640 || height < 360 || width > 3840 || height > 2160 || frames < 0 || (frames > 0 && frames < 60) || frames > 7200)
                throw new ArgumentException("Window must be 640x360..3840x2160; smoke frames 0 or 60..7200.");
            if (!float.IsFinite(cameraSize) || cameraSize < 10.125f || cameraSize > 15.75f || route is not ("walls" or "edges" or "zoom" or "body" or "layered" or "mixed" or "portals" or "portal-failure" or "native-candidate-failure" or "native-resource-failure" or "recovery" or "window-states"))
                throw new ArgumentException("Camera size must be 10.125..15.75 and smoke route must be supported.");
            if(nativeProject!="ExpeditionProject" && (frames==0 || !System.Text.RegularExpressions.Regex.IsMatch(nativeProject,@"\AQA/[a-z-]+\z")))
                throw new ArgumentException("Alternate compiled QA projects require an explicit smoke run and QA/name asset URL.");
            return new(width, height, frames, evidence, content, cameraSize, route,nativeProject);
        }
    }
}
