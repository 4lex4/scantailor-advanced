#!/usr/bin/env python3
"""
Genera project.ScanTailor desde un directorio de imágenes (todo en automático) y opcionalmente
ejecuta scantailor-cli para procesar sin abrir la GUI.

Uso:
  python3 create_project_from_dir.py DIR_IMAGENES [--output-dir OUT] [--project RUTA.ScanTailor] [--run] [--scantailor-cli RUTA]

Ejemplo:
  python3 create_project_from_dir.py ~/mios/libros-enriquecidos/outputs/imagenes --run
"""
import argparse
import os
import subprocess
import sys
import xml.etree.ElementTree as ET
from xml.dom import minidom

EXTENSIONES = (".tif", ".tiff", ".png", ".jpg", ".jpeg", ".bmp")

try:
    from PIL import Image
    HAS_PILLOW = True
except ImportError:
    HAS_PILLOW = False


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


def get_image_info(path):
    """Devuelve (width, height, dpi_x, dpi_y). Si no hay Pillow o falla, (2480, 3508, 300, 300)."""
    if not HAS_PILLOW:
        return (2480, 3508, 300, 300)
    try:
        with Image.open(path) as img:
            w, h = img.size
            dpi = img.info.get("dpi")
            if dpi and len(dpi) >= 2:
                if isinstance(dpi[0], tuple):
                    dpi = (dpi[0][0] / dpi[0][1] if dpi[0][1] else 300,
                           dpi[1][0] / dpi[1][1] if dpi[1][1] else 300)
                dx, dy = float(dpi[0]), float(dpi[1])
            else:
                dx = dy = 300.0
            return (w, h, int(round(dx)), int(round(dy)))
    except Exception:
        return (2480, 3508, 300, 300)


def discover_images(dir_path):
    """Lista de (nombre_archivo, width, height, dpi_x, dpi_y) ordenada por nombre."""
    out = []
    for name in sorted(os.listdir(dir_path)):
        low = name.lower()
        if any(low.endswith(ext) for ext in EXTENSIONES):
            full = os.path.join(dir_path, name)
            if os.path.isfile(full):
                w, h, dx, dy = get_image_info(full)
                out.append((name, w, h, dx, dy))
    return out


def build_filter_elements(filters_el, n):
    """Añade los nodos de los 6 filtros para n páginas (dimensiones genéricas por página)."""
    fix_orient = sub(filters_el, "fix-orientation")
    for i in range(1, n + 1):
        img = sub(fix_orient, "image", id=str(i))
        sub(img, "rotation", degrees="0")
    sub(fix_orient, "image-settings")

    sub(filters_el, "page-split", defaultLayoutType="single-uncut")

    deskew_el = sub(filters_el, "deskew")
    for pid in range(1, n + 1):
        page = sub(deskew_el, "page", id=str(pid))
        params = sub(page, "params", mode="auto", angle="0", oblique="0")
        deps = sub(params, "dependencies")
        sub(deps, "rotation", degrees="0")
        outline = sub(deps, "page-outline")
        for x, y in [(0, 0), (100, 0), (100, 100), (0, 100)]:
            sub(outline, "point", x=str(x), y=str(y))
    sub(deskew_el, "image-settings")

    select_el = sub(filters_el, "select-content")
    sub(select_el, "page-detection-box", width="0", height="0")
    select_el.set("pageDetectionTolerance", "0.1")
    for pid in range(1, n + 1):
        page = sub(select_el, "page", id=str(pid))
        params = sub(page, "params", contentDetectionMode="auto", pageDetectionMode="disabled", fineTuneCorners="0")
        sub(params, "content-rect", x="0", y="0", width="100", height="100")
        sub(params, "page-rect", x="0", y="0", width="100", height="100")
        sub(params, "content-size-mm", width="84.67", height="84.67")
        deps = sub(params, "dependencies")
        sub(deps, "rotation", degrees="0")

    layout_el = sub(filters_el, "page-layout")
    layout_el.set("showMiddleRect", "0")
    for pid in range(1, n + 1):
        page = sub(layout_el, "page", id=str(pid))
        params = sub(page, "params", autoMargins="1")
        sub(params, "hardMarginsMM", left="0", top="0", right="0", bottom="0")
        sub(params, "contentRect", x="0", y="0", width="100", height="100")
        sub(params, "pageRect", x="0", y="0", width="100", height="100")
        sub(params, "contentSizeMM", width="84.67", height="84.67")
        sub(params, "alignment", vert="top", hor="left")

    output_el = sub(filters_el, "output")
    for pid in range(1, n + 1):
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


