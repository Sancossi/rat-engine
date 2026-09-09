"""Bounded stdio protocol and unavailable/misaddressed editor regression."""
import argparse
import asyncio
import json
import time
from pathlib import Path
from mcp import Client
from mcp.client.stdio import StdioServerParameters


async def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--server',required=True)
    parser.add_argument('--connection',required=True)
    parser.add_argument('--output',required=True)
    options=parser.parse_args()
    folder=Path(options.output);folder.mkdir(parents=True,exist_ok=True)
    evidence={}
    descriptor=json.loads(Path(options.connection).read_text(encoding='utf-8-sig'))
    for name,change,diagnostic in [
        ('wrong-session',{'sessionId':'00000000-0000-0000-0000-000000000001'},'Wrong editor process/project/session'),
        ('wrong-pid',{'processId':1},'different editor process'),
        ('unavailable',{'pipeName':'rat-stride-mcp-absent-fixture'},'timed out')]:
        connection=folder/f'{name}.json';connection.write_text(json.dumps(dict(descriptor,**change)),encoding='utf-8')
        started=time.monotonic()
        async with Client(StdioServerParameters(command=options.server,args=['--connection',str(connection.resolve())]),read_timeout_seconds=20) as client:
            result=await client.call_tool('editor_status')
            assert result.is_error,result
            text=' '.join(c.text for c in result.content if c.type=='text')
            # .NET timeout message is localized; the portable observable contract
            # is IsError + bounded elapsed time, not an English string for timeout.
            if name!='unavailable':assert diagnostic in text,text
            evidence[name]={'elapsed':time.monotonic()-started,'result':result.model_dump(mode='json')}
            assert evidence[name]['elapsed']<12
    with (folder/'protocol.stderr.log').open('wb') as stderr:
        process=await asyncio.create_subprocess_exec(options.server,'--connection',options.connection,stdin=asyncio.subprocess.PIPE,stdout=asyncio.subprocess.PIPE,stderr=stderr)
        async def send(message):
            process.stdin.write(json.dumps(message).encode()+b'\n');await process.stdin.drain()
        async def receive(identifier):
            while True:
                raw=await asyncio.wait_for(process.stdout.readline(),10)
                assert raw,'Protocol EOF'
                response=json.loads(raw)
                if response.get('id')==identifier:return response
        try:
            await send({'jsonrpc':'2.0','id':1,'method':'initialize','params':{'protocolVersion':'2025-11-25','capabilities':{},'clientInfo':{'name':'rat-wire-regression','version':'1'}}})
            evidence['initialize']=await receive(1)
            assert evidence['initialize']['result']['protocolVersion']=='2025-11-25'
            await send({'jsonrpc':'2.0','method':'notifications/initialized'})
            await send({'jsonrpc':'2.0','id':2,'method':'ping'})
            assert (await receive(2))['result']=={}
            await send({'jsonrpc':'2.0','id':3,'method':'not_a_method'})
            evidence['unknownMethod']=await receive(3)
            assert evidence['unknownMethod']['error']['code']==-32601
            await send({'jsonrpc':'2.0','id':4,'method':'tools/call','params':{'name':'scene_inspect','arguments':{}}})
            evidence['invalidParams']=await receive(4)
            assert 'error' in evidence['invalidParams'] or evidence['invalidParams'].get('result',{}).get('isError')
            process.stdin.write(b'{' + b'x'*65537+b'\n');await process.stdin.drain()
            process.stdin.close()
            await asyncio.wait_for(process.wait(),10)
            evidence['oversizedInputExited']=process.returncode
        finally:
            if process.returncode is None:process.kill();await process.wait()
    evidence['passed']=True
    (folder/'result.json').write_text(json.dumps(evidence,ensure_ascii=False,indent=2),encoding='utf-8')


if __name__=='__main__':asyncio.run(main())
