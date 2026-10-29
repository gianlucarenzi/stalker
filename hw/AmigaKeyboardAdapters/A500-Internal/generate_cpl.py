import csv
import os
import pcbnew
import re
import sys
from decimal import Decimal, getcontext
from pathlib import Path

ref_ignore = ["TP", "T", "NT", "REF**", "G", "H"]
getcontext().prec = 30

# original rotation db from:
# https://github.com/matthewlai/JLCKicadTools/blob/master/jlc_kicad_tools/cpl_rotations_db.csv
rotations = {
    "^SOT-223": 180,
    "^SOT-23": 180,
    "^SOT-353": 180,
    "^QFN-": 270,
    "^LQFP-": 270,
    "^TQFP-": 270,
    "^SOP-(?!18_)": 270,
    "^TSSOP-": 270,
    # "^DFN-": 270,
    "^SOIC-": 90,
    "^SOP-18_": 0,
    "^VSSOP-10_": 270,
    "^CP_EIA-3216-18_": 180,
    "^CP_Elec_8x10.5": 180,
    "^CP_Elec_6.3x7.7": 180,
    "^CP_Elec_8x6.7": 180,
    "^(.*?_|V)?QFN-(16|20|24|28|40)(-|_|$)": 270,
    "^MSOP-10_": 90,
    "^R_Array_Convex_4x0603": 90,
    "^XCVR_ESP32-WROVER-B": 270,
    "^PinSocket_1x(04|05)_P2.54mm_Vertical": 270,
    "Buzzer_MLT-8530_C94599": 0,
    "SW_Tactile_SPST_Angled_PTS645Vx58-2LFS": 180,
    "USB_C_Receptacle_HRO_TYPE-C-31-M-12": 180,
    "USB_Micro-B_Molex-105017-0001": 270,
}

midpoint_correction = {
    "^PinSocket_1x04_P2.54mm_Vertical": (Decimal(0), Decimal(-3.81)),
    "^PinSocket_1x05_P2.54mm_Vertical": (Decimal(0), Decimal(-5.08)),
    "^XCVR_ESP32-WROVER-B": (Decimal(0), Decimal(0.04)),
    "BarrelJack": (Decimal(-6.5), Decimal(0)),
    "^SW_SPST_HRO": (Decimal(0), Decimal(1.65)),
    "USB_C_Receptacle_HRO_TYPE-C-31-M-12": (Decimal(1.8), Decimal(0.65)),
    "SW_Tactile_SPST_Angled_PTS645Vx58-2LFS": (Decimal(2.2), Decimal(-1)),
}

def pi():
    getcontext().prec += 2
    three = Decimal(3)
    lasts, t, s, n, na, d, da = 0, three, 3, 1, 0, 0, 24
    while s != lasts:
        lasts = s
        n, na = n+na, na+8
        d, da = d+da, da+32
        t = (t * n) / d
        s += t
    getcontext().prec -= 2
    return +s

def cos(x):
    getcontext().prec += 2
    i, lasts, s, fact, num, sign = 0, 0, 1, 1, 1, 1
    while s != lasts:
        lasts = s
        i += 2
        fact *= i * (i-1)
        num *= x * x
        sign *= -1
        s += num / fact * sign
    getcontext().prec -= 2
    return +s

def sin(x):
    getcontext().prec += 2
    i, lasts, s, fact, num, sign = 1, 0, x, 1, x, 1
    while s != lasts:
        lasts = s
        i += 2
        fact *= i * (i-1)
        num *= x * x
        sign *= -1
        s += num / fact * sign
    getcontext().prec -= 2
    return +s

def generate_cpl_files(board_file):
    board = pcbnew.LoadBoard(board_file)
    modules = board.GetModules()

    fn = Path(board.GetFileName()).with_suffix("")

    bot = open("{}_cpl_bot.csv".format(fn), "w", newline='')
    top = open("{}_cpl_top.csv".format(fn), "w", newline='')
    botw = csv.writer(bot, delimiter=',', quotechar='"',
                      quoting=csv.QUOTE_ALL)
    topw = csv.writer(top, delimiter=',', quotechar='"',
                      quoting=csv.QUOTE_ALL)

    hdr = ["Designator", "Mid X", "Mid Y", "Layer", "Rotation"]
    botw.writerow(hdr)
    topw.writerow(hdr)

    for mod in modules:
        skip = False
        ref = mod.GetReference()

        for prefix in ref_ignore:
            if ref.startswith(prefix):
                skip = True

        if skip:
            continue

        pos = mod.GetPosition()
        rot = mod.GetOrientationDegrees()
        layer = board.GetLayerName(mod.GetLayer())
        mid_x = Decimal(pos[0]) / Decimal(1000000)
        mid_y = Decimal(pos[1]) / Decimal(-1000000)
        footprint = str(mod.GetFPID().GetLibItemName())

        for exp in midpoint_correction:
            if re.match(exp, footprint):
                px, py = midpoint_correction[exp]
                rad = Decimal(rot) * pi() / Decimal(180)

                qx = cos(rad) * px - sin(rad) * py
                qy = sin(rad) * px + cos(rad) * py

                qx = qx.quantize(Decimal('0.001'))
                qy = qy.quantize(Decimal('0.001'))

                mid_x += qx
                mid_y += qy
        
        for exp in rotations:
            if re.match(exp, footprint):
                rot = (rot + rotations[exp]) % 360

        x = str(mid_x) + "mm"
        y = str(mid_y) + "mm"

        if layer == "F.Cu":
            topw.writerow([ref, x, y, "top", rot])
        elif layer == "B.Cu":
            botw.writerow([ref, x, y, "bottom", rot])

    bot.close()
    top.close()

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python generate_cpl.py <path_to_kicad_pcb_file>")
        sys.exit(1)
    
    board_file = sys.argv[1]
    if not os.path.exists(board_file):
        print("Error: File not found at {}".format(board_file))
        sys.exit(1)

    generate_cpl_files(board_file)
    print("Placement files generated.")
