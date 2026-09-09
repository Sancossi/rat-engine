using System.IO;

namespace Rat.StrideMcp;

// A byte limit around official SDK stdin; MCP parsing/negotiation stays in SDK.
internal sealed class LineLimitStream(Stream input,int maximum):Stream
{
    private int lineBytes;
    private int Check(ReadOnlySpan<byte> data)
    {
        foreach(byte value in data)
        {if(value==10)lineBytes=0;else if(++lineBytes>maximum)throw new InvalidDataException("MCP input line exceeds byte limit.");}
        return data.Length;
    }
    public override int Read(byte[] buffer,int offset,int count)=>Check(buffer.AsSpan(offset,input.Read(buffer,offset,count)));
    public override async ValueTask<int> ReadAsync(Memory<byte> buffer,CancellationToken cancellationToken=default)
    {int count=await input.ReadAsync(buffer,cancellationToken);return Check(buffer.Span[..count]);}
    public override Task<int> ReadAsync(byte[] buffer,int offset,int count,CancellationToken cancellationToken)=>ReadAsync(buffer.AsMemory(offset,count),cancellationToken).AsTask();
    public override bool CanRead=>true;public override bool CanSeek=>false;public override bool CanWrite=>false;
    public override long Length=>throw new NotSupportedException();
    public override long Position {get=>throw new NotSupportedException();set=>throw new NotSupportedException();}
    public override void Flush()=>throw new NotSupportedException();
    public override long Seek(long offset,SeekOrigin origin)=>throw new NotSupportedException();
    public override void SetLength(long value)=>throw new NotSupportedException();
    public override void Write(byte[] buffer,int offset,int count)=>throw new NotSupportedException();
}
