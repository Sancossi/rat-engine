"""Derive bounded native QA assets from the authored maps; never edit source assets.

The generated package adds these fixtures to the ordinary Stride compiler roots.
This is a fixture builder for our pinned SceneAsset text format, not a YAML editor.
Only the exact scalar properties below are changed, with unique-match assertions.
"""
import argparse
import re
import uuid
from pathlib import Path

GAME = Path(__file__).resolve().parents[1]
ASSETS = GAME / 'Rat.Expedition.Authoring/Assets'
NS = uuid.UUID('6673f6d8-002b-4284-85a3-f673242f5a2d')
# Stable fixture addresses in the authored Courtyard. Display names may change.
IDS = {'Editor ambient': '018dce00-7665-5a24-9203-7c74d00fd5d6',
 'courtyard-ladder/topExit': '078b3f27-4068-505d-b113-086456b15be4',
 'bridge-post': '08f1d20a-6502-5fae-9e76-a399dfba8b4c',
 'entry': '0afba030-34f8-5fa1-9173-1135966f8e68',
 'north-boundary': '1c4f9d9a-9eed-54a2-957d-151a3a003a14',
 'arch-cut': '1de46740-6e43-53b4-bab2-f0d51f81fb35',
 'courtyard-ladder/bottomEntry': '219c6e1f-22b8-517c-aa36-b1fc39ce22f1',
 'east-pillar': '2fea8448-3e23-553a-ac37-da9577b3ba31',
 'bridge-deck': '3b3d2c74-248c-5224-919d-983065577ecf',
 'bridge-rail': '3fa26a33-f109-5b38-919b-48d1c84f68de',
 'courtyard-wall': '64e053ae-e44a-530f-a63a-824bf7c62343',
 'courtyard-ladder': '66d92055-563c-5756-ac4a-41063622cc9b',
 'passage-left-post': '6c0d0478-b954-57df-89f5-14987619fb24',
 'bridge-crate': '6e2c9e86-46e4-58c1-9058-d0cb8dcbe3dc',
 'low-ceiling': '6e37c5dc-0cff-5b49-9f8e-02863e3a4c05',
 'expedition_courtyard': '83513e47-0a32-54e1-8a67-a95efe621e55',
 'to-sluice': '8fb5a79f-3fb4-5a3e-955f-319cb346520a',
 'west-boundary': '9564bfa5-106f-51e6-aeb5-b8fd38706f77',
 'north-independent': '997abfe4-ee73-54e6-92f9-f2021a18d804',
 'courtyard-ladder/top': 'a8250111-fca8-51e2-a7fd-6e0f19532fd5',
 'passage-right-post': 'ae7c65cc-8c83-5b87-b39f-2696e7462f9e',
 'courtyard-ladder/bottom': 'bd7af0df-a21d-5ad6-bfaf-63a54eeb1598',
 'bridge-rail-cut': 'c35eba36-1f36-5155-b98c-ca376f45d359',
 'from-sluice': 'd185ae4a-8177-59e0-9b03-49a9ad2148d5',
 'bridge-cut': 'd1ab8473-07bf-573d-9c4b-23342392c4c5',
 'floor': 'dc4e6f8a-3556-5b89-ac7c-3787bbb970b0',
 'courtyard-ladder/bottomExit': 'eb00935f-8ca8-576c-a543-8227d377515f',
 'courtyard-ladder/topEntry': 'f86aea0d-a87f-5a12-82cb-3b9fdc44bded',
 'upper-platform': 'fd7ad7bd-eb6c-55aa-b761-c6680030b30e',
 'bridge-ramp': 'fffd4a8c-c072-53b9-ae43-85448ab0d402'}


def guid(name):
    return str(uuid.uuid5(NS, name))


