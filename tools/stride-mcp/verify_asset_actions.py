"""Actual MCP qualification of three native asset actions; no screenshots or input."""
import argparse, asyncio, json
from pathlib import Path
from mcp import Client
from mcp.client.stdio import StdioServerParameters

async def main():
    p=argparse.ArgumentParser()
    for key in ('server','connection','fixture','output','snapshot'): p.add_argument('--'+key,required=True)
    p.add_argument('--reopen',action='store_true'); args=p.parse_args()
    fixture=json.loads(Path(args.fixture).read_text(encoding='utf-8-sig')); ids=fixture['assets']; evidence={'passed':False,'calls':[]}
    try:
        async with Client(StdioServerParameters(command=args.server,args=['--connection',args.connection]),read_timeout_seconds=20) as client:
            evidence['tools']=[t.name for t in (await client.list_tools()).tools]; assert len(evidence['tools'])==19
            async def call(tool,arguments=None,error=False):
                response=await client.call_tool(tool,arguments or {})
                value=json.loads(next(c.text for c in response.content if c.type=='text'))
                evidence['calls'].append(dict(tool=tool,arguments=arguments,result=value,isError=response.is_error))
                assert bool(response.is_error)==error,(tool,value)
                return value
            async def state():
                await asyncio.sleep(.12)
                return (await call('editor_status'))['data']
            async def inspect(name): return (await call('asset_inspect',dict(assetId=ids[name])))['data']
            async def mutate(tool,**kwargs): return (await call(tool,dict(expectedRevision=(await state())['revision'],**kwargs)))['data']
            async def history(direction):
                s=await state();return await call('editor_'+direction,dict(expectedRevision=s['revision'],expectedTransactionId=s[direction+'TransactionId']))
            async def rejected(tool,**kwargs):
                before=await state();snapshot=await inspect('Target')
                response=await call(tool,dict(expectedRevision=before['revision'],**kwargs),True)
                after=await state()
                for key in ('revision','dirtyAssets','undoTransactionId','redoTransactionId'): assert before[key]==after[key],(tool,key)
                assert snapshot==await inspect('Target')
                return response
            async def save():
                op=await call('save_session',dict(expectedRevision=(await state())['revision']))
                for _ in range(100):
                    result=(await call('editor_operation',dict(operationId=op['operationId'])))['data']
                    if result['state']=='completed': assert result['success'],result;break
                    await asyncio.sleep(.1)
                else: raise AssertionError('Save exceeded deadline')
                assert not (await state())['dirtyAssets']
            async def snapshots():
                # URLs are native session representations (reload can add package
                # prefixes). Check each against the actual target, then compare IDs.
                assets={name:await inspect(name) for name in ids if name!='Unreferenced'}
                urls={asset['id']:asset['url'] for asset in assets.values()}
                def semantic(value):
                    if isinstance(value,list):
                        return sorted((semantic(v) for v in value),key=lambda v:json.dumps(v,sort_keys=True))
                    if not isinstance(value,dict): return value
                    for url_key,id_key in (('url','id'),('targetUrl','targetAssetId'),('baseUrl','baseAssetId')):
                        if value.get(url_key) is not None:
                            assert value[url_key]==urls[value[id_key]],(url_key,value,urls)
                    return {k:semantic(v) for k,v in value.items() if k not in ('url','targetUrl','baseUrl')}
                return semantic(assets)
            if args.reopen:
                saved=json.loads(Path(args.snapshot).read_text(encoding='utf-8'))
                assert await snapshots()==saved,'Fresh editor differs from saved assets/base references/local override'
                await call('asset_inspect',dict(assetId=ids['Unreferenced']),True)
                assert not (await state())['dirtyAssets']
            else:
                capabilities=(await call('editor_diagnostics'))['data']
                assert capabilities['unsupportedResourceOperations']==['import','reimport']
                for name in ('CON','../escape','ModelB','bad/name'):
                    await rejected('asset_rename',assetId=ids['ModelA'],name=name)
                await rejected('asset_delete',assetId=fixture['uneditableId'])
                await rejected('asset_rename',assetId=fixture['uneditableId'],name='Uneditable')
                await rejected('asset_rename',assetId='00000000-0000-0000-0000-000000000001',name='Wrong')
                before=await state();await call('asset_delete',dict(assetId=ids['Unreferenced'],expectedRevision=-1),True);assert (await state())['revision']==before['revision']
                await mutate('asset_rename',assetId=ids['ModelA'],name='RenamedModel')
                assert (await inspect('Prefab'))['references'][0]['targetUrl'].endswith('/RenamedModel')
                await mutate('asset_rename',assetId=ids['Material'],name='RenamedMaterial')
                assert (await inspect('ModelA'))['references'][0]['targetUrl'].endswith('/RenamedMaterial')
                await history('undo');assert (await inspect('ModelA'))['references'][0]['targetUrl'].endswith('/Material')
                await history('redo');assert (await inspect('ModelA'))['references'][0]['targetUrl'].endswith('/RenamedMaterial')
                for target,dependent in (('ModelA','Prefab'),('BaseMaterial','DerivedMaterial')):
                    error=await rejected('asset_delete',assetId=ids[target]);assert ids[dependent] in json.dumps(error)
                await mutate('asset_delete',assetId=ids['Unreferenced']);await call('asset_inspect',dict(assetId=ids['Unreferenced']),True)
                await history('undo');assert (await inspect('Unreferenced'))['id']==ids['Unreferenced']
                await history('redo');await call('asset_inspect',dict(assetId=ids['Unreferenced']),True)
                await history('undo')
                for prefab,position in (('Overflow',dict(X=3.4028234663852886e38,Y=0,Z=0)),('Cycle',dict(X=0,Y=0,Z=0)),('Material',dict(X=0,Y=0,Z=0)),('Prefab',dict(X=1))):
                    await rejected('prefab_place',sceneId=ids['Target'],prefabId=ids[prefab],position=position)
                await rejected('prefab_place',sceneId=ids['Material'],prefabId=ids['Prefab'],position=dict(X=0,Y=0,Z=0))
                await rejected('prefab_place',sceneId=ids['Target'],prefabId='00000000-0000-0000-0000-000000000001',position=dict(X=0,Y=0,Z=0))
                placed=[]
                for x in (2,-2): placed.append(await mutate('prefab_place',sceneId=ids['Target'],prefabId=ids['Prefab'],position=dict(X=x,Y=0,Z=0)))
                scene=await inspect('Target'); entity_map={e['id']:e for e in scene['entities']}
                prefab_url=(await inspect('Prefab'))['url']
                for placement,expected in zip(placed,(3,-1)):
                    root=placement['rootEntityIds'][0]; entity=entity_map[root]
                    transform=next(c for c in entity['components'] if c['type']=='Stride.Engine.TransformComponent')
                    assert {k:transform['properties']['Position'][k] for k in ('X','Y','Z')}==dict(X=expected,Y=2,Z=3)
                    parts=[p for p in scene['parts'] if p['instanceId']==placement['instanceId']]
                    assert len(parts)==2 and all(p['baseAssetId']==ids['Prefab'] for p in parts)
                    assert all(p['baseUrl']==prefab_url for p in parts)
                    assert any(p['parentId']==root for p in parts)
                all_ids=[value for e in scene['entities'] for value in [e['id']]+[c['id'] for c in e['components']]]
                assert len(all_ids)==len(set(all_ids))
                await history('undo');assert not set(placed[1]['entityIds'])&{e['id'] for e in (await inspect('Target'))['entities']}
                await history('redo');assert {e['id'] for e in (await inspect('Target'))['entities']}==set(entity_map)
                base_ref=(await inspect('Prefab'))['references'][0]
                async def reference(owner,ref,target):
                    await mutate('asset_set_reference',assetId=ids[owner],property='Model',entityId=ref['entityId'],componentId=ref['componentId'],targetAssetId=ids[target])
                await reference('Prefab',base_ref,'ModelB')
                scene=await inspect('Target');refs=scene['references'];assert all(r['targetAssetId']==ids['ModelB'] for r in refs)
                assert (await inspect('ModelB'))['references'][0]['targetAssetId']==ids['Material']
                color=dict(R=.125,G=.25,B=.5,A=1)
                await mutate('asset_set_property',assetId=ids['Material'],property='Value',value=color)
                assert {k:(await inspect('Material'))['fields']['Value'][k] for k in color}==color
                local=next(r for r in refs if r['entityId']==placed[0]['rootEntityIds'][0])
                await reference('Target',local,'ModelC');await reference('Prefab',base_ref,'ModelA')
                refs=(await inspect('Target'))['references'];assert {r['entityId']:r['targetAssetId'] for r in refs}=={placed[0]['rootEntityIds'][0]:ids['ModelC'],placed[1]['rootEntityIds'][0]:ids['ModelA']}
                assert (await inspect('ModelA'))['references'][0]['targetAssetId']==ids['Material']
                assert (await inspect('ModelC'))['references'][0]['targetAssetId']==ids['BaseMaterial']
                await mutate('asset_rename',assetId=ids['Prefab'],name='PlacedSource')
                assert all(p['baseUrl'].endswith('/PlacedSource') for p in (await inspect('Target'))['parts'] if p['baseAssetId'])
                error=await rejected('asset_delete',assetId=ids['Prefab']);assert ids['Target'] in json.dumps(error)
                await mutate('asset_delete',assetId=ids['Unreferenced']);await save()
                Path(args.snapshot).write_text(json.dumps(await snapshots(),ensure_ascii=False,indent=2),encoding='utf-8')
            evidence['passed']=True
    finally: Path(args.output).write_text(json.dumps(evidence,ensure_ascii=False,indent=2),encoding='utf-8')

asyncio.run(main())
