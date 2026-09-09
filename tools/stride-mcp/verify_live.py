"""API-only qualification against the owned A1.1 fixture; official mcp==2.2.0."""
import argparse
import asyncio
import base64
import ctypes
from ctypes import wintypes
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
    evidence={'calls':[]}
    user32=ctypes.windll.user32
    user32.GetForegroundWindow.restype=wintypes.HWND
    user32.GetWindowThreadProcessId.argtypes=[wintypes.HWND,ctypes.POINTER(wintypes.DWORD)]
    user32.ShowWindow.argtypes=[wintypes.HWND,ctypes.c_int]
    user32.IsIconic.argtypes=[wintypes.HWND]
    editor_pid=json.loads(Path(options.connection).read_text(encoding='utf-8-sig'))['processId']
    own_windows=[]
    callback=ctypes.WINFUNCTYPE(wintypes.BOOL,wintypes.HWND,wintypes.LPARAM)
    def find(hwnd,_):
        pid=wintypes.DWORD();user32.GetWindowThreadProcessId(hwnd,ctypes.byref(pid))
        if pid.value==editor_pid and user32.IsWindowVisible(hwnd):own_windows.append(hwnd)
        return True
    user32.EnumWindows(callback(find),0)
    assert own_windows,'Owned editor window not found'
    hwnd=own_windows[0]
    try:
        async with Client(StdioServerParameters(command=options.server,args=['--connection',options.connection]),read_timeout_seconds=30) as client:
            evidence['protocol']=client.protocol_version
            evidence['tools']=(await client.list_tools()).model_dump(mode='json')
            async def call(name,arguments=None,error=False):
                result=await client.call_tool(name,arguments or {})
                texts=[c.text for c in result.content if c.type=='text']
                value=json.loads(texts[0])
                evidence['calls'].append({'tool':name,'arguments':arguments,'result':value,'isError':result.is_error})
                for c in result.content:
                    if c.type=='image':
                        (folder/f'viewport-{len(evidence["calls"])}.png').write_bytes(base64.b64decode(c.data))
                assert bool(result.is_error)==error,(name,value)
                return value
            async def status():return (await call('editor_status'))['data']
            async def revision():return (await status())['revision']
            async def complete(operation):
                until=time.monotonic()+30
                while time.monotonic()<until:
                    value=(await call('editor_operation',{'operationId':operation['operationId']}))['data']
                    if value['state']=='completed':assert value['success'],value;return
                    await asyncio.sleep(.2)
                raise AssertionError('Native operation did not complete')
            scenes=(await call('scene_list'))['data']
            scene=next(s for s in scenes if s['name']=='QualificationScene')
            other=next(s for s in scenes if s['name']=='McpOtherScene')
            sid=scene['id']
            other_before=(await call('scene_inspect',{'sceneId':other['id']}))['data']
            def identity(snapshot):
                entity=next(e for e in snapshot['data']['entities'] if e['name']=='Qualification box')
                component=next(c for c in entity['components'] if c['type'].endswith('ExpeditionIdentityComponent'))
                return entity,component
            initial=await call('scene_inspect',{'sceneId':sid})
            entity,component=identity(initial)
            original=component['properties']['DisplayLabel']
            edited='Edited through MCP: qualified' if original!='Edited through MCP: qualified' else 'Edited through MCP: requalified'
            address={'sceneId':sid,'entityId':entity['id'],'componentId':component['id'],'property':'DisplayLabel'}
            await complete(await call('scene_open',{'sceneId':other['id']}))
            await complete(await call('scene_open',{'sceneId':sid}))
            await asyncio.sleep(2)
            await call('viewport_capture',{'sceneId':sid})
            before=await revision()
            user32.ShowWindow(hwnd,6)
            await asyncio.sleep(.2)
            foreground_pid=wintypes.DWORD();user32.GetWindowThreadProcessId(user32.GetForegroundWindow(),ctypes.byref(foreground_pid))
            evidence['backgroundMutation']={'editorPid':editor_pid,'foregroundPid':foreground_pid.value,'isIconic':bool(user32.IsIconic(hwnd))}
            assert evidence['backgroundMutation']['isIconic'] and foreground_pid.value!=editor_pid
            await call('entity_set_property',dict(address,value=edited,expectedRevision=before))
            changed=await call('scene_inspect',{'sceneId':sid})
            assert identity(changed)[1]['properties']['DisplayLabel']==edited
            assert (await call('scene_inspect',{'sceneId':other['id']}))['data']==other_before
            assert any(a['id']==sid for a in (await status())['dirtyAssets'])
            await call('entity_set_property',dict(address,value='STALE MUST NOT APPLY',expectedRevision=before),error=True)
            current=await status()
            await call('editor_undo',{'expectedRevision':current['revision'],'expectedTransactionId':current['undoTransactionId']})
            assert identity(await call('scene_inspect',{'sceneId':sid}))[1]['properties']['DisplayLabel']==original
            current=await status()
            await call('editor_redo',{'expectedRevision':current['revision'],'expectedTransactionId':current['redoTransactionId']})
            assert identity(await call('scene_inspect',{'sceneId':sid}))[1]['properties']['DisplayLabel']==edited
            stale=await revision()
            async with Client(StdioServerParameters(command=options.server,args=['--connection',options.connection]),read_timeout_seconds=30) as independent:
                external=await independent.call_tool('entity_set_property',dict(address,value='Independent client edit',expectedRevision=stale))
                assert not external.is_error,external
                evidence['independentClientMutation']=external.model_dump(mode='json')
            await call('entity_set_property',dict(address,value='STALE EXTERNAL',expectedRevision=stale),error=True)
            await call('entity_set_property',dict(address,value=edited,expectedRevision=await revision()))
            transform=next(c for c in entity['components'] if c['type']=='Stride.Engine.TransformComponent')
            transform_address=dict(address,componentId=transform['id'],property='Position')
            moved_x=.75 if transform['properties']['Position']['X']!=.75 else 0.0
            for bad in ({'foo':1},{'x':2,'y':0,'z':0},{'X':2,'Y':0},{'X':2,'Y':0,'Z':0,'extra':1}):
                rev=await revision()
                await call('entity_set_property',dict(transform_address,value=bad,expectedRevision=rev),error=True)
                assert await revision()==rev
            for bad_address in (dict(address,entityId='00000000-0000-0000-0000-000000000001'),dict(address,sceneId=other['id']),dict(address,property='Transform.Position'),dict(address,componentId='00000000-0000-0000-0000-000000000001')):
                rev=await revision()
                await call('entity_set_property',dict(bad_address,value='invalid',expectedRevision=rev),error=True)
                assert await revision()==rev
            camera=next(e for e in initial['data']['entities'] if e['name']=='Camera')
            camera_component=next(c for c in camera['components'] if c['type']=='Stride.Engine.CameraComponent')
            rev=await revision()
            await call('entity_set_property',dict(address,entityId=camera['id'],componentId=camera_component['id'],property='Projection',value=999,expectedRevision=rev),error=True)
            assert await revision()==rev
            await call('entity_set_property',dict(transform_address,value={'X':moved_x,'Y':0,'Z':0},expectedRevision=await revision()))
            moved=identity(await call('scene_inspect',{'sceneId':sid}))[0]
            assert next(c for c in moved['components'] if c['type']=='Stride.Engine.TransformComponent')['properties']['Position']['X']==moved_x
            user32.ShowWindow(hwnd,4)
            await asyncio.sleep(.5)
            await call('viewport_capture',{'sceneId':sid})
            assert (await call('scene_inspect',{'sceneId':other['id']}))['data']==other_before
            await complete(await call('save_session',{'expectedRevision':await revision()}))
            await call('scene_close',{'sceneId':sid,'expectedRevision':await revision()})
            await complete(await call('scene_open',{'sceneId':sid}))
            assert identity(await call('scene_inspect',{'sceneId':sid}))[1]['properties']['DisplayLabel']==edited
            await call('editor_diagnostics')
            evidence['passed']=True
    finally:
        if user32.IsIconic(hwnd):user32.ShowWindow(hwnd,4)
        (folder/'result.json').write_text(json.dumps(evidence,ensure_ascii=False,indent=2),encoding='utf-8')


if __name__=='__main__':asyncio.run(main())
