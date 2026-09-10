"""Qualify Dremma library copies in the normal Authoring Game Studio session."""
import argparse
import asyncio
import json
from pathlib import Path

from mcp import Client
from mcp.client.stdio import StdioServerParameters


async def main():
    parser = argparse.ArgumentParser()
    for name in ('server', 'connection', 'fixture', 'output', 'edit-output', 'reimport'):
        parser.add_argument('--' + name, required=True)
    parser.add_argument('--reopen', action='store_true')
    args = parser.parse_args()
    fixture_path = Path(args.fixture)
    output_path = Path(args.output)
    edit_path = Path(args.edit_output)
    reimport_path = Path(args.reimport)
    fixture = json.loads(fixture_path.read_text(encoding='utf-8-sig'))
    ids = fixture['assets']
    evidence = {'passed': False, 'calls': []}
    edit_written = False
    try:
        async with Client(StdioServerParameters(command=args.server, args=['--connection', args.connection]), read_timeout_seconds=20) as client:
            evidence['protocol'] = client.protocol_version
            evidence['tools'] = [tool.name for tool in (await client.list_tools()).tools]
            assert len(evidence['tools']) == 19, evidence['tools']

            async def call(name, arguments=None, error=False):
                response = await client.call_tool(name, arguments or {})
                value = json.loads(next(content.text for content in response.content if content.type == 'text'))
                evidence['calls'].append(dict(tool=name, arguments=arguments, result=value, isError=response.is_error))
                assert bool(response.is_error) == error, (name, value)
                return value

            async def state():
                await asyncio.sleep(.15)
                return (await call('editor_status'))['data']

            async def inspect(name):
                return (await call('asset_inspect', {'assetId': ids[name]}))['data']

            async def history(direction):
                current = await state()
                await call('editor_' + direction, dict(expectedRevision=current['revision'], expectedTransactionId=current[direction + 'TransactionId']))

            async def save():
                operation = await call('save_session', {'expectedRevision': (await state())['revision']})
                for _ in range(150):
                    outcome = (await call('editor_operation', {'operationId': operation['operationId']}))['data']
                    if outcome['state'] == 'completed':
                        assert outcome['success'], outcome
                        break
                    await asyncio.sleep(.1)
                else:
                    raise AssertionError('Native save exceeded 15 seconds')
                assert not (await state())['dirtyAssets']

            async def open_scene():
                operation = await call('scene_open', {'sceneId': ids['Scene']})
                for _ in range(200):
                    outcome = (await call('editor_operation', {'operationId': operation['operationId']}))['data']
                    if outcome['state'] == 'completed':
                        assert outcome['success'], outcome
                        break
                    await asyncio.sleep(.1)
                else:
                    raise AssertionError('Native scene open exceeded 20 seconds')
                scenes = (await call('scene_list'))['data']
                assert next(scene for scene in scenes if scene['id'] == ids['Scene'])['opened']

            async def fixture_snapshot():
                return {name: await inspect(name) for name in ids}

            def placed_positions(scene):
                entities = {entity['id']: entity for entity in scene['entities']}
                positions = set()
                for part in scene['parts']:
                    if part['baseAssetId'] != ids['Prefab']:
                        continue
                    transform = next(component for component in entities[part['entityId']]['components'] if component['type'] == 'Stride.Engine.TransformComponent')
                    value = transform['properties']['Position']
                    positions.add((value['X'], value['Y'], value['Z']))
                return positions

            catalog = (await call('asset_list'))['data']
            catalog_ids = {asset['id'] for asset in catalog['assets']}
            assert set(ids.values()).issubset(catalog_ids)
            # The bounded catalog exposes 81 supported resource types from the
            # 97 editor-loaded assets. Skeletons, clips, compositor and the
            # procedural preview model are intentionally outside this MCP slice.
            assert len([asset for asset in catalog['assets'] if '/CanalCity/' in asset['url'].replace('\\', '/')]) == 81
            await open_scene()

            if args.reopen:
                snapshot = await fixture_snapshot()
                model = snapshot['Model']
                assert [reference['targetAssetId'] for reference in model['references'] if reference['targetAssetId']] == [ids['SharedMaterial']] * 3
                assert {reference['targetAssetId'] for reference in snapshot['Prefab']['references']} == {ids['Model']}
                scene = snapshot['Scene']
                parts = [part for part in scene['parts'] if part['baseAssetId'] == ids['Prefab']]
                assert len(parts) == 2 and len({part['instanceId'] for part in parts}) == 2
                assert placed_positions(scene) == {(-3, 0, 0), (3, 0, 0)}
                assert snapshot['SharedMaterial']['fields']['Value'] == {'R': .2, 'G': .35, 'B': .8, 'A': 1}
                assert not (await state())['dirtyAssets']
                evidence['snapshot'] = snapshot
                evidence['passed'] = True
                return

            baseline = await fixture_snapshot()
            assert baseline['SharedMaterial']['fields']['Value'] == {'R': .8, 'G': .45, 'B': .12, 'A': 1}
            assert {reference['targetAssetId'] for reference in baseline['Prefab']['references']} == {ids['Model']}
            assert ids['Texture'] in {dependency['id'] for dependency in baseline['TextureMaterial']['dependencies']}

            color = {'R': .2, 'G': .35, 'B': .8, 'A': 1}
            await call('asset_set_property', dict(assetId=ids['SharedMaterial'], property='Value', value=color, expectedRevision=(await state())['revision']))
            assert (await inspect('SharedMaterial'))['fields']['Value'] == color
            await history('undo')
            assert (await inspect('SharedMaterial'))['fields']['Value'] == baseline['SharedMaterial']['fields']['Value']
            await history('redo')
            assert (await inspect('SharedMaterial'))['fields']['Value'] == color

            placements = []
            for x in (-3, 3):
                response = await call('prefab_place', dict(sceneId=ids['Scene'], prefabId=ids['Prefab'], position={'X': x, 'Y': 0, 'Z': 0}, expectedRevision=(await state())['revision']))
                placements.append(response['data'])
            scene = await inspect('Scene')
            placed = [part for part in scene['parts'] if part['baseAssetId'] == ids['Prefab']]
            assert len(placed) == 2 and len({part['instanceId'] for part in placed}) == 2
            assert len({part['entityId'] for part in placed}) == 2
            assert placed_positions(scene) == {(-3, 0, 0), (3, 0, 0)}
            await history('undo')
            after_undo = await inspect('Scene')
            assert len([part for part in after_undo['parts'] if part['baseAssetId'] == ids['Prefab']]) == 1
            assert placed_positions(after_undo) == {(-3, 0, 0)}
            await history('redo')
            after_redo = await inspect('Scene')
            assert len([part for part in after_redo['parts'] if part['baseAssetId'] == ids['Prefab']]) == 2
            assert placed_positions(after_redo) == {(-3, 0, 0), (3, 0, 0)}
            await save()
            edit_path.write_text(json.dumps({'passed': True, 'placements': placements}, ensure_ascii=False, indent=2), encoding='utf-8')
            edit_written = True

            for _ in range(900):
                if reimport_path.exists():
                    break
                await asyncio.sleep(.1)
            else:
                raise AssertionError('Real source reimport exceeded 90 seconds')
            evidence['reimport'] = json.loads(reimport_path.read_text(encoding='utf-8-sig'))
            reimported = await inspect('Model')
            assert [reference['targetAssetId'] for reference in reimported['references'] if reference['targetAssetId']].count(ids['SharedMaterial']) == 3
            assert {reference['targetAssetId'] for reference in (await inspect('Prefab'))['references']} == {ids['Model']}
            assert ids['Texture'] in {dependency['id'] for dependency in (await inspect('TextureMaterial'))['dependencies']}
            await history('undo')
            assert len((await inspect('Model'))['references']) == len(fixture['initialSlots'])
            await history('redo')
            assert len((await inspect('Model'))['references']) == 7
            await save()
            evidence['finalSnapshot'] = await fixture_snapshot()
            evidence['passed'] = True
    except Exception as error:
        evidence['error'] = repr(error)
        if not edit_written:
            edit_path.write_text(json.dumps({'passed': False, 'error': repr(error)}, ensure_ascii=False, indent=2), encoding='utf-8')
        raise
    finally:
        output_path.write_text(json.dumps(evidence, ensure_ascii=False, indent=2), encoding='utf-8')


if __name__ == '__main__':
    asyncio.run(main())
