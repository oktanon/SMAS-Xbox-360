# Generates src/app/app.ico: an original abstract icon (star on a rounded tile).
import math, struct, zlib, sys

def png(w, h, px):
    raw = b''.join(b'\x00' + bytes(px[y*w*4:(y+1)*w*4]) for y in range(h))
    def chunk(t, d):
        c = struct.pack('>I', len(d)) + t + d
        return c + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)
    return (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 6, 0, 0, 0))
            + chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b''))

def inside_star(x, y, cx, cy, R, r):
    ang = math.atan2(y - cy, x - cx)
    d = math.hypot(x - cx, y - cy)
    # star radius at this angle (5 points, one pointing up)
    a = (ang + math.pi / 2) % (2 * math.pi / 5)
    t = abs(a - math.pi / 5) / (math.pi / 5)
    lim = r + (R - r) * t
    return d <= lim

def render(n):
    ss = 4
    px = []
    for y in range(n):
        for x in range(n):
            acc = [0, 0, 0, 0]
            for sy in range(ss):
                for sx in range(ss):
                    fx = (x + (sx + 0.5) / ss) / n
                    fy = (y + (sy + 0.5) / ss) / n
                    # rounded square tile
                    m = 0.06; rad = 0.2
                    qx = max(abs(fx - 0.5) - (0.5 - m - rad), 0)
                    qy = max(abs(fy - 0.5) - (0.5 - m - rad), 0)
                    if math.hypot(qx, qy) > rad:
                        continue
                    g = 1.0 - 0.35 * fy
                    col = (int(228 * g), int(48 * g), int(40 * g))
                    if inside_star(fx, fy, 0.5, 0.53, 0.36, 0.15):
                        col = (255, 208, 48) if fy < 0.5 else (240, 180, 30)
                    acc[0] += col[0]; acc[1] += col[1]; acc[2] += col[2]; acc[3] += 255
            k = ss * ss
            a = acc[3] // k
            if a:
                px += [acc[0] // (acc[3] // 255), acc[1] // (acc[3] // 255), acc[2] // (acc[3] // 255), a]
            else:
                px += [0, 0, 0, 0]
    return png(n, n, px)

sizes = [16, 24, 32, 48, 64, 128, 256]
imgs = [render(s) for s in sizes]
out = struct.pack('<HHH', 0, 1, len(sizes))
off = 6 + 16 * len(sizes)
for s, d in zip(sizes, imgs):
    out += struct.pack('<BBBBHHII', s % 256, s % 256, 0, 0, 1, 32, len(d), off)
    off += len(d)
out += b''.join(imgs)
open(sys.argv[1], 'wb').write(out)