class Scene:
    def __init__(self, path):
        text = path.read_text(encoding='utf-8-sig')
        self.parts = re.split(r'(?m)^        -   Entity:\n', text)[1:]
        self.roots=re.findall(r'(?m)^        - ref!! ([0-9a-f-]+)$',text)
        assert self.parts and text.startswith('!SceneAsset\n')

    def name(self, part):
        identity=re.search(r'(?m)^                Id: (.+)$',part)[1]
        return next((name for name,value in IDS.items() if value==identity),re.search(r'(?m)^                Name: (.+)$',part)[1])

    def id(self, name):
        return re.search(r'(?m)^                Id: (.+)$', self.part(name))[1]

    def part(self, name):
        found = [p for p in self.parts if self.name(p) == name]
        assert len(found) == 1, name
        return found[0]

    def set(self, name, key, value):
        before = self.part(name)
        after, count = re.subn(r'(?m)^(                        ' + key + r': ).*$',
                               lambda m: m[1] + value, before)
        assert count == 1, (name, key, count)
        self.parts[self.parts.index(before)] = after

    def keep(self, names):
        self.parts = [p for p in self.parts if self.name(p) in names]

    def without(self, tag):
        self.parts = [p for p in self.parts if tag not in p]

    def box(self, name, position, size):
        p = self.part('courtyard-wall')
        for old in set(re.findall(r'[0-9a-f]{8}-[0-9a-f-]{27}|[0-9a-f]{32}', p)):
            p = p.replace(old, guid(name + old).replace('-', '') if len(old) == 32 else guid(name + old))
        p = re.sub(r'(?m)^(                Name: ).*$',lambda m:m[1]+name,p)
        self.parts.append(p)
        self.roots.append(re.search(r'(?m)^                Id: (.+)$',p)[1])
        self.set(name, 'Position', position)
        self.set(name, 'Size', size)

    def native_visual(self, name, position, model, geometry, selectors, replaces=True):
        entity_id = guid(name + '-entity')
        entries = ''.join('                            ' + guid(name + node).replace('-', '') + ': ' + node + '\n' for node in selectors)
        part = f'''                Id: {entity_id}
                Name: {name}
                Components:
                    {guid(name+'-transform').replace('-', '')}: !TransformComponent
                        Id: {guid(name+'-transform-id')}
                        Position: {position}
                        Rotation: {{X: 0, Y: 0, Z: 0, W: 1}}
                        Scale: {{X: 1, Y: 1, Z: 1}}
                        Children: {{}}
                    {guid(name+'-model').replace('-', '')}: !ModelComponent
                        Id: {guid(name+'-model-id')}
                        Model: {model}
                        Materials: {{}}
                    {guid(name+'-binding').replace('-', '')}: !ExpeditionNativeVisual
                        Id: {guid(name+'-binding-id')}
                        GeometryId: {geometry}
                        ReplacesGeometry: {str(replaces).lower()}
                        SkeletonNodes:
{entries}'''
        self.parts.append(part)
        self.roots.append(entity_id)

    def write(self, output, name):
        ids = [re.search(r'(?m)^                Id: (.+)$', p)[1] for p in self.parts]
        text = '!SceneAsset\nId: ' + guid(name) + '\nSerializedVersion: {Stride: 3.1.0.1}\nTags: []\nChildrenIds: []\nOffset: {X: 0, Y: 0, Z: 0}\nHierarchy:\n    RootParts:\n'
        text += ''.join('        - ref!! ' + i + '\n' for i in self.roots if i in ids)
        text += '    Parts:\n' + ''.join('        -   Entity:\n' + p for p in self.parts)
        (output / (name + '.sdscene')).write_text(text, encoding='utf8')


def manifest(output, name, scenes):
    # Build-time native references, not a runtime JSON override.
    template = (ASSETS / 'ExpeditionProject.sdscene').read_text(encoding='utf8')
    template = template.replace('2ecc6dd7-3fc3-592d-8abe-8bac6974f435', guid(name))
    template = re.sub(r'(?m)^(                        StartScene: ).*$', lambda m: m[1] + scenes[0], template)
    template = template.split('                        Scenes:')[0] + '                        Scenes:\n'
    template += ''.join('                            ' + guid(name + str(i)).replace('-', '') + ': ' + ref + '\n' for i, ref in enumerate(scenes))
    (output / (name + '.sdscene')).write_text(template, encoding='utf8')


