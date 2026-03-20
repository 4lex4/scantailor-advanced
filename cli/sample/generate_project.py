#!/usr/bin/env python3
"""
Genera project.ScanTailor mínimo válido para las imágenes in/page1.png e in/page2.png.
Ejecutar desde la raíz del repo o desde cli/sample: python3 generate_project.py
"""
import os
import xml.etree.ElementTree as ET
from xml.dom import minidom

# Ruta al directorio del script
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
OUT_PATH = os.path.join(SCRIPT_DIR, "project.ScanTailor")

def elem(tag, parent=None, **attrs):
    e = ET.Element(tag)
    for k, v in attrs.items():
        if v is not None:
            e.set(k, str(v))
    if parent is not None:
        parent.append(e)
    return e

def sub(parent, tag, **attrs):
    return elem(tag, parent, **attrs)

def main():
    project = ET.Element("project", version="3", outputDirectory="out", layoutDirection="LTR")

    # directories
    dirs = sub(project, "directories")
    sub(dirs, "directory", id="1", path="in")

    # files
    files = sub(project, "files")
    sub(files, "file", id="1", dirId="1", name="page1.png")
    sub(files, "file", id="2", dirId="1", name="page2.png")

    # images (50x50, 300 DPI - igual que los PNG generados)
    images = sub(project, "images")
    for i, (fid, w, h) in enumerate([(1, 50, 50), (2, 50, 50)], start=1):
        img = sub(images, "image", id=str(i), subPages="1", fileId=str(fid), fileImage="1")
        sub(img, "size", width=str(w), height=str(h))
        sub(img, "dpi", horizontal="300", vertical="300")

    # pages (PAGE_VIEW: una página por imagen)
    pages_el = sub(project, "pages")
    sub(pages_el, "page", id="1", imageId="1", subPage="single", selected="selected")
    sub(pages_el, "page", id="2", imageId="2", subPage="single")

    # file-name-disambiguation (file = fileId como string, label único)
    disambig = sub(project, "file-name-disambiguation")
    sub(disambig, "mapping", file="1", label="1")
    sub(disambig, "mapping", file="2", label="2")

    # filters (mínimo para que loadSettings no falle; loadDefaultSettings rellena al ejecutar)
    filters_el = sub(project, "filters")

    fix_orient = sub(filters_el, "fix-orientation")
    for i in (1, 2):
        img = sub(fix_orient, "image", id=str(i))
        sub(img, "rotation", degrees="0")
    sub(fix_orient, "image-settings")

    sub(filters_el, "page-split", defaultLayoutType="single-uncut")

    deskew_el = sub(filters_el, "deskew")
    for pid in (1, 2):
        page = sub(deskew_el, "page", id=str(pid))
        params = sub(page, "params", mode="auto", angle="0", oblique="0")
        deps = sub(params, "dependencies")
        sub(deps, "rotation", degrees="0")
        outline = sub(deps, "page-outline")
        for x, y in [(0, 0), (50, 0), (50, 50), (0, 50)]:
            sub(outline, "point", x=str(x), y=str(y))
    sub(deskew_el, "image-settings")

    select_el = sub(filters_el, "select-content")
    sub(select_el, "page-detection-box", width="0", height="0")
    select_el.set("pageDetectionTolerance", "0.1")
    for pid in (1, 2):
        page = sub(select_el, "page", id=str(pid))
        params = sub(page, "params", contentDetectionMode="auto", pageDetectionMode="disabled", fineTuneCorners="0")
        sub(params, "content-rect", x="0", y="0", width="50", height="50")
        sub(params, "page-rect", x="0", y="0", width="50", height="50")
        sub(params, "content-size-mm", width="42.33", height="42.33")
        deps = sub(params, "dependencies")
        sub(deps, "rotation", degrees="0")

    layout_el = sub(filters_el, "page-layout")
    layout_el.set("showMiddleRect", "0")
    for pid in (1, 2):
        page = sub(layout_el, "page", id=str(pid))
        params = sub(page, "params", autoMargins="1")
        sub(params, "hardMarginsMM", left="0", top="0", right="0", bottom="0")
        sub(params, "contentRect", x="0", y="0", width="50", height="50")
        sub(params, "pageRect", x="0", y="0", width="50", height="50")
        sub(params, "contentSizeMM", width="42.33", height="42.33")
        sub(params, "alignment", vert="top", hor="left")

    output_el = sub(filters_el, "output")
    for pid in (1, 2):
        page = sub(output_el, "page", id=str(pid))
        sub(page, "zones")
        sub(page, "fill-zones")
        params = sub(page, "params", depthPerception="1", despeckleLevel="1", blackOnWhite="1")
        sub(params, "distortion-model")
        sub(params, "picture-shape-options", pictureShape="free", sensitivity="100", higherSearchSensitivity="0")
        sub(params, "dewarping-options", mode="off", postDeskew="1", postDeskewAngle="0")
        sub(params, "dpi", horizontal="300", vertical="300")
        cp = sub(params, "color-params", colorMode="bw")
        cg = sub(cp, "color-or-grayscale", fillOffcut="1", fillMargins="1", normalizeIlluminationColor="0",
                 fillingColor="background", wienerCoef="0", wienerWinSize="5")
        sub(cg, "posterization-options", enabled="0", level="4", normalizationEnabled="0", forceBlackAndWhite="1")
        bw_el = sub(cp, "bw", thresholdAdj="0", savitzkyGolaySmoothing="1", morphologicalSmoothing="1",
                    normalizeIlluminationBW="1", windowSize="200", binarizationMethod="otsu")
        sub(bw_el, "color-segmenter-options", enabled="0", noiseReduction="7", redThresholdAdjustment="0",
            greenThresholdAdjustment="0", blueThresholdAdjustment="0")
        sub(params, "splitting", splitOutput="0", splittingMode="bw", originalBackground="0")
        sub(page, "processing-params", autoZonesFound="0")

    rough = ET.tostring(project, default_namespace="", encoding="unicode")
    pretty = minidom.parseString(rough).toprettyxml(indent="  ", encoding=None)
    with open(OUT_PATH, "w", encoding="utf-8") as f:
        f.write(pretty)
    print("Written:", OUT_PATH)

if __name__ == "__main__":
    main()
