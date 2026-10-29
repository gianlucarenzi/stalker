#!/bin/bash

export KILIBPATH=$HOME/Progetti/RetroBitLab-Library

# --- Setup and Auto-Install for eeplot ---
LOCAL_UTILS_DIR="$(pwd)/utilities"
EEPLOT_SRC_DIR="eeplot-src" # Relative path for source
EEPLOT_EXEC="${LOCAL_UTILS_DIR}/bin/eeplot"
EESHOW_EXEC="${LOCAL_UTILS_DIR}/bin/eeshow" # Assuming eeshow is also built
EEPLOT_PATCHES=("$(pwd)"/patches/eeplot-*.patch)
# Patches the installed eeplot was built with (see eeplot_patches_id)
EEPLOT_STAMP="${LOCAL_UTILS_DIR}/bin/.eeplot-patches"

# Applies our local fixes to the eeplot source:
# - eeplot-page-size.patch: each schematic sheet keeps its own PDF page size
#   and is drawn flush with its own origin, instead of every page being
#   forced to the size of the largest sheet and left anchored top-left
# - eeplot-kicad5-schematics.patch: symbols of a library given without a
#   directory (e.g. the project -rescue.lib) are found, and embedded images
#   ($Bitmap) are skipped instead of stopping the plot
# Safe to call again: it detects an already-applied patch and skips it.
apply_eeplot_patches() {
    local patch
    for patch in "${EEPLOT_PATCHES[@]}"; do
        [ -f "${patch}" ] || continue
        if (cd "${EEPLOT_SRC_DIR}" && git apply --check "${patch}" 2>/dev/null); then
            echo "Applying local patch: $(basename "${patch}")"
            (cd "${EEPLOT_SRC_DIR}" && git apply "${patch}") || {
                echo "ERROR: failed to apply ${patch}."
                exit 1
            }
        elif (cd "${EEPLOT_SRC_DIR}" && git apply --check --reverse "${patch}" 2>/dev/null); then
            echo "Local patch $(basename "${patch}") already applied, skipping."
        else
            echo "--------------------------------------------------------------------"
            echo "ERROR: ${patch} does not apply cleanly to ${EEPLOT_SRC_DIR}."
            echo "The upstream eeplot source may have changed; the patch needs a refresh."
            echo "--------------------------------------------------------------------"
            exit 1
        fi
    done
}

eeplot_patches_id() {
    cat "${EEPLOT_PATCHES[@]}" 2>/dev/null | md5sum | cut -d' ' -f1
}

# System packages needed to build eeplot (fig2dev converts the eeshow icons)
check_eeplot_deps() {
    local missing=""
    for pkg in libgit2-dev fig2dev; do
        dpkg -s "${pkg}" >/dev/null 2>&1 || missing="${missing} ${pkg}"
    done
    if [ -n "${missing}" ]; then
        echo "--------------------------------------------------------------------"
        echo "ERROR: System dependencies not found:${missing}"
        echo "Please run the following command to install them:"
        echo "sudo apt-get install${missing}"
        echo "--------------------------------------------------------------------"
        exit 1
    fi
    echo "Dependencies OK."
}

# A binary built on another Debian release can miss its libraries (e.g.
# libgit2.so.1.5 after moving from Debian 12 to 13): drop it to rebuild it
if [ -f "${EEPLOT_EXEC}" ] && ldd "${EEPLOT_EXEC}" | grep -q 'not found'; then
    echo "--- eeplot was built against other system libraries. Rebuilding. ---"
    rm -f "${EEPLOT_EXEC}" "${EESHOW_EXEC}"
fi

# Rebuild it also when it was built with other local patches (e.g. a new
# one was added to patches/)
if [ -f "${EEPLOT_EXEC}" ] && [ "$(cat "${EEPLOT_STAMP}" 2>/dev/null)" != "$(eeplot_patches_id)" ]; then
    echo "--- eeplot was built without the current local patches. Rebuilding. ---"
    rm -f "${EEPLOT_EXEC}" "${EESHOW_EXEC}"
fi

# Check if eeplot is already installed locally
if [ -f "${EEPLOT_EXEC}" ]; then
    echo "--- eeplot found locally. Skipping setup. ---"
