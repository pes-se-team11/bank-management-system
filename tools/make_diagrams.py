# -*- coding: utf-8 -*-
"""Render the two UML use-case diagrams to SVG, PNG and PDF.

    python tools/make_diagrams.py

Everything is emitted from the LAYOUT tables below, so a diagram change is a
data edit rather than a fight with a drawing tool. PNG is what gets embedded
in the DOCX; PDF is the submittable copy; SVG is the editable source.
"""
import os
import math

OUT = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "diagrams")

FONT = "Segoe UI, Calibri, Helvetica, Arial, sans-serif"
INK = "#111111"
LINE = "#333333"
FILL = "#F4F6F8"
STROKE = "#2B4C7E"
BOUND = "#8A94A6"

RX, RY = 112, 34          # use-case ellipse radii
ACTOR_H = 74              # stick figure height


# --------------------------------------------------------------------------- helpers

def esc(s):
    return (s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;"))


def wrap(text, width=20):
    words, lines, cur = text.split(), [], ""
    for w in words:
        trial = (cur + " " + w).strip()
        if len(trial) <= width:
            cur = trial
        else:
            if cur:
                lines.append(cur)
            cur = w
    if cur:
        lines.append(cur)
    return lines


def actor(x, y, label):
    """Stick figure whose (x, y) is the centre of the head."""
    head_r = 12
    body_top = y + head_r
    body_bot = body_top + 30
    parts = [
        f'<circle cx="{x}" cy="{y}" r="{head_r}" fill="none" stroke="{INK}" stroke-width="2"/>',
        f'<line x1="{x}" y1="{body_top}" x2="{x}" y2="{body_bot}" stroke="{INK}" stroke-width="2"/>',
        f'<line x1="{x-18}" y1="{body_top+11}" x2="{x+18}" y2="{body_top+11}" stroke="{INK}" stroke-width="2"/>',
        f'<line x1="{x}" y1="{body_bot}" x2="{x-15}" y2="{body_bot+24}" stroke="{INK}" stroke-width="2"/>',
        f'<line x1="{x}" y1="{body_bot}" x2="{x+15}" y2="{body_bot+24}" stroke="{INK}" stroke-width="2"/>',
    ]
    ly = body_bot + 44
    for i, ln in enumerate(wrap(label, 16)):
        parts.append(
            f'<text x="{x}" y="{ly + i*15}" font-family="{FONT}" font-size="13" '
            f'font-weight="600" fill="{INK}" text-anchor="middle">{esc(ln)}</text>')
    return "\n".join(parts)


def usecase(x, y, uid, label):
    lines = wrap(label, 19)
    n = len(lines) + 1                      # +1 for the id line
    start = y - (n - 1) * 8
    parts = [f'<ellipse cx="{x}" cy="{y}" rx="{RX}" ry="{RY}" fill="{FILL}" '
             f'stroke="{STROKE}" stroke-width="1.8"/>',
             f'<text x="{x}" y="{start}" font-family="{FONT}" font-size="11" '
             f'font-weight="700" fill="{STROKE}" text-anchor="middle">{esc(uid)}</text>']
    for i, ln in enumerate(lines):
        parts.append(
            f'<text x="{x}" y="{start + 15 + i*14}" font-family="{FONT}" font-size="12.5" '
            f'fill="{INK}" text-anchor="middle">{esc(ln)}</text>')
    return "\n".join(parts)


def _edge_point(cx, cy, tx, ty):
    """Where the line from (tx,ty) meets the ellipse centred at (cx,cy)."""
    dx, dy = tx - cx, ty - cy
    d = math.hypot(dx, dy) or 1.0
    k = 1.0 / math.hypot(dx / (RX * d), dy / (RY * d)) if d else 0
    return cx + dx / d * k, cy + dy / d * k


def assoc(ax, ay, uc):
    """Plain association: actor to use case."""
    cx, cy = uc
    px, py = _edge_point(cx, cy, ax, ay)
    return (f'<line x1="{ax:.1f}" y1="{ay:.1f}" x2="{px:.1f}" y2="{py:.1f}" '
            f'stroke="{LINE}" stroke-width="1.6"/>')


def stereo(src, dst, kind):
    """Dashed, open-arrowed dependency. kind is 'include' or 'extend'."""
    sx, sy = _edge_point(src[0], src[1], dst[0], dst[1])
    dx, dy = _edge_point(dst[0], dst[1], src[0], src[1])
    mx, my = (sx + dx) / 2, (sy + dy) / 2
    ang = math.degrees(math.atan2(dy - sy, dx - sx))
    if ang > 90:
        ang -= 180
    elif ang < -90:
        ang += 180
    # Steep edges get a horizontal label set beside the line; rotating one to
    # near-vertical is technically correct and miserable to read.
    if abs(ang) > 55:
        placement = (f'x="{mx+10:.1f}" y="{my:.1f}" text-anchor="start"')
    else:
        placement = (f'x="{mx:.1f}" y="{my-6:.1f}" text-anchor="middle" '
                     f'transform="rotate({ang:.1f} {mx:.1f} {my:.1f})"')
    return (f'<line x1="{sx:.1f}" y1="{sy:.1f}" x2="{dx:.1f}" y2="{dy:.1f}" '
            f'stroke="{LINE}" stroke-width="1.5" stroke-dasharray="7,5" '
            f'marker-end="url(#open)"/>\n'
            f'<text {placement} font-family="{FONT}" font-size="11.5" '
            f'font-style="italic" fill="{LINE}">&#171;{kind}&#187;</text>')


def boundary(x1, y1, x2, y2, title):
    return (f'<rect x="{x1}" y="{y1}" width="{x2-x1}" height="{y2-y1}" rx="6" '
            f'fill="none" stroke="{BOUND}" stroke-width="2"/>\n'
            f'<text x="{(x1+x2)/2}" y="{y1+26}" font-family="{FONT}" font-size="14" '
            f'font-weight="700" fill="{BOUND}" text-anchor="middle">{esc(title)}</text>')


def svg(width, height, body, caption):
    return f'''<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">
<defs>
  <marker id="open" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="9" markerHeight="9" orient="auto-start-reverse">
    <path d="M 0 0 L 10 5 L 0 10" fill="none" stroke="{LINE}" stroke-width="1.6"/>
  </marker>
</defs>
<rect width="{width}" height="{height}" fill="#FFFFFF"/>
<text x="{width/2}" y="30" font-family="{FONT}" font-size="16" font-weight="700" fill="{INK}" text-anchor="middle">{esc(caption)}</text>
{body}
</svg>'''


# --------------------------------------------------------------------------- diagram 1

def diagram_one():
    W, H = 1120, 760
    uc03 = (430, 130)
    uc01 = (430, 268)
    uc08 = (430, 400)
    uc02 = (430, 532)
    uc09 = (430, 664)
    uc06 = (740, 330)
    uc07 = (740, 470)

    host = (150, 100)
    guest = (150, 430)
    svc = (975, 470)

    body = [
        boundary(280, 62, 900, 720, "Podcast Guest Scheduling & Outline Builder"),
        actor(host[0], host[1], "Show Host"),
        actor(guest[0], guest[1], "Podcast Guest"),
        actor(svc[0], svc[1], "Calendar & Notification Service"),
        usecase(*uc03, "UC-03", "Publish Availability"),
        usecase(*uc01, "UC-01", "Book Interview Slot"),
        usecase(*uc08, "UC-08", "Request Reschedule"),
        usecase(*uc02, "UC-02", "Submit Talking-Point Outline"),
        usecase(*uc09, "UC-09", "Attach Biography Links"),
        usecase(*uc06, "UC-06", "Validate Slot Availability"),
        usecase(*uc07, "UC-07", "Send Booking Notification"),
        assoc(host[0] + 20, host[1] + 30, uc03),
        assoc(guest[0] + 20, guest[1] + 30, uc01),
        assoc(guest[0] + 20, guest[1] + 30, uc02),
        stereo(uc01, uc06, "include"),
        stereo(uc01, uc07, "include"),
        stereo(uc08, uc01, "extend"),
        stereo(uc09, uc02, "extend"),
        assoc(svc[0] - 20, svc[1] + 30, uc07),
    ]
    return W, H, svg(W, H, "\n".join(body),
                     "Use-Case Diagram 1 - Scheduling & Guest Content Submission")


# --------------------------------------------------------------------------- diagram 2

def diagram_two():
    W, H = 1120, 760
    uc04 = (430, 150)
    uc13 = (740, 150)
    uc05 = (430, 330)
    uc11 = (740, 300)
    uc10 = (740, 430)
    uc12 = (430, 470)
    uc14 = (430, 610)
    uc15 = (430, 700)

    host = (150, 200)
    admin = (150, 590)
    svc = (975, 150)
    guest = (975, 560)

    body = [
        boundary(280, 62, 900, 745, "Podcast Guest Scheduling & Outline Builder"),
        actor(host[0], host[1], "Show Host"),
        actor(admin[0], admin[1], "Administrator"),
        actor(svc[0], svc[1], "Calendar & Notification Service"),
        actor(guest[0], guest[1], "Podcast Guest"),
        usecase(*uc04, "UC-04", "Review & Approve Outline"),
        usecase(*uc13, "UC-13", "Notify Approval Decision"),
        usecase(*uc05, "UC-05", "Generate Run-of-Show Sheet"),
        usecase(*uc11, "UC-11", "Recompute Segment Timestamps"),
        usecase(*uc10, "UC-10", "Export PDF Production Sheet"),
        usecase(*uc12, "UC-12", "Insert Host Segment"),
        usecase(*uc14, "UC-14", "Manage User Accounts"),
        usecase(*uc15, "UC-15", "View Audit Log"),
        assoc(host[0] + 20, host[1] + 30, uc04),
        assoc(host[0] + 20, host[1] + 30, uc05),
        assoc(admin[0] + 20, admin[1] + 30, uc14),
        assoc(admin[0] + 20, admin[1] + 30, uc15),
        stereo(uc04, uc13, "include"),
        stereo(uc05, uc11, "include"),
        stereo(uc05, uc10, "include"),
        stereo(uc12, uc05, "extend"),
        assoc(svc[0] - 20, svc[1] + 30, uc13),
        assoc(guest[0] - 20, guest[1] + 30, uc10),
    ]
    return W, H, svg(W, H, "\n".join(body),
                     "Use-Case Diagram 2 - Episode Production & Administration")


# --------------------------------------------------------------------------- main

def main():
    import cairosvg
    os.makedirs(OUT, exist_ok=True)
    for name, fn in (("UseCase_1_Scheduling", diagram_one),
                     ("UseCase_2_Production", diagram_two)):
        w, h, src = fn()
        svg_path = os.path.join(OUT, name + ".svg")
        with open(svg_path, "w", encoding="utf-8") as fh:
            fh.write(src)
        data = src.encode("utf-8")
        cairosvg.svg2png(bytestring=data, write_to=os.path.join(OUT, name + ".png"),
                         output_width=w * 2, output_height=h * 2)
        cairosvg.svg2pdf(bytestring=data, write_to=os.path.join(OUT, name + ".pdf"))
        print("wrote", name, "svg/png/pdf")


if __name__ == "__main__":
    main()
