"""Editable original prototype art. Rebuild with python games/rat-expedition/tools/rat_sprite.py.

Eight hand-authored palette/pixel frames, no external image inputs or dependencies.
Rows: front, left, right, back. Foot baseline y=43; cells 32x48.
"""
from pathlib import Path
import struct
import zlib

PALETTE = {
    'outline': (28, 25, 35, 255), 'fur': (121, 111, 113, 255),
    'light': (184, 174, 157, 255), 'shadow': (72, 67, 81, 255),
    'ear': (176, 112, 124, 255), 'cloak': (75, 68, 99, 255),
    'hem': (109, 96, 137, 255), 'belt': (155, 117, 57, 255),
    'eye': (244, 205, 110, 255), 'nose': (44, 32, 42, 255),
}

def frame(direction, step):
    pixels = [[(0, 0, 0, 0) for _ in range(32)] for _ in range(48)]
    def pixel(x, y, color):
        if 0 <= x < 32 and 0 <= y < 48:
            pixels[y][x] = PALETTE[color]
    def rect(x0, y0, x1, y1, color):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1): pixel(x, y, color)
    def ellipse(cx, cy, rx, ry, color):
        for y in range(cy-ry, cy+ry+1):
            for x in range(cx-rx, cx+rx+1):
                if (x-cx)**2 / rx**2 + (y-cy)**2 / ry**2 <= 1: pixel(x,y,color)
    # Curved exposed rat tail, boots and travelling cloak.
    for x,y in [(22,35),(24,36),(26,37),(28,38),(29,40),(28,42),(26,43),(24,43)]:
        rect(x,y,x+1,y+1,'outline'); pixel(x,y,'ear')
    rect(10,39+step,13,43,'outline'); rect(18,40-step,21,43,'outline')
    rect(11,40+step,14,42,'shadow'); rect(18,40-step,22,42,'shadow')
    ellipse(16,29,9,12,'outline'); ellipse(16,29,8,11,'cloak')
    rect(10,37,22,39,'hem'); rect(9,28,23,30,'belt')
    rect(14,23,15,36,'hem'); rect(17,29,18,30,'eye')
    ellipse(8,29-step,2,5,'outline'); ellipse(8,29-step,1,3,'fur')
    ellipse(24,29+step,2,5,'outline'); ellipse(24,29+step,1,3,'fur')
    if direction in (0,3):
        ellipse(10,11,5,6,'outline'); ellipse(22,11,5,6,'outline')
        ellipse(10,11,3,4,'ear'); ellipse(22,11,3,4,'ear')
        ellipse(16,17,8,9,'outline'); ellipse(16,17,7,8,'fur')
        if direction == 0:
            ellipse(16,22,5,4,'light'); rect(15,24,17,25,'nose')
            pixel(11,17,'eye'); pixel(20,17,'eye')
            rect(5,22,9,22,'light'); rect(23,22,27,22,'light')
        else:
            ellipse(16,17,6,7,'shadow'); rect(14,13,15,22,'fur')
    else:
        ellipse(17,11,5,6,'outline'); ellipse(17,11,3,4,'ear')
        ellipse(16,18,7,8,'outline'); ellipse(16,18,6,7,'fur')
        ellipse(10,22,7,4,'outline'); ellipse(10,22,6,3,'light')
        rect(3,22,5,23,'nose'); pixel(11,17,'eye')
        rect(2,25,8,25,'light')
    return [list(reversed(row)) for row in pixels] if direction == 2 else pixels

def atlas_bytes():
    rows = []
    for direction in range(4):
        frames = [frame(direction, step) for step in range(2)]
        for y in range(48):
            rows.append(b'\0' + bytes(channel for cell in frames for rgba in cell[y] for channel in rgba))
    def chunk(kind, data):
        return struct.pack('>I',len(data)) + kind + data + struct.pack('>I',zlib.crc32(kind+data))
    return b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR',struct.pack('>IIBBBBB',64,192,8,6,0,0,0)) + chunk(b'IDAT',zlib.compress(b''.join(rows),9)) + chunk(b'IEND',b'')

if __name__ == '__main__':
    target = Path(__file__).resolve().parents[1] / 'Content/sprites/rat.png'
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes(atlas_bytes())
    print(target)