def main():
    parser = argparse.ArgumentParser(
        description="Genera project.ScanTailor desde un directorio de imágenes (todo en automático) y opcionalmente ejecuta scantailor-cli."
    )
    parser.add_argument(
        "imagenes_dir",
        type=str,
        help="Directorio que contiene las imágenes (tif, tiff, png, jpg, jpeg, bmp)",
    )
    parser.add_argument(
        "--output-dir",
        type=str,
        default="out",
        help="Directorio de salida relativo al proyecto (default: out)",
    )
    parser.add_argument(
        "--project",
        type=str,
        default=None,
        help="Ruta del archivo .ScanTailor a crear (default: IMAGENES_DIR/project.ScanTailor)",
    )
    parser.add_argument(
        "--run",
        action="store_true",
        help="Ejecutar scantailor-cli sobre el proyecto generado",
    )
    parser.add_argument(
        "--scantailor-cli",
        type=str,
        default=None,
        help="Ruta al ejecutable scantailor-cli (default: buscar en build/ o en PATH)",
    )
    parser.add_argument(
        "--threads",
        type=int,
        default=None,
        help="Número de hilos para scantailor-cli (solo con --run)",
    )
    args = parser.parse_args()

    imagenes_dir = os.path.abspath(os.path.expanduser(args.imagenes_dir))
    if not os.path.isdir(imagenes_dir):
        print("Error: no existe el directorio:", imagenes_dir, file=sys.stderr)
        sys.exit(1)

    items = discover_images(imagenes_dir)
    if not items:
        print("Error: no se encontraron imágenes en", imagenes_dir, file=sys.stderr)
        print("Extensiones admitidas:", ", ".join(EXTENSIONES), file=sys.stderr)
        sys.exit(1)

    project_path = args.project
    if project_path is None:
        project_path = os.path.join(imagenes_dir, "project.ScanTailor")
    else:
        project_path = os.path.abspath(os.path.expanduser(project_path))

    n = len(items)
    print("Imágenes encontradas:", n)
    if not HAS_PILLOW:
        print("(Pillow no instalado: se usan tamaño/DPI por defecto; instala Pillow para valores reales)")

    # Proyecto: directorio de imágenes = mismo dir que el .ScanTailor → path "."
    project = ET.Element(
        "project",
        version="3",
        outputDirectory=args.output_dir,
        layoutDirection="LTR",
    )

    dirs = sub(project, "directories")
    sub(dirs, "directory", id="1", path=".")

    files_el = sub(project, "files")
    images_el = sub(project, "images")
    for i, (name, w, h, dx, dy) in enumerate(items, start=1):
        sub(files_el, "file", id=str(i), dirId="1", name=name)
        img = sub(images_el, "image", id=str(i), subPages="1", fileId=str(i), fileImage="1")
        sub(img, "size", width=str(w), height=str(h))
        sub(img, "dpi", horizontal=str(dx), vertical=str(dy))

    pages_el = sub(project, "pages")
    for i in range(1, n + 1):
        attrs = {"id": str(i), "imageId": str(i), "subPage": "single"}
        if i == 1:
            attrs["selected"] = "selected"
        sub(pages_el, "page", **attrs)

    disambig = sub(project, "file-name-disambiguation")
    for i in range(1, n + 1):
        sub(disambig, "mapping", file=str(i), label=str(i))

    filters_el = sub(project, "filters")
    build_filter_elements(filters_el, n)

    rough = ET.tostring(project, default_namespace="", encoding="unicode")
    pretty = minidom.parseString(rough).toprettyxml(indent="  ", encoding=None)
    with open(project_path, "w", encoding="utf-8") as f:
        f.write(pretty)
    print("Proyecto escrito:", project_path)

    if args.run:
        cli = args.scantailor_cli
        if not cli:
            script_dir = os.path.dirname(os.path.abspath(__file__))
            repo_root = os.path.dirname(script_dir)
            build_cli = os.path.join(repo_root, "build", "scantailor-cli")
            if os.path.isfile(build_cli):
                cli = build_cli
            else:
                cli = "scantailor-cli"
        cmd = [cli, project_path]
        if args.threads is not None:
            cmd.extend(["--threads", str(args.threads)])
        print("Ejecutando:", " ".join(cmd))
        r = subprocess.run(cmd)
        sys.exit(r.returncode)


if __name__ == "__main__":
    main()
