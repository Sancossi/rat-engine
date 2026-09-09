using System.IO;
using System.Buffers.Binary;
using System.Text.Json;
using System.Text.Json.Nodes;

namespace Rat.StrideMcp;

internal static class Wire
{
    public const int MaximumRequest=65536;
    public const int MaximumResponse=24*1024*1024;
    public static async Task<JsonObject> Read(Stream stream,int maximum,CancellationToken token)
    {
        byte[] prefix=new byte[4];await stream.ReadExactlyAsync(prefix,token);
        int length=BinaryPrimitives.ReadInt32LittleEndian(prefix);
        if(length<=0||length>maximum)throw new InvalidDataException("Frame length outside allowed bounds.");
        byte[] bytes=new byte[length];await stream.ReadExactlyAsync(bytes,token);
        return JsonNode.Parse(bytes,new JsonNodeOptions{PropertyNameCaseInsensitive=false},new JsonDocumentOptions{MaxDepth=20}) as JsonObject
            ??throw new InvalidDataException("Expected JSON object.");
    }
    public static async Task Write(Stream stream,JsonObject document,int maximum,CancellationToken token)
    {
        byte[] bytes=JsonSerializer.SerializeToUtf8Bytes(document);
        if(bytes.Length>maximum)throw new InvalidDataException("Response exceeds allowed bounds.");
        byte[] prefix=new byte[4];BinaryPrimitives.WriteInt32LittleEndian(prefix,bytes.Length);
        await stream.WriteAsync(prefix,token);await stream.WriteAsync(bytes,token);await stream.FlushAsync(token);
    }
    public static JsonObject Object(object value)=>JsonSerializer.SerializeToNode(value)!.AsObject();
}