elif [ -d "${EEPLOT_SRC_DIR}" ]; then
    echo "--- eeplot source found. Updating and rebuilding. ---"
    # 1. Check for system dependencies (still needed for rebuilds)
    echo "[1/5] Checking system dependencies..."
    check_eeplot_deps

    # 2. Update source
    echo "[2/5] Updating eeplot source (git pull)..."
    (cd "${EEPLOT_SRC_DIR}" && git pull) || { echo "ERROR: git pull failed."; exit 1; }

    # 3. Apply local patches
    echo "[3/5] Applying local patches..."
    apply_eeplot_patches

    # 4. Compile
    echo "[4/5] Compiling eeplot..."
    if ! make -C "${EEPLOT_SRC_DIR}"; then
        echo "ERROR: Compilation (make) failed during update."
        exit 1
    fi

    # 5. Install locally (manual copy)
    cp "${EEPLOT_SRC_DIR}/eeplot" "${LOCAL_UTILS_DIR}/bin/" || { echo "ERROR: Copy 'eeplot' failed."; exit 1; }
    if [ -f "${EEPLOT_SRC_DIR}/eeshow" ]; then
        cp "${EEPLOT_SRC_DIR}/eeshow" "${LOCAL_UTILS_DIR}/bin/" || { echo "ERROR: Copy 'eeshow' failed."; exit 1; }
    fi
    eeplot_patches_id > "${EEPLOT_STAMP}"
    echo "--- eeplot update and rebuild complete. ---"
else
    echo "--- eeplot not found locally. Starting one-time setup procedure ---"
    # 1. Check for system dependencies
    echo "[1/6] Checking system dependencies..."
    check_eeplot_deps

    # 2. Clone the repository
    echo "[2/6] Cloning eeplot repository..."
    # rm -rf "${EEPLOT_SRC_DIR}" # Not needed here, as we are in the 'else' branch, meaning it doesn't exist
    if ! git clone https://github.com/DavidWKnight/eeshow-Kicad5-eeplot.git "${EEPLOT_SRC_DIR}"; then
        echo "ERROR: Unable to clone repository. Check connection or URL."
        exit 1
    fi

    # 3. Apply local patches
    echo "[3/6] Applying local patches..."
    apply_eeplot_patches

    # 4. Compile
    echo "[4/6] Compiling eeplot..."
    if ! make -C "${EEPLOT_SRC_DIR}"; then
        echo "ERROR: Compilation (make) failed."
        rm -rf "${EEPLOT_SRC_DIR}"
        exit 1
    fi

    # 5. Install locally (manual copy due to make install issues)
    echo "[5/6] Installing eeplot into ${LOCAL_UTILS_DIR} (manual copy)..."
    mkdir -p "${LOCAL_UTILS_DIR}/bin"
    cp "${EEPLOT_SRC_DIR}/eeplot" "${LOCAL_UTILS_DIR}/bin/" || { echo "ERROR: Copy 'eeplot' failed."; exit 1; }
    if [ -f "${EEPLOT_SRC_DIR}/eeshow" ]; then
        cp "${EEPLOT_SRC_DIR}/eeshow" "${LOCAL_UTILS_DIR}/bin/" || { echo "ERROR: Copy 'eeshow' failed."; exit 1; }
    fi

    # 6. Cleanup
    echo "[6/6] Cleaning up source files..."
    rm -rf "${EEPLOT_SRC_DIR}"

    eeplot_patches_id > "${EEPLOT_STAMP}"
    echo "--- eeplot installation complete. ---"
fi

# Add local utilities to the PATH for this script execution
export PATH="${LOCAL_UTILS_DIR}/bin:${PATH}"
# --- End of Setup ---


PCBNAME=StandAlone-Adapter
PCBNAMEOUT=${PCBNAME}
PDFOUTPUT=schematics.pdf
PYTHON3=$(which python3)

# Official KiCad 5 symbol libraries for eeplot: the system ones (e.g. Debian
# 11), otherwise a copy taken once from the KiCad 5 AppImage
KICAD5_SYMBOLS=/usr/share/kicad/library
if [ ! -d "${KICAD5_SYMBOLS}" ]; then
	KICAD5_SYMBOLS=${HOME}/.cache/kicad5/library
	if [ ! -d "${KICAD5_SYMBOLS}" ] && command -v kicad5 >/dev/null 2>&1; then
		echo "Copying the KiCad 5 symbol libraries from the AppImage..."
		kicad5 --python -c 'import os, shutil, sys; shutil.copytree(os.environ["KICAD_SYMBOL_DIR"], sys.argv[1] + ".partial"); os.rename(sys.argv[1] + ".partial", sys.argv[1])' "${KICAD5_SYMBOLS}"
	fi
fi

# Official KiCad 5 3D models for kicad-cli: the libraries of KiCad 7+ have no
# .wrl models, so the ${KISYS3DMOD}/...wrl of these boards would be missing
# from the STEP/VRML. The system ones (e.g. Debian 11), otherwise a copy
# taken once from the KiCad 5 AppImage (about 5 GB)
KICAD5_3DMODELS=/usr/share/kicad/modules/packages3d
if [ ! -d "${KICAD5_3DMODELS}" ]; then
	KICAD5_3DMODELS=${HOME}/.cache/kicad5/packages3d
	if [ ! -d "${KICAD5_3DMODELS}" ] && command -v kicad5 >/dev/null 2>&1; then
		echo "Copying the KiCad 5 3D models from the AppImage (about 5 GB, only once)..."
		kicad5 --python -c 'import os, shutil, sys; shutil.copytree(os.environ["KISYS3DMOD"], sys.argv[1] + ".partial"); os.rename(sys.argv[1] + ".partial", sys.argv[1])' "${KICAD5_3DMODELS}"
	fi