def build(output):
    output.mkdir(parents=True, exist_ok=True)
    roots = []
    cases = ['edges', 'large-y', 'recovery-courtyard', 'recovery-sluice', 'upper-void', 'native-visuals',
             'invalid-size', 'missing-spawn', 'blocked-ladder-exit', 'missing-ladder-point',
             'invalid-ramp', 'bad-portal-target', 'bad-occlusion-parent', 'invalid-rotation', 'invalid-scale']
    for case in cases:
        scene = Scene(ASSETS / ('Sluice.sdscene' if case == 'recovery-sluice' else 'Courtyard.sdscene'))
        if case == 'edges':
            scene.box('edge-test-east', '{X: 18.5625, Y: 2.25, Z: .5625}', '{X: 1.125, Y: 4.5, Z: 28.125}')
            scene.box('edge-test-south', '{X: .5625, Y: 2.25, Z: 14.0625}', '{X: 37.125, Y: 4.5, Z: 1.125}')
        elif case == 'native-visuals':
            model='7453c19f-4830-5209-a02e-9828735efc32:CanalCity/bridge_arch'
            geometry=scene.id('bridge-deck')
            position='{X: 3.375, Y: 0, Z: 5}'
            scene.native_visual('native-bridge-deck',position,model,geometry,['bridge_arch_deck'])
            scene.native_visual('native-bridge-ironwork',position,model,geometry,['bridge_arch_ironwork'])
            scene.native_visual('native-bridge-posts',position,model,geometry,['bridge_arch_posts'])
        elif case == 'invalid-size': scene.set('courtyard-wall', 'Size', '{X: 0, Y: 4.05, Z: 1.125}')
        elif case == 'missing-spawn': scene.set('expedition_courtyard', 'DefaultSpawn', 'null')
        elif case == 'blocked-ladder-exit': scene.set('courtyard-ladder/topExit', 'Position', '{X: 11.25, Y: 3.6, Z: 0}')
        elif case == 'missing-ladder-point': scene.set('courtyard-ladder', 'Top', 'null')
        elif case == 'invalid-ramp': scene.set('bridge-ramp', 'Size', '{X: 0, Y: .45, Z: 2.7}')
        elif case == 'bad-portal-target': scene.set('to-sluice', 'TargetSpawnId', guid('absent spawn'))
        elif case == 'bad-occlusion-parent': scene.set('bridge-rail-cut', 'HideWith', 'ref!! ' + scene.id('entry'))
        elif case == 'invalid-rotation': scene.set('courtyard-wall', 'Rotation', '{X: 0, Y: .1, Z: 0, W: .995}')
        elif case == 'invalid-scale': scene.set('courtyard-wall', 'Scale', '{X: 2, Y: 1, Z: 1}')
        elif case.startswith('recovery-'): scene.without('!ExpeditionPortal')
        elif case == 'upper-void':
            scene.keep({'expedition_courtyard', 'floor', 'entry', 'upper-platform'})
            scene.set('floor', 'Position', '{X: -4.5, Y: -.3375, Z: 0}')
            scene.set('floor', 'Size', '{X: 9, Y: .675, Z: 27}')
            scene.set('upper-platform', 'Position', '{X: 5.625, Y: 3.375, Z: 4.5}')
            scene.set('upper-platform', 'Size', '{X: 6.75, Y: .45, Z: 4.5}')
            scene.set('entry', 'Position', '{X: 5.625, Y: 3.6, Z: 4.5}')
        elif case == 'large-y':
            scene.keep({'expedition_courtyard', 'floor', 'entry', 'upper-platform', 'courtyard-ladder'} |
                       {scene.name(p) for p in scene.parts if scene.name(p).startswith('courtyard-ladder/')})
            # Float ULP .5 here; use exactly representable one-unit-thick boxes.
            scene.set('floor', 'Position', '{X: 0, Y: 4194303.5, Z: 0}')
            scene.set('floor', 'Size', '{X: 36, Y: 1, Z: 27}')
            scene.set('entry', 'Position', '{X: 3.375, Y: 4194304, Z: 6.75}')
            scene.set('upper-platform', 'Position', '{X: -9.9, Y: 4194307.5, Z: -6.75}')
            scene.set('upper-platform', 'Size', '{X: 6.3, Y: 1, Z: 4.5}')
            for point in ('bottom','bottomEntry','bottomExit','top','topEntry','topExit'):
                part=scene.part('courtyard-ladder/' + point)
                position=re.search(r'(?m)^                        Position: (.+)$',part)[1]
                scene.set('courtyard-ladder/'+point,'Position',re.sub(r'Y: [^,}]+', 'Y: '+('4194308' if point.startswith('top') else '4194304'),position))
        scene_name = 'Scene-' + case
        scene.write(output, scene_name)
        # Keep normal companion scene refs available for production portal validation.
        refs = [guid(scene_name) + ':QA/' + scene_name]
        if case not in ('large-y', 'upper-void', 'recovery-courtyard', 'recovery-sluice'):
            refs.append('70789606-338b-58d8-88ef-86bf53250749:Sluice')
            # Target Sluice points back to the ordinary Courtyard, included as catalog entry.
            refs.append('a6d301f9-3171-5137-b3c7-4622618bebc5:Courtyard')
            # Scene ids must also be unique inside a project, including its QA clone.
            old=scene.id('expedition_courtyard')
            path=output/(scene_name+'.sdscene')
            path.write_text(path.read_text(encoding='utf8').replace(old,guid(scene_name+'root')),encoding='utf8')
        manifest(output, case, refs)
        roots.append(guid(case) + ':QA/' + case)
    missing='missing-start-scene'
    manifest(output,missing,['70789606-338b-58d8-88ef-86bf53250749:Sluice'])
    path=output/(missing+'.sdscene')
    text=path.read_text(encoding='utf8').replace('StartScene: 70789606-338b-58d8-88ef-86bf53250749:Sluice',
                                 'StartScene: a6d301f9-3171-5137-b3c7-4622618bebc5:Courtyard')
    path.write_text(text,encoding='utf8')
    roots.append(guid(missing)+':QA/'+missing)
    # Core candidate remains valid; its visual preparation fails independently.
    bad_visual=Scene(ASSETS/'Sluice.sdscene')
    bad_visual.native_visual('bad-native-binding','{X: 0, Y: 0, Z: 0}',
        '7453c19f-4830-5209-a02e-9828735efc32:CanalCity/bridge_arch',bad_visual.id('floor'),['absent-node'])
    bad_scene='Scene-bad-native-binding';bad_visual.write(output,bad_scene)
    # Preserve the production Sluice root identity so a production Core candidate resolves it.
    bad_path=output/(bad_scene+'.sdscene')
    bad_path.write_text(bad_path.read_text(encoding='utf8').replace(guid(bad_scene),'70789606-338b-58d8-88ef-86bf53250749',1),encoding='utf8')
    manifest(output,'bad-native-binding',['70789606-338b-58d8-88ef-86bf53250749:QA/'+bad_scene,
        'a6d301f9-3171-5137-b3c7-4622618bebc5:Courtyard'])
    roots.append(guid('bad-native-binding')+':QA/bad-native-binding')
    package = (GAME/'Rat.Expedition.Authoring/Rat.Expedition.Authoring.sdpkg').read_text(encoding='utf8')
    package = package.replace('Path: !dir Assets', 'Path: !dir ' + ASSETS.as_posix())
    package = package.replace('ResourceFolders:', '    - Path: !dir ' + output.parent.as_posix() + '\nResourceFolders:')
    package = package.replace('!dir Resources', '!dir ' + (GAME/'Rat.Expedition.Authoring/Resources').as_posix())
    package += ''.join('    - ' + r + '\n' for r in roots)
    result = output.parent/'Rat.Expedition.Qualification.sdpkg'
    result.write_text(package,encoding='utf8')
    print(result)


if __name__ == '__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('--output',type=Path,required=True)
    build(parser.parse_args().output.resolve())
