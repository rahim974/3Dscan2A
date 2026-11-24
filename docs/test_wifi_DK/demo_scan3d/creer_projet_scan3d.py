#!/usr/bin/env python3
"""Crée le projet STM32CubeIDE ST67W6X_Scan3D_DK (démo scanner 3D autonome avec écran).

Prérequis : avoir cloné x-cube-st67w61 et lancé port_STM32N6570-DK.py
(le dossier ST67W6X_CLI_DK doit exister).

Usage (depuis n'importe où) :
    python3 creer_projet_scan3d.py <racine x-cube-st67w61> <wifi_secrets.h>

Le script :
  - copie ST67W6X_CLI_DK en ST67W6X_Scan3D_DK ;
  - récupère (git clone) le BSP LCD du STM32N6570-DK, le pilote RK050HR18, lcd.h et
    les polices ST, puis les copie dans FSBL/Appli/App avec firmware/*.c|h ;
  - ajoute les fichiers au projet Eclipse (.project), active LTDC/DMA2D/RIF dans la HAL ;
  - les framebuffers LCD sont en AXISRAM3-6 (0x34200000-0x3437B800), hors RAM du programme ;
  - lance la démo depuis main_app() (Scan3D_Start()).
wifi_secrets.h (SSID, mot de passe, IP du PC) n'est jamais versionné : voir
firmware/wifi_secrets_template.h.
"""
import pathlib, shutil, subprocess, sys

ICI = pathlib.Path(__file__).resolve().parent
root = pathlib.Path(sys.argv[1]).resolve()
secrets = pathlib.Path(sys.argv[2]).resolve()
apps = root / "Projects/NUCLEO-N657X0-Q/Applications/ST67W6X"
src, dst = apps / "ST67W6X_CLI_DK", apps / "ST67W6X_Scan3D_DK"
if not src.exists():
    sys.exit(f"{src} introuvable : lancer d'abord port_STM32N6570-DK.py")
if dst.exists():
    sys.exit(f"{dst} existe déjà")
if not secrets.exists():
    sys.exit(f"{secrets} introuvable (copier firmware/wifi_secrets_template.h)")

# --- dépendances ST (licence BSD-3, téléchargées, non versionnées ici) ---------
ext = root / "_ext_scan3d"
ext.mkdir(exist_ok=True)
def clone(repo, sparse=None):
    d = ext / repo
    if not d.exists():
        cmd = ["git", "clone", "-q", "--depth", "1"]
        if sparse:
            cmd += ["--filter=blob:none", "--sparse"]
        subprocess.check_call(cmd + [f"https://github.com/STMicroelectronics/{repo}", str(d)])
        if sparse:
            subprocess.check_call(["git", "-C", str(d), "sparse-checkout", "set", *sparse])
    return d
bsp = clone("stm32n6570-dk-bsp")
rk = clone("stm32-rk050hr18")
common = clone("stm32-bsp-common")
fonts = clone("STM32CubeN6", ["Utilities/Fonts"]) / "Utilities/Fonts"

shutil.copytree(src, dst, ignore=shutil.ignore_patterns("Release", "Debug"))
app = dst / "FSBL/Appli/App"
for f in (ICI / "firmware").iterdir():
    if f.suffix in (".c", ".h") and f.name != "wifi_secrets_template.h":
        shutil.copy(f, app)
shutil.copy(secrets, app / "wifi_secrets.h")
for f in ("stm32n6570_discovery_lcd.c", "stm32n6570_discovery_lcd.h", "stm32n6570_discovery_errno.h"):
    shutil.copy(bsp / f, app)
shutil.copy(rk / "rk050hr18.h", app)
shutil.copy(common / "lcd.h", app)
for f in ("fonts.h", "font24.c", "font20.c", "font16.c"):
    shutil.copy(fonts / f, app)

def patch(path, pairs):
    t = path.read_text()
    for old, new in pairs:
        if old not in t:
            sys.exit(f"Motif introuvable dans {path} :\n{old}")
        t = t.replace(old, new, 1)
    path.write_text(t)
    print("modifié :", path.relative_to(dst))

patch(app / "stm32n6570_discovery_lcd.h",
      [('#include "../Components/rk050hr18/rk050hr18.h"', '#include "rk050hr18.h"')])

patch(dst / "FSBL/Core/Inc/stm32n6xx_hal_conf.h", [
    ("/*#define HAL_DMA2D_MODULE_ENABLED   */", "#define HAL_DMA2D_MODULE_ENABLED"),
    ("/*#define HAL_LTDC_MODULE_ENABLED   */", "#define HAL_LTDC_MODULE_ENABLED"),
    ("/*#define HAL_RIF_MODULE_ENABLED   */", "#define HAL_RIF_MODULE_ENABLED"),
])

patch(app / "main_app.c", [
    ("/* USER CODE BEGIN Includes */\n", "/* USER CODE BEGIN Includes */\n#include \"scan3d_demo.h\"\n"),
    ("  /* USER CODE BEGIN main_app_3 */\n", "  /* USER CODE BEGIN main_app_3 */\n  Scan3D_Start();\n"),
])

links = ""
for f in ("scan3d_demo.c", "lcd_status.c", "stm32n6570_discovery_lcd.c", "font24.c", "font20.c", "font16.c"):
    links += (f"\t\t<link>\n\t\t\t<name>Application/User/Appli/App/{f}</name>\n\t\t\t<type>1</type>\n"
              f"\t\t\t<locationURI>PARENT-2-PROJECT_LOC/FSBL/Appli/App/{f}</locationURI>\n\t\t</link>\n")
for f in ("ltdc", "ltdc_ex", "dma2d", "rif"):
    n = f"stm32n6xx_hal_{f}.c"
    links += (f"\t\t<link>\n\t\t\t<name>Drivers/STM32N6xx_HAL_Driver/{n}</name>\n\t\t\t<type>1</type>\n"
              f"\t\t\t<locationURI>PARENT-7-PROJECT_LOC/Drivers/STM32N6xx_HAL_Driver/Src/{n}</locationURI>\n\t\t</link>\n")
patch(dst / "STM32CubeIDE/FSBL/.project", [
    ("<name>ST67W6X_CLI_FSBL</name>", "<name>ST67W6X_Scan3D_FSBL</name>"),
    ("\t<linkedResources>\n", "\t<linkedResources>\n" + links),
])

print(f"\nProjet prêt : {dst}\nCompiler le projet Eclipse STM32CubeIDE/FSBL (nom ST67W6X_Scan3D_FSBL), "
      "puis signer et flasher comme ST67W6X_CLI_DK.")
