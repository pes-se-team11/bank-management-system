# -*- coding: utf-8 -*-
"""Render the two UML use-case diagrams to SVG, PNG and PDF (Bank Management System).

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


def stereo(src, dst, kind, t=0.30):
    """Dashed, open-arrowed dependency. kind is 'include' or 'extend'.

    Several use cases here are shared hubs with fan-in from three bases, so
    include lines necessarily cross. Two things keep that readable: the label
    sits at `t` along the line rather than the midpoint, which spreads labels
    apart because the sources are at different heights; and the text is drawn
    with a white halo so a line passing behind it does not run through the
    glyphs. Pass a different `t` to nudge one label clear of another.
    """
    sx, sy = _edge_point(src[0], src[1], dst[0], dst[1])
    dx, dy = _edge_point(dst[0], dst[1], src[0], src[1])
    mx, my = sx + (dx - sx) * t, sy + (dy - sy) * t
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
    # The halo is a separate underlay rather than paint-order="stroke":
    # cairosvg ignores paint-order and would paint the white stroke over the
    # fill, erasing the label.
    common = (f'font-family="{FONT}" font-size="11.5" font-style="italic"')
    label = f'&#171;{kind}&#187;'
    return (f'<line x1="{sx:.1f}" y1="{sy:.1f}" x2="{dx:.1f}" y2="{dy:.1f}" '
            f'stroke="{LINE}" stroke-width="1.5" stroke-dasharray="7,5" '
            f'marker-end="url(#open)"/>\n'
            f'<text {placement} {common} fill="#FFFFFF" stroke="#FFFFFF" '
            f'stroke-width="4" stroke-linejoin="round">{label}</text>\n'
            f'<text {placement} {common} fill="{LINE}">{label}</text>')


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
    """Customer-facing account transactions."""
    W, H = 1200, 800
    uc01 = (470, 155)   # Deposit
    uc02 = (470, 300)   # Withdraw Cash
    uc03 = (470, 445)   # Funds Transfer
    uc04 = (470, 590)   # Balance Inquiry
    uc05 = (470, 710)   # Mini Statement
    uc07 = (800, 225)   # Record Ledger Entry   - included by 01, 02, 03
    uc06 = (800, 415)   # Validate Sufficient Balance - included by 02, 03
    uc08 = (800, 600)   # Apply Overdraft       - extends 02

    customer = (135, 290)
    teller = (135, 600)

    body = [
        boundary(275, 60, 955, 770, "Bank Management System"),
        actor(customer[0], customer[1], "Customer"),
        actor(teller[0], teller[1], "Bank Teller"),
        usecase(*uc01, "UC-01", "Deposit"),
        usecase(*uc02, "UC-02", "Withdraw Cash"),
        usecase(*uc03, "UC-03", "Funds Transfer"),
        usecase(*uc04, "UC-04", "Balance Inquiry"),
        usecase(*uc05, "UC-05", "Mini Statement"),
        usecase(*uc07, "UC-07", "Record Ledger Entry"),
        usecase(*uc06, "UC-06", "Validate Sufficient Balance"),
        usecase(*uc08, "UC-08", "Apply Overdraft"),
        assoc(customer[0] + 20, customer[1] + 30, uc01),
        assoc(customer[0] + 20, customer[1] + 30, uc02),
        assoc(customer[0] + 20, customer[1] + 30, uc03),
        assoc(customer[0] + 20, customer[1] + 30, uc04),
        assoc(customer[0] + 20, customer[1] + 30, uc05),
        assoc(teller[0] + 20, teller[1] + 30, uc01),
        assoc(teller[0] + 20, teller[1] + 30, uc02),
        stereo(uc01, uc07, "include"),
        stereo(uc02, uc07, "include"),
        stereo(uc02, uc06, "include", t=0.62),
        stereo(uc03, uc06, "include"),
        stereo(uc03, uc07, "include", t=0.55),
        stereo(uc08, uc02, "extend"),
    ]
    return W, H, svg(W, H, "\n".join(body),
                     "Use-Case Diagram 1 - Account Transactions")


# --------------------------------------------------------------------------- diagram 2

def diagram_two():
    """Account administration, audit and reporting."""
    W, H = 1200, 880
    uc09 = (470, 150)   # Open Account
    uc11 = (470, 290)   # Modify Customer Details
    uc10 = (470, 430)   # Close Account
    uc12 = (470, 570)   # Unlock Locked Account
    uc13 = (470, 700)   # Generate End-of-Day Report
    uc14 = (470, 820)   # View Audit Log
    uc17 = (800, 215)   # Authenticate User      - included by 09, 11
    uc16 = (800, 365)   # Verify Zero Balance    - included by 10
    uc15 = (800, 505)   # Record Audit Entry     - included by 10, 11, 12
    uc18 = (800, 645)   # Transfer Residual Balance - extends 10

    teller = (135, 190)
    manager = (135, 600)

    body = [
        boundary(275, 60, 955, 850, "Bank Management System"),
        actor(teller[0], teller[1], "Bank Teller"),
        actor(manager[0], manager[1], "Bank Manager"),
        usecase(*uc09, "UC-09", "Open Account"),
        usecase(*uc11, "UC-11", "Modify Customer Details"),
        usecase(*uc10, "UC-10", "Close Account"),
        usecase(*uc12, "UC-12", "Unlock Locked Account"),
        usecase(*uc13, "UC-13", "Generate End-of-Day Report"),
        usecase(*uc14, "UC-14", "View Audit Log"),
        usecase(*uc17, "UC-17", "Authenticate User"),
        usecase(*uc16, "UC-16", "Verify Zero Balance"),
        usecase(*uc15, "UC-15", "Record Audit Entry"),
        usecase(*uc18, "UC-18", "Transfer Residual Balance"),
        assoc(teller[0] + 20, teller[1] + 30, uc09),
        assoc(teller[0] + 20, teller[1] + 30, uc11),
        assoc(manager[0] + 20, manager[1] + 30, uc10),
        assoc(manager[0] + 20, manager[1] + 30, uc12),
        assoc(manager[0] + 20, manager[1] + 30, uc13),
        assoc(manager[0] + 20, manager[1] + 30, uc14),
        stereo(uc09, uc17, "include"),
        stereo(uc11, uc17, "include"),
        stereo(uc10, uc16, "include"),
        stereo(uc10, uc15, "include"),
        stereo(uc11, uc15, "include", t=0.78),
        stereo(uc12, uc15, "include"),
        stereo(uc18, uc10, "extend"),
    ]
    return W, H, svg(W, H, "\n".join(body),
                     "Use-Case Diagram 2 - Account Administration, Audit & Reporting")


# --------------------------------------------------------------------------- main

def main():
    import cairosvg
    os.makedirs(OUT, exist_ok=True)
    for name, fn in (("UseCase_1_Transactions", diagram_one),
                     ("UseCase_2_Administration", diagram_two)):
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