fi

ARGS_NUM=$#
# Lanciare con la versione del PCB!!

if [ ${ARGS_NUM} -lt 1 ]; then
	echo "Need the PCB Version!"
	exit 1
fi

# Need KiCAD CLI from a v7+ version installed
KICADCLI=$(which kicad-cli)
if [ "${KICADCLI}" == "" ]; then
	echo "Need to install kicad-cli from a version of KiCAD v7 or greater"
	echo "(on Debian 11 I am using the flatpak option). Put the script"
	echo "into a path available for shell..."
	exit 1
fi

# La versione e' il primo argomento
PCBVER=$1
PCBNAMEOUT=${PCBNAMEOUT}-v${PCBVER}

# La prima volta va generato il file xml!
if [ ! -f ${PCBNAME}.xml ]; then
	echo "Need to run once the BOM Generator from EESCHEMA / Schematic Designer"
	exit 1
fi

# Creiamo la BOM per JLCPCB
${PYTHON3} generate_bom.py ${PCBNAME}.xml ${PCBNAME}.csv

echo "Creating BOM for JLCPCB"
${PYTHON3} jlcpcb-check-bom.py ${PCBNAME}.csv ${PCBNAMEOUT}-JLCPCB-BOM.csv

# Generiamo la CPL con uno script simile al plugin di PCBNEW!!!!
./run_generate_cpl.sh ${PCBNAME}.kicad_pcb
if [ $? -ne 0 ]; then
	echo "Error on generating CPL files."
	exit 1
fi

echo "Creating JLCPCB CPL"
if [ -f ${PCBNAME}_cpl_top.csv ]; then
	# Verify if the bot is present
	if [ -f ${PCBNAME}_cpl_bot.csv ]; then
		# We are 2 side board
		./clean_virtual.sh ${PCBNAME}_cpl_top.csv ${PCBNAME}_cpl_bot.csv --pcb ${PCBNAME}.kicad_pcb
		cat ${PCBNAME}_cpl_top.csv <(tail -n +2 ${PCBNAME}_cpl_bot.csv) > ${PCBNAMEOUT}-JLCPCB-CPL.csv
	else
		# We have only TOP Side components!
		./clean_virtual.sh ${PCBNAME}_cpl_top.csv --pcb ${PCBNAME}.kicad_pcb
		cp ${PCBNAME}_cpl_top.csv ${PCBNAMEOUT}-JLCPCB-CPL.csv
	fi
else
	# Verify if the bot is present
	if [ -f ${PCBNAME}_cpl_bot.csv ]; then
		# We have only BOT side components
		./clean_virtual.sh ${PCBNAME}_cpl_bot.csv --pcb ${PCBNAME}.kicad_pcb
		cp ${PCBNAME}_cpl_bot.csv ${PCBNAMEOUT}-JLCPCB-CPL.csv
	else
		# There is no top and no bottom!
		echo "No CPL files present error!"
		exit 1
	fi
fi

echo "Creating PDF..."
EEPLOT_FILES=(
    ${KICAD5_SYMBOLS}/Connector_Generic.lib
    ${KICAD5_SYMBOLS}/Device.lib
    ${KICAD5_SYMBOLS}/Regulator_Linear.lib
    ${KILIBPATH}/RetroBitLab.lib
    ${PCBNAME}-rescue.lib
    ${PCBNAME}.sch
)
# The table of contents breaks with cairo 1.18 (Debian 13): eeplot aborts
# with "pdftoc_end: Assertion", so then make the PDF without it (-T)
if ! eeplot -o ${PDFOUTPUT} "${EEPLOT_FILES[@]}"; then
	echo "eeplot failed, creating the PDF without table of contents..."
	eeplot -T -o ${PDFOUTPUT} "${EEPLOT_FILES[@]}" || { echo "Error on creating the PDF."; exit 1; }
fi

echo "Creating STEP 3D object..."
KISYS3DMOD=${KICAD5_3DMODELS} ${KICADCLI} pcb export step -o ${PCBNAME}.step --force --grid-origin --no-dnp --subst-models ${PCBNAME}.kicad_pcb

echo "Creating VRML 3D object..."
KISYS3DMOD=${KICAD5_3DMODELS} ${KICADCLI} pcb export vrml -o ${PCBNAME}.wrl --force --no-dnp ${PCBNAME}.kicad_pcb

# Remove the existing
rm ${PCBNAMEOUT}.zip 2>/dev/null
echo "Generating JLCPCB Project"
cd production
7z -tzip a ../${PCBNAMEOUT}.zip *
exit 0
