"""Extracts the vector map-marker glyphs from the game's interface files into glyphs.js.

    python extract.py "<Skyrim Special Edition>\\Data" [ffdec.jar] [java]

Needs `pip install lz4` and JPEXS FFDec (Java). Reads `Skyrim - Interface.bsa`, pulls out
map.swf (all map markers, exported as `<Type>Marker` sprites) plus a few single shapes from
other menus (EXTRA), converts their shapes to SVG with FFDec and writes glyphs.js next to this script.
The glyphs are kept as-is; only the two game colours are swapped for CSS variables.
"""
import json, os, re, struct, subprocess, sys, tempfile
import xml.etree.ElementTree as ET
import lz4.frame

HERE = os.path.dirname(os.path.abspath(__file__))
SKIP = {'MarkerButton', 'YouAreHereMarker', 'QuestTargetMarker', 'QuestTargetDoorMarker',
        'MultipleQuestTargetMarker', 'PlayerSetMarker', 'DoorMarker', 'EmptyMarker'}
# Single shapes from other menus: (menu swf, shape id) -> glyph name
EXTRA = {
    ('statsmenu', '19'): 'SkyrimLogo',  # dragon logo above the skill constellations
    ('hudmenu', '535'): 'SneakEye',     # stealth eye over the crosshair
}


def bsa_extract(path, wanted, outdir):
    """Minimal SSE BSA (v105) reader: writes files whose path ends with one of `wanted`."""
    with open(path, 'rb') as f:
        _, ver, _, aflags, nfold, _, _, lfile, _ = struct.unpack('<4sIIIIIIII', f.read(36))
        assert ver == 105, f'unsupported BSA version {ver}'
        counts = [struct.unpack('<QIIQ', f.read(24))[1] for _ in range(nfold)]
        entries = []
        for count in counts:
            folder = f.read(f.read(1)[0])[:-1].decode('cp1252')
            entries += [(folder, *struct.unpack('<QII', f.read(16))[1:]) for _ in range(count)]
        names = f.read(lfile).split(b'\0')
        out = {}
        for (folder, size, off), name in zip(entries, names):
            full = folder + '\\' + name.decode('cp1252')
            hit = next((w for w in wanted if full.endswith(w)), None)
            if not hit:
                continue
            compressed = bool(aflags & 4) ^ bool(size & 0x40000000)
            size &= 0x3FFFFFFF
            f.seek(off)
            if aflags & 0x100:  # embedded file name
                n = f.read(1)[0]
                f.read(n)
                size -= n + 1
            data = f.read(size)
            if compressed:
                data = lz4.frame.decompress(data[4:])
            out[hit] = os.path.join(outdir, hit.rsplit('\\', 1)[-1])
            with open(out[hit], 'wb') as o:
                o.write(data)
        return out


def ffdec(java, jar, *args):
    subprocess.run([java, '-Djna.nosys=true', '-jar', jar, *args], check=True, stdout=subprocess.DEVNULL)


def marker_shapes(xml_path):
    """{MarkerName: shapeId} from the ExportAssets sprites of map.swf."""
    tags = ET.parse(xml_path).getroot().find('tags')
    exports = {}
    for t in tags:
        if t.get('type') == 'ExportAssetsTag':
            for i, n in zip(t.find('tags'), t.find('names')):
                exports[i.text] = n.text
    sprites = {t.get('spriteId'): t for t in tags if t.get('type') == 'DefineSpriteTag'}
    shapes = {t.get('shapeId') for t in tags if 'Shape' in (t.get('type') or '')}
    def first_shape(cid, depth=0):  # markers nest the icon shape in a sprite or two
        if cid in shapes:
            return cid
        if cid not in sprites or depth > 4:
            return None
        for t in sprites[cid].find('subTags'):
            if t.get('type') == 'ShowFrameTag':
                break
            if t.get('characterId') and (found := first_shape(t.get('characterId'), depth + 1)):
                return found
        return None

    out = {}
    for sid, name in exports.items():
        if name.endswith('Marker') and name not in SKIP and (shape := first_shape(sid)):
            out[name[:-len('Marker')]] = shape
    return out


def glyph(svg_path):
    s = open(svg_path, encoding='utf-8').read()
    w = float(re.search(r'width="([\d.]+)px"', s).group(1))
    h = float(re.search(r'height="([\d.]+)px"', s).group(1))
    body = s[s.index('<g'):s.rindex('</svg>')]
    body = re.sub(r'fill="#(?:bcbec0|f5f5f5|993300)"', 'style="fill:var(--fg)"', body)
    body = re.sub(r'fill="#(?:000000|231f20)"', 'style="fill:var(--bg)"', body)
    return {'w': w, 'h': h, 'svg': re.sub(r'\s+', ' ', body).strip()}


def main():
    data = sys.argv[1]
    jar = sys.argv[2] if len(sys.argv) > 2 else r'C:\Program Files (x86)\FFDec\ffdec.jar'
    java = sys.argv[3] if len(sys.argv) > 3 else 'java'
    with tempfile.TemporaryDirectory() as tmp:
        menus = ['map'] + sorted({menu for menu, _ in EXTRA})
        swf = bsa_extract(os.path.join(data, 'Skyrim - Interface.bsa'), [f'interface\\{m}.swf' for m in menus], tmp)
        for m in menus:
            ffdec(java, jar, '-format', 'shape:svg', '-export', 'shape', os.path.join(tmp, m), swf[f'interface\\{m}.swf'])
        ffdec(java, jar, '-swf2xml', swf['interface\\map.swf'], os.path.join(tmp, 'map.xml'))
        glyphs = {name: glyph(os.path.join(tmp, 'map', sid + '.svg'))
                  for name, sid in sorted(marker_shapes(os.path.join(tmp, 'map.xml')).items())}
        for (menu, sid), name in EXTRA.items():
            glyphs[name] = glyph(os.path.join(tmp, menu, sid + '.svg'))
    with open(os.path.join(HERE, 'glyphs.js'), 'w', encoding='utf-8', newline='\n') as f:
        f.write('// Generated by extract.py from the game\'s interface .swf files. Do not edit.\n')
        f.write('const GAME_GLYPHS = {\n')
        f.write(',\n'.join(f'  {json.dumps(k)}: {json.dumps(v)}' for k, v in glyphs.items()))
        f.write('\n};\n')
    print(f'{len(glyphs)} glyphs -> glyphs.js')


main()
