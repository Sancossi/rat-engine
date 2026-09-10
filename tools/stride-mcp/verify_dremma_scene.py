"""Open, edit, save, and freshly reopen the production Dremma scene via MCP."""
import argparse
import asyncio
import json
from pathlib import Path

from mcp import Client
from mcp.client.stdio import StdioServerParameters


async def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--server', required=True)
    parser.add_argument('--connection', required=True)
    parser.add_argument('--ready', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--baseline')
    args = parser.parse_args()
    ready = json.loads(Path(args.ready).read_text(encoding='utf-8-sig'))
    evidence = {'passed': False, 'calls': []}
    try:
        async with Client(StdioServerParameters(command=args.server, args=['--connection', args.connection]), read_timeout_seconds=30) as client:
            tools = [tool.name for tool in (await client.list_tools()).tools]
            assert len(tools) == 19, tools

            async def call(name, arguments=None):
                response = await client.call_tool(name, arguments or {})
                value = json.loads(next(content.text for content in response.content if content.type == 'text'))
                evidence['calls'].append({'tool': name, 'arguments': arguments, 'result': value, 'isError': response.is_error})
                assert not response.is_error, (name, value)
                return value

            async def status():
                await asyncio.sleep(.15)
                return (await call('editor_status'))['data']

            async def complete(operation):
                for _ in range(200):
                    result = (await call('editor_operation', {'operationId': operation['operationId']}))['data']
                    if result['state'] == 'completed':
                        assert result['success'], result
                        return
                    await asyncio.sleep(.1)
                raise AssertionError('Editor operation exceeded 20 seconds')

            async def snapshot():
                return (await call('scene_inspect', {'sceneId': ready['sceneId']}))['data']

            catalog = (await call('scene_list'))['data']
            scene = next(item for item in catalog if item['id'] == ready['sceneId'])
            assert scene['name'] == 'Dremma'
            await complete(await call('scene_open', {'sceneId': ready['sceneId']}))
            current = await snapshot()
            entity_ids = sorted(entity['id'] for entity in current['entities'])
            assert entity_ids == ready['entityIds']
            asset = (await call('asset_inspect', {'assetId': ready['sceneId']}))['data']
            native_models = sorted(reference['targetAssetId'] for reference in asset['references'] if reference['targetAssetId'])
            identity = {'sceneId': current['sceneId'], 'entityIds': entity_ids, 'nativeModelIds': native_models}

            if args.baseline:
                baseline = json.loads(Path(args.baseline).read_text(encoding='utf-8-sig'))
                assert identity == baseline['identity']
                assert not (await status())['dirtyAssets']
                evidence.update({'identity': identity, 'freshReopen': True, 'passed': True})
                return

            wall = next(entity for entity in current['entities'] if entity['name'] == 'canal-wall')
            transform = next(component for component in wall['components'] if component['type'] == 'Stride.Engine.TransformComponent')
            original = transform['properties']['Position']
            assert (original['X'], original['Y'], original['Z']) == (0, 0, 13)
            address = {'sceneId': ready['sceneId'], 'entityId': wall['id'], 'componentId': transform['id'], 'property': 'Position'}
            await call('entity_set_property', dict(address, value={'X': 0, 'Y': 0, 'Z': 12.9}, expectedRevision=(await status())['revision']))
            changed = await snapshot()
            changed_wall = next(entity for entity in changed['entities'] if entity['id'] == wall['id'])
            changed_position = next(component for component in changed_wall['components'] if component['id'] == transform['id'])['properties']['Position']
            assert abs(changed_position['Z'] - 12.9) < .0001
            assert (changed_position['X'], changed_position['Y'], changed_position['Z']) != (original['X'], original['Y'], original['Z'])
            state = await status()
            await call('editor_undo', {'expectedRevision': state['revision'], 'expectedTransactionId': state['undoTransactionId']})
            state = await status()
            await call('editor_redo', {'expectedRevision': state['revision'], 'expectedTransactionId': state['redoTransactionId']})
            state = await status()
            await call('editor_undo', {'expectedRevision': state['revision'], 'expectedTransactionId': state['undoTransactionId']})
            restored = await snapshot()
            restored_wall = next(entity for entity in restored['entities'] if entity['id'] == wall['id'])
            restored_position = next(component for component in restored_wall['components'] if component['id'] == transform['id'])['properties']['Position']
            assert (restored_position['X'], restored_position['Y'], restored_position['Z']) == (0, 0, 13)
            await complete(await call('save_session', {'expectedRevision': (await status())['revision']}))
            assert not (await status())['dirtyAssets']
            evidence.update({'identity': identity, 'editedEntityId': wall['id'], 'beforePosition': {key: original[key] for key in ('X','Y','Z')},
                             'editedPosition': {key: changed_position[key] for key in ('X','Y','Z')},
                             'negativePositionOracle': True, 'undoRedo': True, 'saved': True, 'passed': True})
    except Exception as error:
        evidence['error'] = repr(error)
        raise
    finally:
        Path(args.output).write_text(json.dumps(evidence, ensure_ascii=False, indent=2), encoding='utf-8')


if __name__ == '__main__':
    asyncio.run(main())
