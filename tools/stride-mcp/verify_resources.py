"""Official MCP client proof for opt-in temporary native resource fixtures. No images/input."""
import argparse
import asyncio
import json
from pathlib import Path
from mcp import Client
from mcp.client.stdio import StdioServerParameters


async def main():
    parser = argparse.ArgumentParser()
    for name in ('server', 'connection', 'fixture', 'output'):
        parser.add_argument('--' + name, required=True)
    args = parser.parse_args()
    fixture = json.loads(Path(args.fixture).read_text(encoding='utf-8-sig'))
    ids = fixture['assets']
    evidence = {'calls': [], 'passed': False}
    try:
        async with Client(StdioServerParameters(command=args.server, args=['--connection', args.connection]), read_timeout_seconds=20) as client:
            evidence['protocol'] = client.protocol_version
            evidence['tools'] = [tool.name for tool in (await client.list_tools()).tools]
            assert len(evidence['tools']) == 16
            async def call(name, arguments=None, error=False):
                response = await client.call_tool(name, arguments or {})
                value = json.loads(next(c.text for c in response.content if c.type == 'text'))
                evidence['calls'].append(dict(tool=name, arguments=arguments, result=value, isError=response.is_error))
                assert bool(response.is_error) == error, (name, value)
                return value
            async def state():
                await asyncio.sleep(.15)
                return (await call('editor_status'))['data']
            async def inspect(name):
                return (await call('asset_inspect', {'assetId': ids[name]}))['data']
            async def set_field(name, key, value, **extra):
                return await call('asset_set_property', dict(assetId=ids[name], property=key, value=value, expectedRevision=(await state())['revision'], **extra))
            async def undo_redo(direction):
                current = await state()
                return await call('editor_' + direction, dict(expectedRevision=current['revision'], expectedTransactionId=current[direction + 'TransactionId']))
            catalog = (await call('asset_list'))['data']
            assert set(ids.values()).issubset({a['id'] for a in catalog['assets']})
            original = await inspect('TextureA')
            await set_field('TextureA', 'Width', 75)
            assert (await inspect('TextureA'))['fields']['Width'] == 75
            assert (await inspect('TextureB'))['fields']['Width'] == 100
            await undo_redo('undo')
            assert (await inspect('TextureA'))['fields']['Width'] == original['fields']['Width']
            await undo_redo('redo')
            assert (await inspect('TextureA'))['fields']['Width'] == 75
            await undo_redo('undo')

            # Finite JSON doubles may overflow native floats; reject without graph,
            # dirty, revision or Undo changes, not merely with an eventual build error.
            for name, key, value, extra in (
                ('Sheet', 'Center', {'X': 1e40, 'Y': 43}, {'itemIndex': 0}),
                ('Model', 'PivotPosition', {'X': 0, 'Y': 1e40, 'Z': 0}, {}),
                ('Material', 'Value', {'R': 1e40, 'G': .2, 'B': .3, 'A': 1}, {}),
                ('Sheet', 'Center', {'X': 1}, {'itemIndex': 0}),
                ('TextureA', 'NotAProperty', 1, {}),
                ('Sound', 'CompressionRatio', -1, {}),
                ('Sound', 'CompressionRatio', 0, {}),
                ('Sound', 'CompressionRatio', 41, {}),
                ('Sound', 'SampleRate', -44100, {}),
                ('Sound', 'SampleRate', 0, {}),
            ):
                before = await state(); snapshot = await inspect(name)
                await call('asset_set_property', dict(assetId=ids[name], property=key, value=value, expectedRevision=before['revision'], **extra), error=True)
                after = await state()
                for field in ('revision', 'dirtyAssets', 'undoTransactionId', 'redoTransactionId'):
                    assert after[field] == before[field], (name, field)
                assert await inspect(name) == snapshot
            await set_field('Sheet', 'Center', {'X': 17, 'Y': 43}, itemIndex=0)
            assert (await inspect('Sheet'))['sprites'][0]['Center']['X'] == 17
            await undo_redo('undo')
            await set_field('Material', 'Value', {'R': .4, 'G': .6, 'B': .2, 'A': 1})
            await set_field('Font', 'Size', 22)
            page = await inspect('Page'); element = page['ui'][0]['id']
            await set_field('Page', 'Text', 'Ресурсы через MCP: ёж', elementId=element)
            assert (await inspect('Page'))['ui'][0]['text'] == 'Ресурсы через MCP: ёж'
            for name, key, target, extra in (
                ('Model', 'Material', 'MaterialB', {'itemIndex': 0}),
                ('Scene', 'Model', 'ModelB', {k: v for k, v in (await inspect('Scene'))['references'][0].items() if k in ('entityId', 'componentId')}),
            ):
                await call('asset_set_reference', dict(assetId=ids[name], property=key, targetAssetId=ids[target], expectedRevision=(await state())['revision'], **extra))
                assert any(r['targetAssetId'] == ids[target] for r in (await inspect(name))['references'])
                await undo_redo('undo')
            before = await state()
            await call('asset_set_reference', dict(assetId=ids['Model'], property='Material', itemIndex=0, targetAssetId=ids['TextureA'], expectedRevision=before['revision']), error=True)
            await call('asset_set_property', dict(assetId=ids['TextureA'], property='Width', value=60, expectedRevision=-1), error=True)
            await call('asset_inspect', {'assetId': '00000000-0000-0000-0000-000000000001'}, error=True)
            assert (await state())['revision'] == before['revision']
            operation = await call('save_session', {'expectedRevision': (await state())['revision']})
            for _ in range(100):
                outcome = (await call('editor_operation', {'operationId': operation['operationId']}))['data']
                if outcome['state'] == 'completed':
                    assert outcome['success'], outcome
                    break
                await asyncio.sleep(.1)
            else:
                raise AssertionError('Native save timeout')
            assert not (await state())['dirtyAssets']
            evidence['passed'] = True
    except Exception as error:
        evidence['error'] = repr(error)
        raise
    finally:
        Path(args.output).write_text(json.dumps(evidence, ensure_ascii=False, indent=2), encoding='utf-8')


if __name__ == '__main__':
    asyncio.run(main())
