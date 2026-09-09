using Stride.Engine;

namespace Rat.Expedition.Authoring;

public static class NativePrefabInstances
{
    // Runtime Prefab.Instantiate preserves serialized IDs at the pinned version.
    // GameStudio duplication already generates IDs. Use this boundary for runtime
    // instances; object references remapped by Stride's clone remain intact.
    public static List<Entity> Instantiate(Prefab prefab)
    {
        var roots=prefab.Instantiate();var queue=new Queue<Entity>(roots);var visited=new HashSet<Entity>();
        while(queue.TryDequeue(out var entity))
        {
            if(!visited.Add(entity))throw new InvalidDataException("Prefab contains a repeated entity/hierarchy cycle.");
            entity.Id=Guid.NewGuid();
            foreach(var child in entity.Transform.Children)queue.Enqueue(child.Entity);
        }
        return roots;
    }
}
